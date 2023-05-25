//
// Created by stephane bourque on 2021-03-12.
//
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <tuple>

#include "OWLS_utils.h"

#include "Poco/NObserver.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/URI.h"

#include "OWLSclient.h"
#include "OWLSdefinitions.h"
#include "framework/MicroServiceFuncs.h"

#include "SimStats.h"
#include "SimulationCoordinator.h"
#include "fmt/format.h"

using namespace std::chrono_literals;

namespace OpenWifi {

	OWLSclient::OWLSclient(std::string SerialNumber,
                           Poco::Logger &Logger, SimulationRunner *runner,
                           Poco::Net::SocketReactor &R)
		: Logger_(Logger), SerialNumber_(std::move(SerialNumber)),
          Memory_(1),
          Load_(1),
          Runner_(runner),
          Reactor_(R) {

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
		CurrentConfig_ = SimulationCoordinator()->GetSimConfigurationPtr(Utils::Now());
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
        Memory_.reset();
        Load_.reset();

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

        for(auto &lan_client:AllLanClients_) {
            lan_client.reset();
        }
	}

	void OWLSclient::UpdateConfiguration() {
		//  go through the config and harvest the SSID names, also update all the client stuff
		auto Interfaces = CurrentConfig_->getArray("interfaces");
		AllAssociations_.clear();
		AllLanClients_.clear();
		AllRadios_.clear();
		bssid_index = 1;
		for (uint interface_index=0;interface_index<Interfaces->size();interface_index++) {
            auto interfacePtr = Interfaces->get(interface_index);
            auto interface = interfacePtr.extract<Poco::JSON::Object::Ptr>();
            if (interface->has("role")) {
                ap_interface_types current_interface_role = upstream;
                if (FindInterfaceRole(interface->get("role"), current_interface_role)) {
                    auto SSIDs = interface->getArray("ssids");
                    for (uint ssid_index=0; ssid_index< SSIDs->size(); ++ssid_index) {
                        auto SSIDptr = SSIDs->get(ssid_index);
                        auto SSID = SSIDptr.extract<Poco::JSON::Object::Ptr>();
                        auto Bands = SSID->getArray("wifi-bands");
                        for(uint band_index=0;band_index<Bands->size(); band_index++) {
                            std::string band = Bands->get(band_index);
                            std::string ssidName = SSID->get("name");
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

		auto radios = CurrentConfig_->getArray("radios");
		for (uint radio_index=0;radio_index<radios->size();radio_index++) {
            auto radioPtr = radios->get(radio_index);
            auto radio = radioPtr.extract<Poco::JSON::Object::Ptr>();

			std::string band = radio->get("band");
			MockRadio R;

            R.band.push_back(band);
            if (band == "2G") {
                R.radioBands = radio_bands::band_2g;
            } else if (band == "5G") {
                R.radioBands = radio_bands::band_5g;
            } else if (band == "6G") {
                R.radioBands =  radio_bands::band_6g;
            }

            if(radio->has("channel-width")) {
                R.channel_width = radio->get("channel-width");
            } else {
                R.channel_width = 20;
            }

			if (!radio->has("channel")) {
                R.channel = OWLSutils::FindAutoChannel(R.radioBands,R.channel_width);
            } else {
                std::string channel = radio->get("channel").toString();
                if(OWLSutils::is_integer(channel)) {
                    R.channel = std::strtoll(channel.c_str(), nullptr,10);
                } else {
                    R.channel = OWLSutils::FindAutoChannel(R.radioBands, R.channel_width);
                }
			}

            OWLSutils::FillinFrequencies(R.channel, R.radioBands, R.channel_width, R.channels, R.frequency);

			OWLSutils::AssignIfPresent(radio, "tx_power", R.tx_power, (uint_fast64_t)23);

			if (radio_index == 0)
				R.phy = "platform/soc/c000000.wifi";
			else
				R.phy = "platform/soc/c000000.wifi+" + std::to_string(radio_index);
			R.index = radio_index;
			AllRadios_[R.radioBands] = R;
		}
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
        }
        return res;
    }

    Poco::JSON::Object OWLSclient::CreateStatePtr() {
        Poco::JSON::Object  State,Unit;

        auto now = Utils::Now();
        Memory_.to_json(Unit);
        Load_.to_json(Unit);
        Unit.set("localtime", now);
        Unit.set("uptime",  now - StartTime_);
        Unit.set("temperature", std::vector<std::int64_t> { OWLSutils::local_random(48,58), OWLSutils::local_random(48,58)});

        Poco::JSON::Array RadioArray;
        for (auto &[_, radio] : AllRadios_) {
            Poco::JSON::Object doc;
            radio.to_json(doc);
            RadioArray.add(doc);
        }

        Poco::JSON::Array all_interfaces;
        for (const auto &ap_interface_type :
                {ap_interface_types::upstream, ap_interface_types::downstream}) {
            if (AllCounters_.find(ap_interface_type) != AllCounters_.end()) {
                Poco::JSON::Object  current_interface;
                Poco::JSON::Array   ue_clients, up_ssids;
                uint64_t ssid_num = 0, interfaces = 0;

                for (auto &[interface, associations] : AllAssociations_) {
                    auto &[interface_type, ssid, band] = interface;
                    if (interface_type == ap_interface_type) {
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
                        Poco::JSON::Object ssid_info;
                        ssid_info.set("associations", association_list);
                        ssid_info.set("bssid", bssid);
                        ssid_info.set("band", OpenWifi::to_string(band));
                        Poco::JSON::Object  Counters;
                        AllCounters_[interface_type].to_json(Counters);
                        ssid_info.set("counters", Counters);
                        ssid_info.set("frequency", AllRadios_[band].frequency);
                        ssid_info.set("iface", AllPortNames_[interface_type]);
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
                current_interface.set("ssids", up_ssids);
                Poco::JSON::Object  C;
                AllCounters_[ap_interface_type].to_json(C);
                current_interface.set("counters", C);

                //  if we have 2 interfaces, then the clients go to the downstream interface
                //  if we only have 1 interface then this is bridged and therefore clients go on the
                //  upstream
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

    void OWLSclient::DoConfigure([[maybe_unused]] std::shared_ptr<OWLSclient> Client, uint64_t Id, const Poco::JSON::Object::Ptr Params) {
		try {
            std::lock_guard  G(Client->Mutex_);
			if (Params->has(uCentralProtocol::SERIAL) &&
                Params->has(uCentralProtocol::CONFIG) &&
                Params->has(uCentralProtocol::UUID)) {

				std::string     Serial = Params->get(uCentralProtocol::SERIAL);
                std::uint64_t   NewUUID = Params->get(uCentralProtocol::UUID);
				auto Configuration = Params->getObject("config");
                Client->UUID_ = Client->Active_ = NewUUID;
                Client->CurrentConfig_ = Configuration;

                if(Configuration->isObject("metrics")) {
                    auto Metrics = Configuration->getObject("metrics");
                    if(Metrics->isObject("health")) {
                        auto Health = Metrics->getObject("health");
                        Client->HealthInterval_ = Health->get("interval");
                    }
                    if(Metrics->isObject("statistics")) {
                        auto Statistics = Metrics->getObject("statistics");
                        Client->StatisticsInterval_ = Statistics->get("interval");
                    }
                }

				//  prepare response...
				Poco::JSON::Object Answer, Result, Status;
                Status.set(uCentralProtocol::ERROR, 0);
                Status.set(uCentralProtocol::TEXT, "Success");
                Result.set(uCentralProtocol::SERIAL, Serial);
                Result.set(uCentralProtocol::UUID, Client->UUID_);
                Result.set(uCentralProtocol::STATUS, Status);
                OWLSutils::MakeRPCHeader(Answer, Id, Result);
                poco_information(Client->Logger_,fmt::format("configure({}): done.", Client->SerialNumber_));
                // std::this_thread::sleep_for(std::chrono::seconds(OWLSutils::local_random(10,30)));
                Client->SendObject(Answer);
			} else {
                poco_warning(Client->Logger_,fmt::format("configure({}): Illegal command.", Client->SerialNumber_));
			}
		} catch (const Poco::Exception &E) {
            DEBUG_LINE("exception 1");
            poco_warning(Client->Logger_,
				fmt::format("configure({}): Exception. {}", Client->SerialNumber_, E.displayText()));
		} catch (const std::exception &E) {
            DEBUG_LINE("exception2");
        }
	}

    void OWLSclient::Disconnect([[
    maybe_unused]] std::lock_guard<std::mutex> &Guard) {
        if(Valid_) {
            Runner_->Report().ev_disconnect++;
            if (Connected_) {
                Runner_->RemoveClientFd(fd_);
                fd_ = -1;
                Reactor_.removeEventHandler(
                        *WS_, Poco::NObserver<SimulationRunner, Poco::Net::ReadableNotification>(
                                *Runner_, &SimulationRunner::OnSocketReadable));
                Reactor_.removeEventHandler(
                        *WS_, Poco::NObserver<SimulationRunner, Poco::Net::ErrorNotification>(
                                *Runner_, &SimulationRunner::OnSocketError));
                Reactor_.removeEventHandler(
                        *WS_, Poco::NObserver<SimulationRunner, Poco::Net::ShutdownNotification>(
                                *Runner_, &SimulationRunner::OnSocketShutdown));
                WS_->shutdown();
                (*WS_).close();
                Connected_ = false;
                std::cout << "Disconnecting a client: " << SerialNumber_ << std::endl;
            }else {
                std::cout << "Disconnecting an unconnected client: " << SerialNumber_ << std::endl;
            }
        } else {
            std::cout << "Disconnecting an invalid client: " << SerialNumber_ << std::endl;
        }
    }

	void OWLSclient::DoReboot(std::shared_ptr<OWLSclient> Client, uint64_t Id, const Poco::JSON::Object::Ptr Params) {
		try {
            std::lock_guard  G(Client->Mutex_);
            if (Params->has("serial") && Params->has("when")) {
                std::string Serial = Params->get("serial");

                Poco::JSON::Object Answer, Result, Status;
                Status.set(uCentralProtocol::ERROR, 0);
                Status.set(uCentralProtocol::TEXT, "Success");
                Result.set(uCentralProtocol::SERIAL, Serial);
                Result.set(uCentralProtocol::UUID, Client->UUID_);
                Result.set(uCentralProtocol::STATUS, Status);

                OWLSutils::MakeRPCHeader(Answer,Id,Result);
                poco_information(Client->Logger_,fmt::format("reboot({}): done.", Client->SerialNumber_));
                Client->SendObject(Answer);
                Client->Disconnect(G);
                Client->Reset();
                std::this_thread::sleep_for(std::chrono::seconds(20));
                OWLSclientEvents::Disconnect(Client, Client->Runner_, "Command: reboot", true);
			} else {
                Client->Logger_.warning(fmt::format("reboot({}): Illegal command.", Client->SerialNumber_));
			}
		} catch (const Poco::Exception &E) {
            Client->Logger_.warning(
				fmt::format("reboot({}): Exception. {}", Client->SerialNumber_, E.displayText()));
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

	void OWLSclient::DoUpgrade(std::shared_ptr<OWLSclient> Client, uint64_t Id, const Poco::JSON::Object::Ptr Params) {
		try {
            std::lock_guard  G(Client->Mutex_);
            if (Params->has("serial") && Params->has("uri")) {
                std::string Serial = Params->get("serial");
                std::string URI = Params->get("uri");

                Poco::JSON::Object Answer, Result, Status;
                Status.set(uCentralProtocol::ERROR, 0);
                Status.set(uCentralProtocol::TEXT, "Success");
                Result.set(uCentralProtocol::SERIAL, Serial);
                Result.set(uCentralProtocol::UUID, Client->UUID_);
                Result.set(uCentralProtocol::STATUS, Status);
                OWLSutils::MakeRPCHeader(Answer, Id, Result);
                poco_information(Client->Logger_,fmt::format("upgrade({}): from URI={}.", Client->SerialNumber_, URI));
                Client->SendObject(Answer);
                Client->Disconnect(G);
                Client->Version_++;
                Client->SetFirmware(GetFirmware(URI));
                std::this_thread::sleep_for(std::chrono::seconds(30));
                Client->Reset();
                OWLSclientEvents::Disconnect(Client, Client->Runner_, "Command: upgrade", true);
			} else {
                Client->Logger_.warning(fmt::format("upgrade({}): Illegal command.", Client->SerialNumber_));
			}
		} catch (const Poco::Exception &E) {
            Client->Logger_.warning(
				fmt::format("upgrade({}): Exception. {}", Client->SerialNumber_, E.displayText()));
		}
	}

	void OWLSclient::DoFactory(std::shared_ptr<OWLSclient> Client, uint64_t Id, const Poco::JSON::Object::Ptr Params) {
		try {
            std::lock_guard  G(Client->Mutex_);
            if (Params->has("serial") && Params->has("when")) {
                std::string Serial = Params->get("serial");

                Client->Version_ = 1;
                Client->SetFirmware();

                Poco::JSON::Object Answer, Result, Status;
                Status.set(uCentralProtocol::ERROR, 0);
                Status.set(uCentralProtocol::TEXT, "Success");
                Result.set(uCentralProtocol::SERIAL, Serial);
                Result.set(uCentralProtocol::UUID, Client->UUID_);
                Result.set(uCentralProtocol::STATUS, Status);
                OWLSutils::MakeRPCHeader(Answer, Id, Result);
                poco_information(Client->Logger_, fmt::format("factory({}): done.", Client->SerialNumber_));
                Client->SendObject(Answer);
                Client->Disconnect(G);
                Client->CurrentConfig_ = SimulationCoordinator()->GetSimConfigurationPtr(Utils::Now());
                Client->UpdateConfiguration();
                std::this_thread::sleep_for(std::chrono::seconds(5));
                Client->Reset();
                OWLSclientEvents::Disconnect(Client, Client->Runner_, "Command: upgrade", true);
			} else {
                Client->Logger_.warning(fmt::format("factory({}): Illegal command.", Client->SerialNumber_));
			}
		} catch (const Poco::Exception &E) {
            Client->Logger_.warning(
				fmt::format("factory({}): Exception. {}", Client->SerialNumber_, E.displayText()));
		}
	}

	void OWLSclient::DoLEDs([[maybe_unused]] std::shared_ptr<OWLSclient> Client, uint64_t Id, const Poco::JSON::Object::Ptr Params) {
		try {
            std::lock_guard  G(Client->Mutex_);
            if (Params->has("serial") && Params->has("pattern")) {
                std::string Serial = Params->get("serial");
                auto Pattern = Params->get("pattern").toString();
                uint64_t Duration = Params->has("when") ? (uint64_t)Params->get("durarion") : 10;

                Poco::JSON::Object Answer, Result, Status;
                Status.set(uCentralProtocol::ERROR, 0);
                Status.set(uCentralProtocol::TEXT, "Success");
                Result.set(uCentralProtocol::SERIAL, Serial);
                Result.set(uCentralProtocol::UUID, Client->UUID_);
                Result.set(uCentralProtocol::STATUS, Status);
                OWLSutils::MakeRPCHeader(Answer, Id, Result);
                poco_information(Client->Logger_,fmt::format("LEDs({}): pattern set to: {} for {} ms.",
                                                             Client->SerialNumber_, Duration, Pattern));
                Client->SendObject(Answer);
			} else {
                Client->Logger_.warning(fmt::format("LEDs({}): Illegal command.", Client->SerialNumber_));
			}
		} catch (const Poco::Exception &E) {
            Client->Logger_.warning(fmt::format("LEDs({}): Exception. {}", Client->SerialNumber_, E.displayText()));
		}
	}

    void OWLSclient::UNsupportedCommand(std::shared_ptr<OWLSclient> Client, uint64_t Id,
                                        const std::string & Method) {
        try {
            Poco::JSON::Object Answer, Result, Status;
            Status.set("error", 1);
            Status.set("text", "Command not supported");
            Result.set("serial", Client->SerialNumber_);
            Result.set("status", Status);
            OWLSutils::MakeRPCHeader(Answer, Id, Result);
            poco_information(Logger_,fmt::format("UNSUPPORTED({}): command {} not allowed for simulated devices.",
                                                 SerialNumber_, Method));
            SendObject(Answer);
        } catch(const Poco::Exception &E) {

        }
    }

	bool OWLSclient::Send(const std::string &Cmd) {

		try {
			uint32_t BytesSent = WS_->sendFrame(Cmd.c_str(), Cmd.size());
			if (BytesSent == Cmd.size()) {
				SimStats()->AddOutMsg(Runner_->Id(),Cmd.size());
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

    bool OWLSclient::SendObject(const Poco::JSON::Object &O) {
        try {
            std::ostringstream os;
            O.stringify(os);
            uint32_t BytesSent = WS_->sendFrame(os.str().c_str(), os.str().size());
            if (BytesSent == os.str().size()) {
                SimStats()->AddOutMsg(Runner_->Id(),BytesSent);
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
