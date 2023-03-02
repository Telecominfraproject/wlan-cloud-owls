//
// Created by stephane bourque on 2021-03-12.
//
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sys/time.h>
#include <thread>

#include "Poco/NObserver.h"
#include "Poco/Net/Context.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Net/SSLException.h"
#include "Poco/URI.h"

#include "uCentralClient.h"

#include "framework/MicroServiceFuncs.h"

#include "SimStats.h"
#include "Simulation.h"
#include "fmt/format.h"
#include <nlohmann/json.hpp>

using namespace std::chrono_literals;

namespace OpenWifi {

	static std::string MakeMac(const char *S, int offset) {
		char b[256];

		int j = 0, i = 0;
		for (int k = 0; k < 6; ++k) {
			b[j++] = S[i++];
			b[j++] = S[i++];
			b[j++] = ':';
		}
		b[--j] = 0;
		b[--j] = '0' + offset;
		return b;
	}

	static std::string RandomMAC() {
		char b[64];
		snprintf(b, sizeof(b), "%02x:%02x:%02x:%02x:%02x:%02x", (int)MicroServiceRandom(255),
				 (int)MicroServiceRandom(255), (int)MicroServiceRandom(255),
				 (int)MicroServiceRandom(255), (int)MicroServiceRandom(255),
				 (int)MicroServiceRandom(255));
		return b;
	}

	static std::string RandomIPv4() {
		char b[64];
		snprintf(b, sizeof(b), "%d.%d.%d.%d", (int)MicroServiceRandom(255),
				 (int)MicroServiceRandom(255), (int)MicroServiceRandom(255),
				 (int)MicroServiceRandom(255));
		return b;
	}

	static std::string RandomIPv6() {
		char b[128];
		snprintf(b, sizeof(b), "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
				 (uint)MicroServiceRandom(0x0ffff), (uint)MicroServiceRandom(0x0ffff),
				 (uint)MicroServiceRandom(0x0ffff), (uint)MicroServiceRandom(0x0ffff),
				 (uint)MicroServiceRandom(0x0ffff), (uint)MicroServiceRandom(0x0ffff),
				 (uint)MicroServiceRandom(0x0ffff), (uint)MicroServiceRandom(0x0ffff));
		return b;
	}

	uCentralClient::uCentralClient(Poco::Net::SocketReactor &Reactor, std::string SerialNumber,
								   Poco::Logger &Logger)
		: Reactor_(Reactor), Logger_(Logger), SerialNumber_(std::move(SerialNumber)) {
		AllInterfaceNames_[ap_interface_types::upstream] = "up0v0";
		AllInterfaceNames_[ap_interface_types::downstream] = "down0v0";

		AllInterfaceRoles_[ap_interface_types::upstream] = "upstream";
		AllInterfaceRoles_[ap_interface_types::downstream] = "downstream";

		AllPortNames_[ap_interface_types::upstream] = "wan0";
		AllPortNames_[ap_interface_types::downstream] = "eth0";

		SetFirmware();
		Active_ = UUID_ = Utils::Now();
		srand(UUID_);
		mac_lan = MakeMac(SerialNumber_.c_str(), 0);
		CurrentConfig_ = SimulationCoordinator()->GetSimConfiguration(Utils::Now());
		UpdateConfiguration();
	}

	static int Find2GAutoChannel() { return 11; }

	static int Find5GAutoChannel() { return 36; }

	static int Find6GAutoChannel() { return 147; }

	template <typename T>
	void AssignIfPresent(const nlohmann::json &doc, const char *name, T &Value, T default_value) {
		if (doc.contains(name) && !doc[name].is_null())
			Value = doc[name];
		else
			Value = default_value;
	}

	bool uCentralClient::FindInterfaceRole(const std::string &role,
										   OpenWifi::ap_interface_types &interface) {
		for (const auto &[interface_type, interface_name] : AllInterfaceRoles_) {
			if (role == interface_name) {
				interface = interface_type;
				return true;
			}
		}
		return false;
	}

	void uCentralClient::Reset() {

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

	void uCentralClient::UpdateConfiguration() {
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
								AllAssociations_[std::make_tuple(current_interface_role, ssidName,
																 radio_bands::band_2g)] =
									CreateAssociations(Utils::SerialToMAC(Utils::IntToSerialNumber(
														   Utils::SerialNumberToInt(SerialNumber_) +
														   bssid_index++)),
													   SimulationCoordinator()
														   ->GetSimulationInfo()
														   .minAssociations,
													   SimulationCoordinator()
														   ->GetSimulationInfo()
														   .maxAssociations);
							}
							if (band == "5G") {
								AllAssociations_[std::make_tuple(current_interface_role, ssidName,
																 radio_bands::band_5g)] =
									CreateAssociations(Utils::SerialToMAC(Utils::IntToSerialNumber(
														   Utils::SerialNumberToInt(SerialNumber_) +
														   bssid_index++)),
													   SimulationCoordinator()
														   ->GetSimulationInfo()
														   .minAssociations,
													   SimulationCoordinator()
														   ->GetSimulationInfo()
														   .maxAssociations);
							}
							if (band == "6G") {
								AllAssociations_[std::make_tuple(current_interface_role, ssidName,
																 radio_bands::band_6g)] =
									CreateAssociations(Utils::SerialToMAC(Utils::IntToSerialNumber(
														   Utils::SerialNumberToInt(SerialNumber_) +
														   bssid_index++)),
													   SimulationCoordinator()
														   ->GetSimulationInfo()
														   .minAssociations,
													   SimulationCoordinator()
														   ->GetSimulationInfo()
														   .maxAssociations);
							}
						}
					}
					FakeCounters F;
					AllCounters_[current_interface_role] = F;
				}
			}
		}

		AllLanClients_ = CreateLanClients(SimulationCoordinator()->GetSimulationInfo().minClients,
										  SimulationCoordinator()->GetSimulationInfo().maxClients);

		auto radios = CurrentConfig_["radios"];
		uint index = 0;
		for (const auto &radio : radios) {
			auto band = radio["band"];
			FakeRadio R;
			radio_bands the_band{radio_bands::band_2g};
			std::uint64_t the_channel = Find2GAutoChannel();
			if (radio.contains("channel")) {
				if (radio["channel"].is_string() && radio["channel"] == "auto") {
					if (band == "2G")
						the_channel = Find2GAutoChannel();
					else if (band == "5G")
						the_channel = Find5GAutoChannel();
					else if (band == "6G")
						the_channel = Find6GAutoChannel();
				} else if (radio["channel"].is_number_integer()) {
					the_channel = radio["channel"];
				}
			}
			R.channel = the_channel;

			if (band == "5G") {
				the_band = radio_bands::band_5g;
			} else if (band == "6G") {
				the_band = radio_bands::band_6g;
			}
			AssignIfPresent(radio, "tx_power", R.tx_power, (uint_fast64_t)23);

			if (index == 0)
				R.phy = "platform/soc/c000000.wifi";
			else
				R.phy = "platform/soc/c000000.wifi+" + std::to_string(index);
			R.index = index;
			AllRadios_[the_band] = R;
			++index;
		}
	}

	FakeLanClients uCentralClient::CreateLanClients(uint64_t min, uint64_t max) {
		FakeLanClients Clients;
		uint64_t Num = MicroServiceRandom(min, max);
		for (uint64_t i = 0; i < Num; i++) {
			FakeLanClient CI;
			CI.mac = RandomMAC();
			CI.ipv4_addresses.push_back(RandomIPv4());
			CI.ipv6_addresses.push_back(RandomIPv6());
			CI.ports.push_back("eth0");
			Clients.push_back(CI);
		}
		return Clients;
	}

	FakeAssociations uCentralClient::CreateAssociations(const std::string &bssid, uint64_t min,
														uint64_t max) {
		FakeAssociations res;

		auto n = MicroServiceRandom(min, max);
		while (n) {
			FakeAssociation FA;

			FA.bssid = bssid;
			FA.station = RandomMAC();
			FA.ack_signal_avg = local_random(-40, -60);
			FA.ack_signal = FA.ack_signal_avg;
			FA.ipaddr_v4 = RandomIPv4();
			FA.ipaddr_v6 = RandomIPv6();
			FA.rssi = local_random(-40, -90);
			res.push_back(FA);
			--n;
		}
		return res;
	}

	nlohmann::json uCentralClient::CreateLinkState() {
		nlohmann::json res;
		for (const auto &[interface_type, _] : AllCounters_) {
			res[AllInterfaceRoles_[interface_type]][AllPortNames_[interface_type]]["carrier"] = 1;
			res[AllInterfaceRoles_[interface_type]][AllPortNames_[interface_type]]["duplex"] =
				"full";
			res[AllInterfaceRoles_[interface_type]][AllPortNames_[interface_type]]["speed"] = 1000;
		}
		return res;
	}

	nlohmann::json uCentralClient::CreateState() {
		nlohmann::json S;

		//  set the version
		S["version"] = 1;

		//  set the unit stuff
		auto now = Utils::Now();
		S["unit"]["load"] = std::vector<double>{(double)(MicroServiceRandom(75)) / 100.0,
												(double)(MicroServiceRandom(50)) / 100.0,
												(double)(MicroServiceRandom(25)) / 100.0};
		S["unit"]["localtime"] = now;
		S["unit"]["uptime"] = now - StartTime_;
		S["unit"]["memory"]["total"] = 973139968;
		S["unit"]["memory"]["buffered"] = 10129408;
		S["unit"]["memory"]["cached"] = 29233152;
		S["unit"]["memory"]["free"] = 760164352;

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

				auto state_ue_clients = nlohmann::json::array();
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
							ue["ports"].push_back(interface_type == upstream ? "eth0" : "eth1");
							// std::cout << "Adding association info" << to_string(ue) << std::endl;
							state_ue_clients.push_back(ue);
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
					for (const auto &ue_assoc : state_ue_clients) {
						state_lan_clients.push_back(ue_assoc);
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

	void uCentralClient::Disconnect(const char *Reason, bool Reconnect) {
		std::lock_guard G(Mutex_);
		Logger_.debug(fmt::format("DEVICE({}): disconnecting because '{}'", SerialNumber_,
								  std::string{Reason}));
		if (Connected_) {
			Reactor_.removeEventHandler(
				*WS_, Poco::NObserver<uCentralClient, Poco::Net::ReadableNotification>(
						  *this, &uCentralClient::OnSocketReadable));
			(*WS_).close();
		}

		Connected_ = false;
		Commands_.clear();

		if (Reconnect)
			AddEvent(ev_reconnect, SimulationCoordinator()->GetSimulationInfo().reconnectInterval +
									   MicroServiceRandom(15));

		SimStats()->Disconnect();
	}

	void uCentralClient::DoCensus(CensusReport &Census) {
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
			}
	}

	void uCentralClient::OnSocketReadable(
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

	void uCentralClient::ProcessCommand(nlohmann::json &Vars) {

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

	void uCentralClient::DoConfigure(uint64_t Id, nlohmann::json &Params) {
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

	void uCentralClient::DoReboot(uint64_t Id, nlohmann::json &Params) {
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
				Disconnect("Rebooting", true);
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

	void uCentralClient::DoUpgrade(uint64_t Id, nlohmann::json &Params) {
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
				Disconnect("Doing an upgrade", true);
			} else {
				Logger_.warning(fmt::format("upgrade({}): Illegal command.", SerialNumber_));
			}
		} catch (const Poco::Exception &E) {
			Logger_.warning(
				fmt::format("upgrade({}): Exception. {}", SerialNumber_, E.displayText()));
		}
	}

	void uCentralClient::DoFactory(uint64_t Id, nlohmann::json &Params) {
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

				Disconnect("Factory reset", true);
			} else {
				Logger_.warning(fmt::format("factory({}): Illegal command.", SerialNumber_));
			}
		} catch (const Poco::Exception &E) {
			Logger_.warning(
				fmt::format("factory({}): Exception. {}", SerialNumber_, E.displayText()));
		}
	}

	void uCentralClient::DoLEDs(uint64_t Id, nlohmann::json &Params) {
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

	void uCentralClient::DoPerform(uint64_t Id, nlohmann::json &Params) {
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

	void uCentralClient::DoTrace(uint64_t Id, nlohmann::json &Params) {
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

	void uCentralClient::EstablishConnection() {
		Poco::URI uri(SimulationCoordinator()->GetSimulationInfo().gateway);

		Poco::Net::Context::Params P;

		P.verificationMode = Poco::Net::Context::VERIFY_STRICT;
		P.verificationDepth = 9;
		P.caLocation = SimulationCoordinator()->GetCasLocation();
		P.loadDefaultCAs = false;
		P.certificateFile = SimulationCoordinator()->GetCertFileName();
		P.privateKeyFile = SimulationCoordinator()->GetKeyFileName();
		P.cipherList = "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH";
		P.dhUse2048Bits = true;

		auto Context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, P);
		Poco::Crypto::X509Certificate Cert(SimulationCoordinator()->GetCertFileName());
		Poco::Crypto::X509Certificate Root(SimulationCoordinator()->GetRootCAFileName());

		Context->useCertificate(Cert);
		Context->addChainCertificate(Root);

		Context->addCertificateAuthority(Root);

		if (SimulationCoordinator()->GetLevel() == Poco::Net::Context::VERIFY_STRICT) {
		}

		Poco::Crypto::RSAKey Key("", SimulationCoordinator()->GetKeyFileName(), "");
		Context->usePrivateKey(Key);

		SSL_CTX *SSLCtx = Context->sslContext();
		if (!SSL_CTX_check_private_key(SSLCtx)) {
			std::cout << "Wrong Certificate: " << SimulationCoordinator()->GetCertFileName()
					  << " for " << SimulationCoordinator()->GetKeyFileName() << std::endl;
		}

		if (SimulationCoordinator()->GetLevel() == Poco::Net::Context::VERIFY_STRICT) {
		}

		Poco::Net::HTTPSClientSession Session(uri.getHost(), uri.getPort(), Context);
		Poco::Net::HTTPRequest Request(Poco::Net::HTTPRequest::HTTP_GET, "/?encoding=text",
									   Poco::Net::HTTPMessage::HTTP_1_1);
		Request.set("origin", "http://www.websocket.org");
		Poco::Net::HTTPResponse Response;

		Logger_.information(fmt::format("connecting({}): host={} port={}", SerialNumber_,
										uri.getHost(), uri.getPort()));

		std::lock_guard guard(Mutex_);

		try {
			WS_ = std::make_unique<Poco::Net::WebSocket>(Session, Request, Response);
			(*WS_).setKeepAlive(true);
			(*WS_).setReceiveTimeout(Poco::Timespan());
			(*WS_).setSendTimeout(Poco::Timespan(20, 0));
			(*WS_).setNoDelay(true);
			Reactor_.addEventHandler(
				*WS_, Poco::NObserver<uCentralClient, Poco::Net::ReadableNotification>(
						  *this, &uCentralClient::OnSocketReadable));
			Connected_ = true;

			AddEvent(ev_connect, 1);
			SimStats()->Connect();
		} catch (const Poco::Exception &E) {
			Logger_.warning(
				fmt::format("connecting({}): exception. {}", SerialNumber_, E.displayText()));
			AddEvent(ev_reconnect, SimulationCoordinator()->GetSimulationInfo().reconnectInterval +
									   MicroServiceRandom(15));
		} catch (const std::exception &E) {
			Logger_.warning(
				fmt::format("connecting({}): std::exception. {}", SerialNumber_, E.what()));
			AddEvent(ev_reconnect, SimulationCoordinator()->GetSimulationInfo().reconnectInterval +
									   MicroServiceRandom(15));
		} catch (...) {
			Logger_.warning(fmt::format("connecting({}): unknown exception. {}", SerialNumber_));
			AddEvent(ev_reconnect, SimulationCoordinator()->GetSimulationInfo().reconnectInterval +
									   MicroServiceRandom(15));
		}
	}

	bool uCentralClient::Send(const std::string &Cmd) {
		std::lock_guard guard(Mutex_);

		try {
			uint32_t BytesSent = WS_->sendFrame(Cmd.c_str(), Cmd.size());
			if (BytesSent == Cmd.size()) {
				SimStats()->AddTX(Cmd.size());
				SimStats()->AddOutMsg();
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

	bool uCentralClient::SendWSPing() {
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

	bool uCentralClient::SendObject(nlohmann::json &O) {
		std::lock_guard guard(Mutex_);

		try {
			auto M = to_string(O);
			uint32_t BytesSent = WS_->sendFrame(M.c_str(), M.size());
			if (BytesSent == M.size()) {
				SimStats()->AddTX(BytesSent);
				SimStats()->AddOutMsg();
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

	static const uint64_t million = 1000000;

	void uCentralClient::AddEvent(uCentralEventType E, uint64_t InSeconds) {
		std::lock_guard guard(Mutex_);

		timeval curTime{0, 0};
		gettimeofday(&curTime, nullptr);
		uint64_t NextCommand = (InSeconds * million) + (curTime.tv_sec * million) + curTime.tv_usec;

		//  we need to make sure we do not possibly overwrite other commands at the same time
		while (Commands_.find(NextCommand) != Commands_.end())
			NextCommand++;

		Commands_[NextCommand] = E;
	}

	uCentralEventType uCentralClient::NextEvent(bool Remove) {
		std::lock_guard guard(Mutex_);

		if (Commands_.empty())
			return ev_none;

		auto EventTime = Commands_.begin()->first;
		timeval curTime{0, 0};
		gettimeofday(&curTime, nullptr);
		uint64_t Now = (curTime.tv_sec * million) + curTime.tv_usec;

		if (EventTime < Now) {
			uCentralEventType E = Commands_.begin()->second;
			if (Remove)
				Commands_.erase(Commands_.begin());
			return E;
		}

		return ev_none;
	}
} // namespace OpenWifi
