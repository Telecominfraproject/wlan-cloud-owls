//
// Created by stephane bourque on 2021-04-03.
//
#include <cstdlib>

#include "Poco/zlib.h"
#include "nlohmann/json.hpp"

#include "framework/MicroService.h"

#include "uCentralEvent.h"
#include "Simulation.h"

namespace OpenWifi {

    bool ConnectEvent::Send() {
        try {
            nlohmann::json  M;
            M["jsonrpc"] = "2.0";
            M["method"] = "connect";
            M["params"]["serial"] = Client_->Serial();
            M["params"]["uuid"] = Client_->UUID();
            M["params"]["firmware"] = Client_->Firmware();
            M["params"]["capabilities"] = SimulationCoordinator()->GetSimCapabilities();
            if(Client_->Send(to_string(M))) {
                Client_->AddEvent(ev_state, SimulationCoordinator()->GetSimulationInfo().stateInterval);
                Client_->AddEvent(ev_healthcheck, SimulationCoordinator()->GetSimulationInfo().healthCheckInterval);
                Client_->AddEvent(ev_log, 120 + (rand() % 200));
                Client_->AddEvent(ev_wsping, 60 * 5);
                return true;
            }
        }
        catch(const Poco::Exception &E) {
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

            auto State = Client_->CreateState();

            nlohmann::json CPayload;

            CPayload["serial"] = Client_->Serial();
            CPayload["uuid"] = Client_->UUID();
            CPayload["state"] = State;

            auto StateStr = to_string(CPayload);

            unsigned long BufSize = StateStr.size() + 4000;
            std::vector<Bytef> Buffer(BufSize);
            compress(&Buffer[0], &BufSize, (Bytef *) StateStr.c_str(), StateStr.size());
            auto Compressed = OpenWifi::Utils::base64encode(&Buffer[0], BufSize);

            M["params"]["compress_64"] = Compressed;

            if(Client_->Send(to_string(M))) {
                Client_->AddEvent(ev_state, SimulationCoordinator()->GetSimulationInfo().stateInterval);
                return true;
            }
        }
        catch(const Poco::Exception &E) {
            Client_->Logger().log(E);
        }
        Client_->Disconnect("Error sending stats event", true);
        return false;
    }

    bool HealthCheckEvent::Send() {
        try {
            nlohmann::json  M,P;

            P["memory"] = 23;

            M["jsonrpc"] = "2.0";
            M["method"] = "healthcheck";
            M["params"]["serial"] = Client_->Serial();
            M["params"]["uuid"] = Client_->UUID();
            M["params"]["sanity"] = 100;
            M["params"]["data"] = P;

            if(Client_->Send(to_string(M))) {
                Client_->AddEvent(ev_healthcheck, SimulationCoordinator()->GetSimulationInfo().healthCheckInterval);
                return true;
            }
        }
        catch(const Poco::Exception &E) {
            Client_->Logger().log(E);
        }
        Client_->Disconnect("Error whiel sending HealthCheck", true);
        return false;
    }

    bool LogEvent::Send() {
        try {
            nlohmann::json  M;

            M["jsonrpc"] = "2.0";
            M["method"] = "log";
            M["params"]["serial"] = Client_->Serial();
            M["params"]["uuid"] = Client_->UUID();
            M["params"]["severity"] = Severity_;
            M["params"]["log"] = LogLine_;

            if(Client_->Send(to_string(M))) {
                Client_->AddEvent(ev_log, 120 + (rand() % 200));
                return true;
            }
        }
        catch(const Poco::Exception &E) {
            Client_->Logger().log(E);
        }
        Client_->Disconnect("Error while sending a Log event", true);
        return false;
    };

    bool CrashLogEvent::Send() {
        return false;
    };

    bool ConfigChangePendingEvent::Send() {
        try {
            nlohmann::json  M;

            M["jsonrpc"] = "2.0";
            M["method"] = "cfgpending";
            M["params"]["serial"] = Client_->Serial();
            M["params"]["uuid"] = Client_->UUID();
            M["params"]["active"] = Client_->Active();

            if(Client_->Send(to_string(M))) {
                Client_->AddEvent(ev_configpendingchange, SimulationCoordinator()->GetSimulationInfo().clientInterval );
                return true;
            }
        }
        catch(const Poco::Exception &E) {
            Client_->Logger().log(E);
        }
        Client_->Disconnect("Error while sending CongifPendingEvent", true);
        return false;
    }

    bool KeepAliveEvent::Send() {
        try {
            nlohmann::json  M;

            M["jsonrpc"] = "2.0";
            M["method"] = "ping";
            M["params"]["serial"] = Client_->Serial();
            M["params"]["uuid"] = Client_->UUID();

            if(Client_->Send(to_string(M))) {
                Client_->AddEvent(ev_keepalive, SimulationCoordinator()->GetSimulationInfo().keepAlive);
                return true;
            }
        }
        catch(const Poco::Exception &E) {
            Client_->Logger().log(E);
        }
        Client_->Disconnect("Error while sending a keepalive", true);
        return false;
    };

    // This is just a fake event, reboot is handled somewhere else.
    bool RebootEvent::Send() {
        return true;
    }

    // This is just a fake event, disconnect is handled somewhere else.
    bool DisconnectEvent::Send() {

        return true;
    }

    bool WSPingEvent::Send() {
        try {
            if (Client_->SendWSPing()) {
                Client_->AddEvent(ev_wsping, 60 * 5);
                return true;
            }
        }
        catch(const Poco::Exception &E) {
            Client_->Logger().log(E);
        }
        Client_->Disconnect("Error in WSPing", true);
        return false;
    }
}