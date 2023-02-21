//
// Created by stephane bourque on 2021-04-03.
//
#include <cstdlib>

#include "Poco/zlib.h"
#include "nlohmann/json.hpp"

#include "Simulation.h"
#include "uCentralEvent.h"

#include "framework/MicroServiceFuncs.h"

namespace OpenWifi {

	bool ConnectEvent::Send() {
		try {
			nlohmann::json M;
			M["jsonrpc"] = "2.0";
			M["method"] = "connect";
			M["params"]["serial"] = Client_->Serial();
			M["params"]["uuid"] = Client_->UUID();
			M["params"]["firmware"] = Client_->Firmware();
			auto TmpCapabilities = SimulationCoordinator()->GetSimCapabilities();
			auto LabelMac = Utils::SerialNumberToInt(Client_->Serial());
			auto LabelMacFormatted = Utils::SerialToMAC(Utils::IntToSerialNumber(LabelMac));
			auto LabelLanMacFormatted = Utils::SerialToMAC(Utils::IntToSerialNumber(LabelMac + 1));
			TmpCapabilities["label_macaddr"] = LabelMac;
			TmpCapabilities["macaddr"]["wan"] = LabelMac;
			TmpCapabilities["macaddr"]["lan"] = LabelLanMacFormatted;
			M["params"]["capabilities"] = TmpCapabilities;
			if (Client_->Send(to_string(M))) {
				Client_->Reset();
				Client_->AddEvent(ev_state,
								  SimulationCoordinator()->GetSimulationInfo().stateInterval);
				Client_->AddEvent(ev_healthcheck,
								  SimulationCoordinator()->GetSimulationInfo().healthCheckInterval);
				Client_->AddEvent(ev_log, MicroServiceRandom(120, 200));
				Client_->AddEvent(ev_wsping, 60 * 5);
				return true;
			}
		} catch (const Poco::Exception &E) {
			Client_->Logger().log(E);
		}
		Client_->Disconnect("Error occurred during connection", true);
		return false;
	}

	bool StateEvent::Send() {
		try {
			nlohmann::json M;

			M["jsonrpc"] = "2.0";
			M["method"] = "state";

			nlohmann::json ParamsObj;
			ParamsObj["serial"] = Client_->Serial();
			ParamsObj["uuid"] = Client_->UUID();
			ParamsObj["state"] = Client_->CreateState();

			auto ParamsStr = to_string(ParamsObj);

			unsigned long BufSize = ParamsStr.size() + 4000;
			std::vector<Bytef> Buffer(BufSize);
			compress(&Buffer[0], &BufSize, (Bytef *)ParamsStr.c_str(), ParamsStr.size());

			auto CompressedBase64Encoded = OpenWifi::Utils::base64encode(&Buffer[0], BufSize);

			M["params"]["compress_64"] = CompressedBase64Encoded;
			M["params"]["compress_sz"] = ParamsStr.size();

			if (Client_->Send(to_string(M))) {
				Client_->AddEvent(ev_state, Client_->GetStateInterval());
				return true;
			}
		} catch (const Poco::Exception &E) {
			Client_->Logger().log(E);
		}
		Client_->Disconnect("Error sending stats event", true);
		return false;
	}

	bool HealthCheckEvent::Send() {
		try {
			nlohmann::json M, P;

			P["memory"] = 23;

			M["jsonrpc"] = "2.0";
			M["method"] = "healthcheck";
			M["params"]["serial"] = Client_->Serial();
			M["params"]["uuid"] = Client_->UUID();
			M["params"]["sanity"] = 100;
			M["params"]["data"] = P;

			if (Client_->Send(to_string(M))) {
				Client_->AddEvent(ev_healthcheck, Client_->GetHealthInterval());
				return true;
			}
		} catch (const Poco::Exception &E) {
			Client_->Logger().log(E);
		}
		Client_->Disconnect("Error while sending HealthCheck", true);
		return false;
	}

	bool LogEvent::Send() {
		try {
			nlohmann::json M;

			M["jsonrpc"] = "2.0";
			M["method"] = "log";
			M["params"]["serial"] = Client_->Serial();
			M["params"]["uuid"] = Client_->UUID();
			M["params"]["severity"] = Severity_;
			M["params"]["log"] = LogLine_;

			if (Client_->Send(to_string(M))) {
				Client_->AddEvent(ev_log, MicroServiceRandom(300, 600));
				return true;
			}
		} catch (const Poco::Exception &E) {
			Client_->Logger().log(E);
		}
		Client_->Disconnect("Error while sending a Log event", true);
		return false;
	};

	bool CrashLogEvent::Send() { return false; };

	bool ConfigChangePendingEvent::Send() {
		try {
			nlohmann::json M;

			M["jsonrpc"] = "2.0";
			M["method"] = "cfgpending";
			M["params"]["serial"] = Client_->Serial();
			M["params"]["uuid"] = Client_->UUID();
			M["params"]["active"] = Client_->Active();

			if (Client_->Send(to_string(M))) {
				Client_->AddEvent(ev_configpendingchange,
								  SimulationCoordinator()->GetSimulationInfo().clientInterval);
				return true;
			}
		} catch (const Poco::Exception &E) {
			Client_->Logger().log(E);
		}
		Client_->Disconnect("Error while sending ConfigPendingEvent", true);
		return false;
	}

	bool KeepAliveEvent::Send() {
		try {
			nlohmann::json M;

			M["jsonrpc"] = "2.0";
			M["method"] = "ping";
			M["params"]["serial"] = Client_->Serial();
			M["params"]["uuid"] = Client_->UUID();

			if (Client_->Send(to_string(M))) {
				Client_->AddEvent(ev_keepalive,
								  SimulationCoordinator()->GetSimulationInfo().keepAlive);
				return true;
			}
		} catch (const Poco::Exception &E) {
			Client_->Logger().log(E);
		}
		Client_->Disconnect("Error while sending keepalive", true);
		return false;
	};

	// This is just a fake event, reboot is handled somewhere else.
	bool RebootEvent::Send() { return true; }

	// This is just a fake event, disconnect is handled somewhere else.
	bool DisconnectEvent::Send() { return true; }

	bool WSPingEvent::Send() {
		try {
			if (Client_->SendWSPing()) {
				Client_->AddEvent(ev_wsping, 60 * 5);
				return true;
			}
		} catch (const Poco::Exception &E) {
			Client_->Logger().log(E);
		}
		Client_->Disconnect("Error in WSPing", true);
		return false;
	}
} // namespace OpenWifi