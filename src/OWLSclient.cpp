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

#include "framework/MicroServiceFuncs.h"

#include "SimStats.h"
#include "SimulationCoordinator.h"
#include "fmt/format.h"
#include <nlohmann/json.hpp>
#include "OWLSscheduler.h"

using namespace std::chrono_literals;

namespace OpenWifi {

	OWLSclient::OWLSclient(Poco::Net::SocketReactor &Reactor, std::string SerialNumber,
                           Poco::Logger &Logger, SimulationRunner *runner)
		: Reactor_(Reactor), Logger_(Logger), SerialNumber_(std::move(SerialNumber)),
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
            bool inserted;
            std::pair<associations_map_t::iterator,bool> insertion_res(interface_hint,inserted);
            insertion_res = AllAssociations_.insert(std::make_pair(interface,MockAssociations {}));
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
							if (band == "2G") {
									CreateAssociations(std::make_tuple(current_interface_role, ssidName,
                                                                       radio_bands::band_2g), Utils::SerialToMAC(Utils::IntToSerialNumber(
														   Utils::SerialNumberToInt(SerialNumber_) +
														   bssid_index++)),
                                                       Runner_->Details().minAssociations,
                                                       Runner_->Details().maxAssociations);
							}
							if (band == "5G") {
									CreateAssociations(std::make_tuple(current_interface_role, ssidName,
                                                                       radio_bands::band_5g), Utils::SerialToMAC(Utils::IntToSerialNumber(
														   Utils::SerialNumberToInt(SerialNumber_) +
														   bssid_index++)),
                                                       Runner_->Details().minAssociations,
                                                       Runner_->Details().maxAssociations);
							}
							if (band == "6G") {
									CreateAssociations(std::make_tuple(current_interface_role, ssidName,
                                                                       radio_bands::band_6g),
                                                       Utils::SerialToMAC(Utils::IntToSerialNumber(
														   Utils::SerialNumberToInt(SerialNumber_) +
														   bssid_index++)),
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

	nlohmann::json OWLSclient::CreateState() {
		nlohmann::json S;

		//  set the version
		S["version"] = 1;

		//  set the unit stuff
		auto now = Utils::Now();
		S["unit"] += Memory_.to_json();
        S["unit"] += Load_.to_json();
		S["unit"]["localtime"] = now;
		S["unit"]["uptime"] = now - StartTime_;
        S["unit"]["temperature"] = std::vector<std::int64_t> { OWLSutils::local_random(48,58), OWLSutils::local_random(48,58)};

		//  get all the radios out
		for (auto &[_, radio] : AllRadios_) {
			radio.next();
			S["radios"].push_back(radio.to_json());
		}

		//  set the link state
		S["link-state"] = CreateLinkState();

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
							association_list.push_back(association.to_json());
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
				current_interface["counters"] = AllCounters_[ap_interface_type].to_json();

				//  if we have 2 interfaces, then the clients go to the downstream interface
				//  if we only have 1 interface then this is bridged and therefore clients go on the
				//  upstream
				if ((AllCounters_.size() == 1 &&
					 ap_interface_type == ap_interface_types::upstream) ||
					(AllCounters_.size() == 2 &&
					 ap_interface_type == ap_interface_types::downstream)) {
					nlohmann::json state_lan_clients;
					for (const auto &lan_client : AllLanClients_) {
						state_lan_clients.push_back(lan_client.to_json());
					}
					// std::cout << "Adding " << state_ue_clients.size() << " UE clients" <<
					// std::endl;
					for (const auto &ue_client : ue_clients) {
						state_lan_clients.push_back(ue_client);
					}
					current_interface["clients"] = state_lan_clients;
				}
				current_interface["name"] = AllInterfaceNames_[ap_interface_type];
				all_interfaces.push_back(current_interface);
			}
		}
		S["interfaces"] = all_interfaces;
		//        std::cout << S << std::endl;
		//        std::cout << std::endl << std::endl << std::endl;

		return S;
	}

/*
	void OWLSclient::Disconnect(const char *Reason, bool Reconnect) {
		std::lock_guard G(Mutex_);
		Logger_.debug(fmt::format("DEVICE({}): disconnecting because '{}'", SerialNumber_,
								  std::string{Reason}));
		if (Connected_) {
			Reactor_.removeEventHandler(
				*WS_, Poco::NObserver<OWLSclient, Poco::Net::ReadableNotification>(
						  *this, &OWLSclient::OnSocketReadable));
			(*WS_).close();
		}

		Connected_ = false;
		Commands_.clear();

		if (Reconnect)
            OWLSscheduler()->Ref().in(std::chrono::seconds(OWLSutils::local_random(3,15)), OWLSclientEvents::Reconnect, Client );
			AddEvent(ev_reconnect, SimulationCoordinator()->GetSimulationInfo().reconnectInterval +
									   MicroServiceRandom(15));

		SimStats()->Disconnect();
	}

	void OWLSclient::DoCensus(CensusReport &Census) {
		std::lock_guard G(Mutex_);

		for (const auto i : Commands_)
			switch (i.second) {
			case ev_none:
				Census.ev_none++;
				break;
			case ev_reconnect:
				Census.ev_reconnect++;
				break;
			case ev_connect:
				Census.ev_connect++;
				break;
			case ev_state:
				Census.ev_state++;
				break;
			case ev_healthcheck:
				Census.ev_healthcheck++;
				break;
			case ev_log:
				Census.ev_log++;
				break;
			case ev_crashlog:
				Census.ev_crashlog++;
				break;
			case ev_configpendingchange:
				Census.ev_configpendingchange++;
				break;
			case ev_keepalive:
				Census.ev_keepalive++;
				break;
			case ev_reboot:
				Census.ev_reboot++;
				break;
			case ev_disconnect:
				Census.ev_disconnect++;
				break;
			case ev_wsping:
				Census.ev_wsping++;
				break;
            case ev_update:
                Census.ev_update++;
                    break;
			}
	}

	void OWLSclient::OnSocketReadable(
		[[maybe_unused]] const Poco::AutoPtr<Poco::Net::ReadableNotification> &pNf) {
		std::lock_guard G(Mutex_);

		try {
			char Message[16000];
			int Flags;

			auto MessageSize = WS_->receiveFrame(Message, sizeof(Message), Flags);
			auto Op = Flags & Poco::Net::WebSocket::FRAME_OP_BITMASK;

			if (MessageSize == 0 && Flags == 0 && Op == 0) {
				Disconnect("Error while waiting for data in WebSocket", true);
				return;
			}

			Message[MessageSize] = 0;
			switch (Op) {
			case Poco::Net::WebSocket::FRAME_OP_PING: {
				WS_->sendFrame("", 0,
							   Poco::Net::WebSocket::FRAME_OP_PONG |
								   Poco::Net::WebSocket::FRAME_FLAG_FIN);
			} break;

			case Poco::Net::WebSocket::FRAME_OP_PONG: {
			} break;

			case Poco::Net::WebSocket::FRAME_OP_TEXT: {
				if (MessageSize > 0) {
					SimStats()->AddRX(MessageSize);
					SimStats()->AddInMsg();
					auto Vars = nlohmann::json::parse(Message);

					if (Vars.contains("jsonrpc") && Vars.contains("id") &&
						Vars.contains("method") && Vars.contains("params")) {
						ProcessCommand(Vars);
					} else {
						Logger_.warning(
							fmt::format("MESSAGE({}): invalid incoming message.", SerialNumber_));
					}
				}
			} break;
			default: {
			} break;
			}
			return;
		} catch (const Poco::Net::SSLException &E) {
			Logger_.warning(
				fmt::format("Exception({}): SSL exception: {}", SerialNumber_, E.displayText()));
		} catch (const Poco::Exception &E) {
			Logger_.warning(fmt::format("Exception({}): Generic exception: {}", SerialNumber_,
										E.displayText()));
		}
		Disconnect("Exception caught during data reception", true);
	}

	void OWLSclient::ProcessCommand(nlohmann::json &Vars) {

		std::string Method = Vars["method"];

		auto Id = Vars["id"];
		auto Params = Vars["params"];

		if (Method == "configure") {
			DoConfigure(Id, Params);
		} else if (Method == "reboot") {
			DoReboot(Id, Params);
		} else if (Method == "upgrade") {
			DoUpgrade(Id, Params);
		} else if (Method == "factory") {
			DoFactory(Id, Params);
		} else if (Method == "leds") {
			DoLEDs(Id, Params);
		} else if (Method == "perform") {
			DoPerform(Id, Params);
		} else if (Method == "trace") {
			DoTrace(Id, Params);
		} else {
			Logger_.warning(fmt::format("COMMAND({}): unknown method '{}'", SerialNumber_, Method));
		}
	}
*/
	void OWLSclient::DoConfigure(uint64_t Id, nlohmann::json &Params) {
		std::lock_guard G(Mutex_);

		try {
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

				Logger_.information(fmt::format("configure({}): done.", SerialNumber_));
				SendObject(Answer);
			} else {
				Logger_.warning(fmt::format("configure({}): Illegal command.", SerialNumber_));
			}
		} catch (const Poco::Exception &E) {
			Logger_.warning(
				fmt::format("configure({}): Exception. {}", SerialNumber_, E.displayText()));
		}
	}

	void OWLSclient::DoReboot(uint64_t Id, nlohmann::json &Params) {
		std::lock_guard G(Mutex_);
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
		std::lock_guard G(Mutex_);
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
		std::lock_guard G(Mutex_);
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
		std::lock_guard G(Mutex_);
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
		std::lock_guard G(Mutex_);
		try {
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
		}
	}

	void OWLSclient::DoTrace(uint64_t Id, nlohmann::json &Params) {
		std::lock_guard G(Mutex_);
		try {
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
			Logger_.warning(
				fmt::format("trace({}): Exception. {}", SerialNumber_, E.displayText()));
		}
	}

	bool OWLSclient::Send(const std::string &Cmd) {
		std::lock_guard guard(Mutex_);

		try {
			uint32_t BytesSent = WS_->sendFrame(Cmd.c_str(), Cmd.size());
			if (BytesSent == Cmd.size()) {
				SimStats()->AddTX(Runner_->Id(),Cmd.size());
				SimStats()->AddOutMsg(Runner_->Id());
				return true;
			} else {
				Logger_.warning(
					fmt::format("SEND({}): incomplete. Sent: {}", SerialNumber_, BytesSent));
			}
		} catch (const Poco::Exception &E) {
			Logger_.log(E);
		}
		return false;
	}

	bool OWLSclient::SendWSPing() {
		std::lock_guard guard(Mutex_);

		try {
			WS_->sendFrame(
				"", 0, Poco::Net::WebSocket::FRAME_OP_PING | Poco::Net::WebSocket::FRAME_FLAG_FIN);
			return true;
		} catch (const Poco::Exception &E) {
			Logger_.log(E);
		}
		return false;
	}

	bool OWLSclient::SendObject(nlohmann::json &O) {
		std::lock_guard guard(Mutex_);

		try {
			auto M = to_string(O);
			uint32_t BytesSent = WS_->sendFrame(M.c_str(), M.size());
			if (BytesSent == M.size()) {
				SimStats()->AddTX(Runner_->Id(),BytesSent);
				SimStats()->AddOutMsg(Runner_->Id());
				return true;
			} else {
				Logger_.warning(
					fmt::format("SEND({}): incomplete send. Sent: {}", SerialNumber_, BytesSent));
			}
		} catch (const Poco::Exception &E) {
			Logger_.log(E);
		}
		return false;
	}

/*	static const uint64_t million = 1000000;

	void OWLSclient::AddEvent(OWLSeventType E, uint64_t InSeconds) {
		std::lock_guard guard(Mutex_);

		timeval curTime{0, 0};
		gettimeofday(&curTime, nullptr);
		uint64_t NextCommand = (InSeconds * million) + (curTime.tv_sec * million) + curTime.tv_usec;

		//  we need to make sure we do not possibly overwrite other commands at the same time
		while (Commands_.find(NextCommand) != Commands_.end())
			NextCommand++;

		Commands_[NextCommand] = E;
	}

	OWLSeventType OWLSclient::NextEvent(bool Remove) {
		std::lock_guard guard(Mutex_);

		if (Commands_.empty())
			return ev_none;

		auto EventTime = Commands_.begin()->first;
		timeval curTime{0, 0};
		gettimeofday(&curTime, nullptr);
		uint64_t Now = (curTime.tv_sec * million) + curTime.tv_usec;

		if (EventTime < Now) {
			OWLSeventType E = Commands_.begin()->second;
			if (Remove)
				Commands_.erase(Commands_.begin());
			return E;
		}

		return ev_none;
	}
 */
} // namespace OpenWifi
