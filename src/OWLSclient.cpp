//
// Created by stephane bourque on 2021-03-12.
//
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sys/time.h>
#include <thread>
#include <tuple>

#include "OWLS_utils.h"

#include "Poco/NObserver.h"
#include "Poco/Net/Context.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Net/SSLException.h"
#include "Poco/URI.h"

#include "OWLSclient.h"
#include "OWLSdefinitions.h"
#include "framework/MicroServiceFuncs.h"

#include "SimStats.h"
#include "SimulationCoordinator.h"
#include "fmt/format.h"
#include <nlohmann/json.hpp>

using namespace std::chrono_literals;

namespace OpenWifi {

	OWLSclient::OWLSclient(std::string SerialNumber,
                           Poco::Logger &Logger, SimulationRunner *runner)
		: Logger_(Logger), SerialNumber_(std::move(SerialNumber)),
          Memory_(1),
          Load_(1),
          Runner_(runner) {

		AllInterfaceNames_[ap_interface_types::upstream] = "up0v0";
		AllInterfaceNames_[ap_interface_types::downstream] = "down0v0";
		AllInterfaceRoles_[ap_interface_types::upstream] = "upstream";
		AllInterfaceRoles_[ap_interface_types::downstream] = "downstream";
		AllPortNames_[ap_interface_types::upstream] = "eth0";
		AllPortNames_[ap_interface_types::downstream] = "eth1";
		SetFirmware();
		Active_ = UUID_ = Utils::Now();
		srand(UUID_);
		mac_lan = OWLSutils::MakeMac(SerialNumber_.c_str(), 0);
		CurrentConfig_ = SimulationCoordinator()->GetSimConfiguration(Utils::Now());
		UpdateConfiguration();
        Valid_ = true;
	}

    void OWLSclient::CreateLanClients(uint64_t min, uint64_t max) {
        AllLanClients_.clear();
        uint64_t Num = MicroServiceRandom(min, max);
        for (uint64_t i = 0; i < Num; i++) {
            MockLanClient CI;
            CI.mac = OWLSutils::RandomMAC();
            CI.ipv4_addresses.push_back(OWLSutils::RandomIPv4());
            CI.ipv6_addresses.push_back(OWLSutils::RandomIPv6());
            CI.ports.emplace_back("eth1");
            AllLanClients_.push_back(CI);
        }
        Load_.SetSize(AllLanClients_.size()+CountAssociations());
        Memory_.SetSize(AllLanClients_.size()+CountAssociations());
    }

    void OWLSclient::CreateAssociations(const interface_location_t &interface, const std::string &bssid, uint64_t min,
                                        uint64_t max) {

        auto interface_hint = AllAssociations_.find(interface);
        if(interface_hint==end(AllAssociations_)) {
            MockAssociations M;
            AllAssociations_[interface] = M;
            interface_hint = AllAssociations_.find(interface);
        }

        interface_hint->second.clear();
        auto NumberOfAssociations = MicroServiceRandom(min, max);
        while (NumberOfAssociations) {
            MockAssociation FA;
            FA.bssid = bssid;
            FA.station = OWLSutils::RandomMAC();
            FA.ack_signal_avg = OWLSutils::local_random(-40, -60);
            FA.ack_signal = FA.ack_signal_avg;
            FA.ipaddr_v4 = OWLSutils::RandomIPv4();
            FA.ipaddr_v6 = OWLSutils::RandomIPv6();
            FA.rssi = OWLSutils::local_random(-40, -90);
            interface_hint->second.push_back(FA);
            --NumberOfAssociations;
        }
        Load_.SetSize(AllLanClients_.size()+CountAssociations());
        Memory_.SetSize(AllLanClients_.size()+CountAssociations());
    }

    void OWLSclient::Update() {
        Memory_.next();
        Load_.next();
        for(auto &[_,radio]:AllRadios_) {
            radio.next();
        }
        for(auto &[_,counters]:AllCounters_) {
            counters.next();
        }
        for(auto &[_,associations]:AllAssociations_) {
            for(auto &association:associations) {
                association.next();
            }
        }
        for(auto &lan_client:AllLanClients_) {
            lan_client.next();
        }
    }

    bool OWLSclient::FindInterfaceRole(const std::string &role,
                                       OpenWifi::ap_interface_types &interface) {
		for (const auto &[interface_type, interface_name] : AllInterfaceRoles_) {
			if (role == interface_name) {
				interface = interface_type;
				return true;
			}
		}
		return false;
	}

	void OWLSclient::Reset() {

		for (auto &[_, radio] : AllRadios_) {
			radio.reset();
		}

		for (auto &[_, association_list] : AllAssociations_) {
			for (auto &association : association_list) {
				association.reset();
			}
		}

		for (auto &[_, counter] : AllCounters_) {
			counter.reset();
		}
	}

	void OWLSclient::UpdateConfiguration() {
		//  go through the config and harvest the SSID names, also update all the client stuff
		auto Interfaces = CurrentConfig_["interfaces"];
		AllAssociations_.clear();
		AllLanClients_.clear();
		AllRadios_.clear();
		bssid_index = 1;
		for (const auto &interface : Interfaces) {
			if (interface.contains("role")) {
				ap_interface_types current_interface_role = upstream;
				if (FindInterfaceRole(interface["role"], current_interface_role)) {
					auto SSIDs = interface["ssids"];
					for (const auto &ssid : SSIDs) {
						for (const auto &band : ssid["wifi-bands"]) {
							auto ssidName = ssid["name"];
                            auto bssid_num = Utils::SerialToMAC(Utils::IntToSerialNumber(
                                    Utils::SerialNumberToInt(SerialNumber_) +
                                    bssid_index++));
							if (band == "2G") {
                                auto index = std::make_tuple(current_interface_role, ssidName,
                                                             radio_bands::band_2g);
									CreateAssociations(index, bssid_num,
                                                       Runner_->Details().minAssociations,
                                                       Runner_->Details().maxAssociations);
							}
							if (band == "5G") {
                                auto index = std::make_tuple(current_interface_role, ssidName,
                                                             radio_bands::band_5g);
									CreateAssociations(index, bssid_num,
                                                       Runner_->Details().minAssociations,
                                                       Runner_->Details().maxAssociations);
							}
							if (band == "6G") {
                                auto index = std::make_tuple(current_interface_role, ssidName,
                                                             radio_bands::band_6g);
									CreateAssociations(index,bssid_num,
                                                       Runner_->Details().minAssociations,
                                                       Runner_->Details().maxAssociations);
							}
						}
					}
					MockCounters F;
					AllCounters_[current_interface_role] = F;
				}
			}
		}

		CreateLanClients(Runner_->Details().minClients, Runner_->Details().maxClients);

		auto radios = CurrentConfig_["radios"];
		uint index = 0;
		for (const auto &radio : radios) {
			auto band = radio["band"];
			MockRadio R;

            R.band.push_back(band);
            if (band == "2G") {
                R.radioBands = radio_bands::band_2g;
            } else if (band == "5G") {
                R.radioBands = radio_bands::band_5g;
            } else if (band == "6G") {
                R.radioBands =  radio_bands::band_6g;
            }

            if(radio.contains("channel-width") && radio["channel-width"].is_number_integer())
                R.channel_width = radio["channel-width"];
            else
                R.channel_width = 20;

			if ((!radio.contains("channel"))
                ||  (radio.contains("channel") && radio["channel"].is_string() && radio["channel"] == "auto")
                ||  (!radio["channel"].is_number_integer())) {
                R.channel = OWLSutils::FindAutoChannel(R.radioBands, R.channel_width);
            } else if (radio["channel"].is_number_integer()) {
                R.channel = radio["channel"];
			}

            OWLSutils::FillinFrequencies(R.channel, R.radioBands, R.channel_width, R.channels, R.frequency);

			OWLSutils::AssignIfPresent(radio, "tx_power", R.tx_power, (uint_fast64_t)23);

			if (index == 0)
				R.phy = "platform/soc/c000000.wifi";
			else
				R.phy = "platform/soc/c000000.wifi+" + std::to_string(index);
			R.index = index;
			AllRadios_[R.radioBands] = R;
			++index;
		}
	}

	nlohmann::json OWLSclient::CreateLinkState() {
		nlohmann::json res;
		for (const auto &[interface_type, _] : AllCounters_) {
			res[AllInterfaceRoles_[interface_type]][AllPortNames_[interface_type]]["carrier"] = 1;
			res[AllInterfaceRoles_[interface_type]][AllPortNames_[interface_type]]["duplex"] =
				"full";
			res[AllInterfaceRoles_[interface_type]][AllPortNames_[interface_type]]["speed"] = 1000;
		}
		return res;
	}

    Poco::JSON::Object OWLSclient::CreateLinkStatePtr() {
        Poco::JSON::Object res;
        for (const auto &[interface_type, _] : AllCounters_) {
            Poco::JSON::Object InterfaceInfo, InterfacePort;
            InterfaceInfo.set("carrier",1);
            InterfaceInfo.set("duplex","full");
            InterfaceInfo.set("speed",1000);
            InterfacePort.set(AllPortNames_[interface_type],InterfaceInfo);
            res.set(AllInterfaceRoles_[interface_type], InterfacePort);
/*

            res[AllInterfaceRoles_[interface_type]][AllPortNames_[interface_type]]["carrier"] = 1;
            res[AllInterfaceRoles_[interface_type]][AllPortNames_[interface_type]]["duplex"] =
                    "full";
            res[AllInterfaceRoles_[interface_type]][AllPortNames_[interface_type]]["speed"] = 1000;
*/
        }
        return res;
    }


    nlohmann::json OWLSclient::CreateState() {
		nlohmann::json State;

		State["version"] = 1;

        DEBUG_LINE("start");
		auto now = Utils::Now();
		Memory_.to_json(State);
        Load_.to_json(State);
		State["unit"]["localtime"] = now;
		State["unit"]["uptime"] = now - StartTime_;
        State["unit"]["temperature"] = std::vector<std::int64_t> { OWLSutils::local_random(48,58), OWLSutils::local_random(48,58)};

		for (auto &[_, radio] : AllRadios_) {
			radio.next();
            nlohmann::json doc;
            radio.to_json(doc);
			State["radios"].push_back(doc);
		}

		//  set the link state
		State["link-state"] = CreateLinkState();

		nlohmann::json all_interfaces;
		for (const auto &ap_interface_type :
			 {ap_interface_types::upstream, ap_interface_types::downstream}) {
			if (AllCounters_.find(ap_interface_type) != AllCounters_.end()) {
				nlohmann::json current_interface;
				nlohmann::json up_ssids;
				uint64_t ssid_num = 0, interfaces = 0;

				auto ue_clients = nlohmann::json::array();
				for (auto &[interface, associations] : AllAssociations_) {
					auto &[interface_type, ssid, band] = interface;
					if (interface_type == ap_interface_type) {
						nlohmann::json association_list;
						std::string bssid;
						for (auto &association : associations) {
							association.next();
							bssid = association.bssid;
                            nlohmann::json doc;
                            association.to_json(doc);
							association_list.push_back(doc);
							nlohmann::json ue;
							ue["mac"] = association.station;
							ue["ipv4_addresses"].push_back(association.ipaddr_v4);
							ue["ipv6_addresses"].push_back(association.ipaddr_v6);
                            if(interface_type==upstream)
							    ue["ports"].push_back("wwan0");
                            else
                                ue["ports"].push_back("wlan0");
                            ue["last_seen"] = 0 ;
                            ue_clients.push_back(ue);
						}
						nlohmann::json ssid_info;
						ssid_info["associations"] = association_list;
						ssid_info["bssid"] = bssid;
						ssid_info["mode"] = "ap";
						ssid_info["ssid"] = ssid;
						ssid_info["phy"] = AllRadios_[band].phy;
						ssid_info["location"] = "/interfaces/" + std::to_string(interfaces) +
												"/ssids/" + std::to_string(ssid_num++);
						ssid_info["radio"]["$ref"] =
							"#/radios/" + std::to_string(AllRadios_[band].index);
						ssid_info["name"] = AllInterfaceNames_[ap_interface_type];
						up_ssids.push_back(ssid_info);
					}
				}
				current_interface["ssids"] = up_ssids;
				AllCounters_[ap_interface_type].next();
                nlohmann::json doc;
                AllCounters_[ap_interface_type].to_json(doc);
				current_interface["counters"] = doc;

				//  if we have 2 interfaces, then the clients go to the downstream interface
				//  if we only have 1 interface then this is bridged and therefore clients go on the
				//  upstream
				if ((AllCounters_.size() == 1 &&
					 ap_interface_type == ap_interface_types::upstream) ||
					(AllCounters_.size() == 2 &&
					 ap_interface_type == ap_interface_types::downstream)) {
					nlohmann::json state_lan_clients;
					for (const auto &lan_client : AllLanClients_) {
                        nlohmann::json d;
                        lan_client.to_json(d);
						state_lan_clients.push_back(d);
					}
					for (const auto &ue_client : ue_clients) {
						state_lan_clients.push_back(ue_client);
					}
					current_interface["clients"] = state_lan_clients;
				}
				current_interface["name"] = AllInterfaceNames_[ap_interface_type];
				all_interfaces.push_back(current_interface);
			}
		}
		State["interfaces"] = all_interfaces;

		return State;
	}


    Poco::JSON::Object OWLSclient::CreateStatePtr() {
        Poco::JSON::Object  State,Unit;


        DEBUG_LINE("start");
        auto now = Utils::Now();
        Memory_.to_json(Unit);
        Load_.to_json(Unit);
        Unit.set("localtime", now);
        Unit.set("uptime",  now - StartTime_);
        Unit.set("temperature", std::vector<std::int64_t> { OWLSutils::local_random(48,58), OWLSutils::local_random(48,58)});

        DEBUG_LINE(__FILE_NAME__);
        Poco::JSON::Array RadioArray;
        for (auto &[_, radio] : AllRadios_) {
            Poco::JSON::Object doc;
            radio.to_json(doc);
            RadioArray.add(doc);
        }

        DEBUG_LINE(__FILE_NAME__);
        Poco::JSON::Array all_interfaces;
        for (const auto &ap_interface_type :
                {ap_interface_types::upstream, ap_interface_types::downstream}) {
            if (AllCounters_.find(ap_interface_type) != AllCounters_.end()) {
                DEBUG_LINE(__FILE_NAME__);
                Poco::JSON::Object  current_interface;
                Poco::JSON::Array   ue_clients, up_ssids;
                uint64_t ssid_num = 0, interfaces = 0;

                DEBUG_LINE(__FILE_NAME__);
                for (auto &[interface, associations] : AllAssociations_) {
                    auto &[interface_type, ssid, band] = interface;
                    DEBUG_LINE(__FILE_NAME__);
                    if (interface_type == ap_interface_type) {
                        DEBUG_LINE(__FILE_NAME__);
                        Poco::JSON::Array association_list;
                        std::string bssid;
                        for (auto &association : associations) {
                            bssid = association.bssid;
                            Poco::JSON::Object doc;
                            association.to_json(doc);
                            association_list.add(doc);
                            Poco::JSON::Object ue;
                            ue.set("mac", association.station);
                            ue.set("ipv4_addresses", std::vector<std::string>{association.ipaddr_v4});
                            ue.set("ipv6_addresses", std::vector<std::string>{association.ipaddr_v6});
                            if(interface_type==upstream)
                                ue.set("ports", std::vector<std::string>{"wwan0"});
                            else
                            ue.set("ports", std::vector<std::string>{"wlan0"});
                            ue.set("last_seen", 0);
                            ue_clients.add(ue);
                        }
                        DEBUG_LINE(__FILE_NAME__);
                        Poco::JSON::Object ssid_info;
                        ssid_info.set("associations", association_list);
                        ssid_info.set("bssid", bssid);
                        ssid_info.set("mode", "ap");
                        ssid_info.set("ssid", ssid);
                        ssid_info.set("phy", AllRadios_[band].phy);
                        ssid_info.set("location", "/interfaces/" + std::to_string(interfaces) +
                                                "/ssids/" + std::to_string(ssid_num++));
                        ssid_info.set("name", AllInterfaceNames_[ap_interface_type]);
                        Poco::JSON::Object  R;
                        R.set("$ref",
                                "#/radios/" + std::to_string(AllRadios_[band].index));
                        ssid_info.set("radio", R);
                        up_ssids.add(ssid_info);
                    }
                }
                DEBUG_LINE(__FILE_NAME__);
                current_interface.set("ssids", up_ssids);
                Poco::JSON::Object  C;
                AllCounters_[ap_interface_type].to_json(C);
                current_interface.set("counters", C);

                //  if we have 2 interfaces, then the clients go to the downstream interface
                //  if we only have 1 interface then this is bridged and therefore clients go on the
                //  upstream
                DEBUG_LINE(__FILE_NAME__);
                if ((AllCounters_.size() == 1 &&
                     ap_interface_type == ap_interface_types::upstream) ||
                    (AllCounters_.size() == 2 &&
                     ap_interface_type == ap_interface_types::downstream)) {
                    Poco::JSON::Array ip_clients;
                    for (const auto &lan_client : AllLanClients_) {
                        Poco::JSON::Object  d;
                        lan_client.to_json(d);
                        ip_clients.add(d);
                    }
                    for (const auto &ue_client : ue_clients) {
                        ip_clients.add(ue_client);
                    }
                    current_interface.set("clients", ip_clients);
                }
                current_interface.set("name", AllInterfaceNames_[ap_interface_type]);
                all_interfaces.add(current_interface);
            }
        }

        State.set("version" , 1 );
        State.set("radios", RadioArray);
        State.set("link-state", CreateLinkStatePtr());
        State.set("unit", Unit);
        State.set("interfaces", all_interfaces);

        return State;
    }

    void OWLSclient::DoConfigure(uint64_t Id, nlohmann::json &Params) {
		try {
            DEBUG_LINE("start");
			if (Params.contains("serial") && Params.contains("uuid") && Params.contains("config")) {
				uint64_t When = Params.contains("when") ? (uint64_t)Params["when"] : 0;
				auto Serial = Params["serial"];
				uint64_t UUID = Params["uuid"];
				auto Configuration = Params["config"];
				CurrentConfig_ = Configuration;
				UUID_ = Active_ = UUID;

				//  We need to digest the configuration and generate what we will need for
				//  state messages.
				auto Radios = CurrentConfig_["radios"];			//  array
				auto Metrics = CurrentConfig_["metrics"];		//  object
				auto Interfaces = CurrentConfig_["interfaces"]; //  array

				HealthInterval_ = Metrics["health"]["interval"];
				StatisticsInterval_ = Metrics["statistics"]["interval"];

				//  prepare response...
				nlohmann::json Answer;

				Answer["jsonrpc"] = "2.0";
				Answer["id"] = Id;
				Answer["result"]["serial"] = Serial;
				Answer["result"]["uuid"] = UUID;
				Answer["result"]["status"]["error"] = 0;
				Answer["result"]["status"]["when"] = When;
				Answer["result"]["status"]["text"] = "No errors were found";
				Answer["result"]["status"]["error"] = 0;
                poco_information(Logger_,fmt::format("configure({}): done.", SerialNumber_));
				SendObject(Answer);
			} else {
                poco_warning(Logger_,fmt::format("configure({}): Illegal command.", SerialNumber_));
			}
		} catch (const Poco::Exception &E) {
            DEBUG_LINE("exception 1");
            poco_warning(Logger_,
				fmt::format("configure({}): Exception. {}", SerialNumber_, E.displayText()));
		} catch (const std::exception &E) {
            DEBUG_LINE("exception2");
        }
	}

	void OWLSclient::DoReboot(uint64_t Id, nlohmann::json &Params) {
		try {
			if (Params.contains("serial")) {
				uint64_t When = Params.contains("when") ? (uint64_t)Params["when"] : 0;
				auto Serial = Params["serial"];

				//  prepare response...
				nlohmann::json Answer;

				Answer["jsonrpc"] = "2.0";
				Answer["id"] = Id;
				Answer["result"]["serial"] = Serial;
				Answer["result"]["status"]["error"] = 0;
				Answer["result"]["status"]["when"] = When;
				Answer["result"]["status"]["text"] = "No errors were found";

				SendObject(Answer);

				Logger_.information(fmt::format("reboot({}): done.", SerialNumber_));
				Reset();
			} else {
				Logger_.warning(fmt::format("reboot({}): Illegal command.", SerialNumber_));
			}
		} catch (const Poco::Exception &E) {
			Logger_.warning(
				fmt::format("reboot({}): Exception. {}", SerialNumber_, E.displayText()));
		}
	}

	std::string GetFirmware(const std::string &U) {
		Poco::URI uri(U);

		auto p = uri.getPath();
		auto tokens = Poco::StringTokenizer(p, "-");
		if (tokens.count() > 4 &&
			(tokens[2] == "main" || tokens[2] == "next" || tokens[2] == "staging")) {
			return "TIP-devel-" + tokens[3];
		}

		if (tokens.count() > 5) {
			return "TIP-" + tokens[2] + "-" + tokens[3] + "-" + tokens[4];
		}

		return p;
	}

	void OWLSclient::DoUpgrade(uint64_t Id, nlohmann::json &Params) {
		try {
			if (Params.contains("serial") && Params.contains("uri")) {

				uint64_t When = Params.contains("when") ? (uint64_t)Params["when"] : 0;
				auto Serial = to_string(Params["serial"]);
				auto URI = to_string(Params["uri"]);

				//  prepare response...
				nlohmann::json Answer;

				Answer["jsonrpc"] = "2.0";
				Answer["id"] = Id;
				Answer["result"]["serial"] = Serial;
				Answer["result"]["status"]["error"] = 0;
				Answer["result"]["status"]["when"] = When;
				Answer["result"]["status"]["text"] = "No errors were found";

				Version_++;
				SetFirmware(GetFirmware(URI));

				SendObject(Answer);
				Logger_.information(fmt::format("upgrade({}): from URI={}.", SerialNumber_, URI));
			} else {
				Logger_.warning(fmt::format("upgrade({}): Illegal command.", SerialNumber_));
			}
		} catch (const Poco::Exception &E) {
			Logger_.warning(
				fmt::format("upgrade({}): Exception. {}", SerialNumber_, E.displayText()));
		}
	}

	void OWLSclient::DoFactory(uint64_t Id, nlohmann::json &Params) {
		try {
			if (Params.contains("serial") && Params.contains("keep_redirector")) {

				uint64_t When = Params.contains("when") ? (uint64_t)Params["when"] : 0;
				auto Serial = to_string(Params["serial"]);
				auto KeepRedirector = Params["uri"];

				Version_ = 1;
				SetFirmware();
				KeepRedirector_ = KeepRedirector;

				nlohmann::json Answer;

				Answer["jsonrpc"] = "2.0";
				Answer["id"] = Id;
				Answer["result"]["serial"] = Serial;
				Answer["result"]["status"]["error"] = 0;
				Answer["result"]["status"]["when"] = When;
				Answer["result"]["status"]["text"] = "No errors were found";
				SendObject(Answer);

				Logger_.information(fmt::format("factory({}): done.", SerialNumber_));

				CurrentConfig_ = SimulationCoordinator()->GetSimConfiguration(Utils::Now());
				UpdateConfiguration();
			} else {
				Logger_.warning(fmt::format("factory({}): Illegal command.", SerialNumber_));
			}
		} catch (const Poco::Exception &E) {
			Logger_.warning(
				fmt::format("factory({}): Exception. {}", SerialNumber_, E.displayText()));
		}
	}

	void OWLSclient::DoLEDs(uint64_t Id, nlohmann::json &Params) {
		try {
			if (Params.contains("serial") && Params.contains("pattern")) {

				uint64_t When = Params.contains("when") ? (uint64_t)Params["when"] : 0;
				auto Serial = to_string(Params["serial"]);
				auto Pattern = to_string(Params["pattern"]);
				uint64_t Duration = Params.contains("duration") ? (uint64_t)Params["durarion"] : 10;

				//  prepare response...
				nlohmann::json Answer;

				Answer["jsonrpc"] = "2.0";
				Answer["id"] = Id;
				Answer["result"]["serial"] = Serial;
				Answer["result"]["status"]["error"] = 0;
				Answer["result"]["status"]["when"] = When;
				Answer["result"]["status"]["text"] = "No errors were found";
				SendObject(Answer);

				Logger_.information(fmt::format("LEDs({}): pattern set to: {} for {} ms.",
												SerialNumber_, Duration, Pattern));
			} else {
				Logger_.warning(fmt::format("LEDs({}): Illegal command.", SerialNumber_));
			}
		} catch (const Poco::Exception &E) {
			Logger_.warning(fmt::format("LEDs({}): Exception. {}", SerialNumber_, E.displayText()));
		}
	}

	void OWLSclient::DoPerform(uint64_t Id, nlohmann::json &Params) {
		try {
            DEBUG_LINE("start");
			if (Params.contains("serial") && Params.contains("command") &&
				Params.contains("payload")) {

				uint64_t When = Params.contains("when") ? (uint64_t)Params["when"] : 0;
				auto Serial = to_string(Params["serial"]);
				auto Command = to_string(Params["command"]);
				auto Payload = Params["payload"];

				//  prepare response...
				nlohmann::json Answer;

				Answer["jsonrpc"] = "2.0";
				Answer["id"] = Id;
				Answer["result"]["serial"] = Serial;
				Answer["result"]["status"]["error"] = 0;
				Answer["result"]["status"]["when"] = When;
				Answer["result"]["status"]["text"] = "No errors were found";
				Answer["result"]["status"]["text"]["resultCode"] = 0;
				Answer["result"]["status"]["text"]["resultText"] = "no return status";
				SendObject(Answer);
				Logger_.information(
					fmt::format("perform({}): command={}.", SerialNumber_, Command));
			} else {
				Logger_.warning(fmt::format("perform({}): Illegal command.", SerialNumber_));
			}
		} catch (const Poco::Exception &E) {
			Logger_.warning(
				fmt::format("perform({}): Exception. {}", SerialNumber_, E.displayText()));
        } catch (const std::exception &E) {
            DEBUG_LINE("exception2");
        }
	}

	void OWLSclient::DoTrace(uint64_t Id, nlohmann::json &Params) {
		try {
            DEBUG_LINE("start");
			if (Params.contains("serial") && Params.contains("duration") &&
				Params.contains("network") && Params.contains("interface") &&
				Params.contains("packets") && Params.contains("uri")) {

				uint64_t When = Params.contains("when") ? (uint64_t)Params["when"] : 0;
				auto Serial = to_string(Params["serial"]);
				auto Network = to_string(Params["network"]);
				auto Interface = to_string(Params["interface"]);
				uint64_t Duration = Params["duration"];
				uint64_t Packets = Params["packets"];
				auto URI = to_string(Params["uri"]);

				//  prepare response...
				nlohmann::json Answer;

				Answer["jsonrpc"] = "2.0";
				Answer["id"] = Id;
				Answer["result"]["serial"] = Serial;
				Answer["result"]["status"]["error"] = 0;
				Answer["result"]["status"]["when"] = When;
				Answer["result"]["status"]["text"] = "No errors were found";
				Answer["result"]["status"]["text"]["resultCode"] = 0;
				Answer["result"]["status"]["text"]["resultText"] = "no return status";
				SendObject(Answer);

				Logger_.information(
					fmt::format("trace({}): network={} interface={} packets={} duration={} URI={}.",
								SerialNumber_, Network, Interface, Packets, Duration, URI));
			} else {
				Logger_.warning(fmt::format("trace({}): Illegal command.", SerialNumber_));
			}
		} catch (const Poco::Exception &E) {
            DEBUG_LINE("exception1");
			Logger_.warning(
				fmt::format("trace({}): Exception. {}", SerialNumber_, E.displayText()));
        } catch (const std::exception &E) {
            DEBUG_LINE("exception2");
        }
	}

	bool OWLSclient::Send(const std::string &Cmd) {

		try {
			uint32_t BytesSent = WS_->sendFrame(Cmd.c_str(), Cmd.size());
			if (BytesSent == Cmd.size()) {
                DEBUG_LINE("sent");
				SimStats()->AddTX(Runner_->Id(),Cmd.size());
				SimStats()->AddOutMsg(Runner_->Id());
				return true;
			} else {
                DEBUG_LINE("fail to send");
				Logger_.warning(
					fmt::format("SEND({}): incomplete. Sent: {}", SerialNumber_, BytesSent));
			}
		} catch (const Poco::Exception &E) {
            DEBUG_LINE("exception1");
			Logger_.log(E);
        } catch (const std::exception &E) {
            DEBUG_LINE("exception2");
        }

		return false;
	}

	bool OWLSclient::SendWSPing() {
		try {
            DEBUG_LINE("start");
			WS_->sendFrame(
				"", 0, Poco::Net::WebSocket::FRAME_OP_PING | Poco::Net::WebSocket::FRAME_FLAG_FIN);
			return true;
		} catch (const Poco::Exception &E) {
            DEBUG_LINE("failed");
			Logger_.log(E);
        } catch (const std::exception &E) {
            DEBUG_LINE("exception2");
        }
		return false;
	}

	bool OWLSclient::SendObject(const nlohmann::json &O) {
		try {
			std::string M = to_string(O);
			uint32_t BytesSent = WS_->sendFrame(M.c_str(), M.size());
			if (BytesSent == M.size()) {
                DEBUG_LINE("sent");
				SimStats()->AddTX(Runner_->Id(),BytesSent);
				SimStats()->AddOutMsg(Runner_->Id());
				return true;
			} else {
                DEBUG_LINE("failed");
				Logger_.warning(
					fmt::format("SEND({}): incomplete send. Sent: {}", SerialNumber_, BytesSent));
			}
		} catch (const Poco::Exception &E) {
            DEBUG_LINE("exception1");
			Logger_.log(E);
        } catch (const std::exception &E) {
            DEBUG_LINE("exception2");
        }
		return false;
	}

    bool OWLSclient::SendObject(const Poco::JSON::Object &O) {
        try {
            std::ostringstream os;
            O.stringify(os);
            uint32_t BytesSent = WS_->sendFrame(os.str().c_str(), os.str().size());
            if (BytesSent == os.str().size()) {
                DEBUG_LINE("sent");
                SimStats()->AddTX(Runner_->Id(),BytesSent);
                SimStats()->AddOutMsg(Runner_->Id());
                return true;
            } else {
                DEBUG_LINE("failed");
                Logger_.warning(
                        fmt::format("SEND({}): incomplete send. Sent: {}", SerialNumber_, BytesSent));
            }
        } catch (const Poco::Exception &E) {
            DEBUG_LINE("exception1");
            Logger_.log(E);
        } catch (const std::exception &E) {
            DEBUG_LINE("exception2");
        }
        return false;
    }

} // namespace OpenWifi
