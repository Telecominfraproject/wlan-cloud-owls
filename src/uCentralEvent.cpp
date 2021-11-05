//
// Created by stephane bourque on 2021-04-03.
//
#include <cstdlib>

#include "Poco/JSON/Object.h"
#include "Poco/JSON/Parser.h"
#include "Poco/JSON/Stringifier.h"
#include "Poco/zlib.h"

#include "framework/MicroService.h"
#include "uCentralEvent.h"
#include "Simulation.h"
#include "nlohmann/json.hpp"

namespace OpenWifi {
    static void SetHeader(Poco::JSON::Object & O, const char * Method)
    {
        O.set("jsonrpc","2.0");
        O.set("method",Method);
    }

    bool uCentralEvent::SendObject(Poco::JSON::Object &Obj) {
        try {
            std::stringstream OS;
            Poco::JSON::Stringifier::stringify(Obj, OS);
            return Client_->Send(OS.str());
        }
        catch (const Poco::Exception &E)
        {

        }
        return false;
    }

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
                return true;
            }
            Client_->Disconnect(true);
        }
        catch(...) {
            Client_->Disconnect(true);
        }
        return false;
    }

/*    bool ConnectEvent::Send_old() {
        try {
            Poco::JSON::Object O;

            SetHeader(O, "connect");

            Poco::JSON::Object Params;
            Params.set("serial", Client_->Serial());
            Params.set("uuid", Client_->UUID());
            Params.set("firmware", Client_->Firmware());

            Poco::JSON::Parser Parser;
            auto CapabilitiesObj = Parser.parse(SimulationCoordinator()->GetSimCapabilities()).extract<Poco::JSON::Object::Ptr>();
            auto CapabilitiesContent = CapabilitiesObj->getObject("capabilities");
            Params.set("capabilities", CapabilitiesContent);

            O.set("params", Params);
            if(SendObject(O)) {
                Client_->AddEvent(ev_state, SimulationCoordinator()->GetSimulationInfo().stateInterval);
                Client_->AddEvent(ev_healthcheck, SimulationCoordinator()->GetSimulationInfo().healthCheckInterval);
                Client_->AddEvent(ev_log, 120 + (rand() % 200));
                return true;
            }
            Client_->Disconnect(true);
        }
        catch(...) {
            Client_->Disconnect(true);
        }
        return false;
    }
*/

    bool StateEvent::Send() {
        try {
            Poco::JSON::Object O;

            SetHeader(O, "state");
            Poco::JSON::Object Params;

            Params.set("serial", Client_->Serial());
            Params.set("uuid", Client_->UUID());

            Poco::JSON::Parser Parser;

            std::string State{ SimulationCoordinator()->GetSimDefaultState(Client_->GetStartTime()) };
            auto StateObj = Parser.parse(State).extract<Poco::JSON::Object::Ptr>();
            Params.set("state", StateObj);

            if (State.size() > 3000) {
                // compress
                std::stringstream OS;
                Poco::JSON::Stringifier::stringify(Params, OS);

                unsigned long BufSize = OS.str().size() + 2000;
                std::vector<Bytef> Buffer(BufSize);

                compress(&Buffer[0], &BufSize, (Bytef *) OS.str().c_str(), OS.str().size());
                auto Compressed = OpenWifi::Utils::base64encode(&Buffer[0], BufSize);
                Poco::JSON::Object CompressedPayload;

                CompressedPayload.set("compress_64", Compressed);
                O.set("params", CompressedPayload);

            } else {
                O.set("params", Params);
            }

            if(SendObject(O)) {
                Client_->AddEvent(ev_state, SimulationCoordinator()->GetSimulationInfo().stateInterval);
                return true;
            }
            Client_->Disconnect(true);
        }
        catch(...) {
            Client_->Disconnect(true);
        }
        return false;
    }

    bool HealthCheckEvent::Send() {
        try {
            Poco::JSON::Object O;

            SetHeader(O, "healthcheck");
            Poco::JSON::Object Params;

            Params.set("serial", Client_->Serial());
            Params.set("uuid", Client_->UUID());
            Params.set("sanity", 100);

            Poco::JSON::Parser Parser;

            auto StateObj = Parser.parse(std::string("{}")).extract<Poco::JSON::Object::Ptr>();
            Params.set("data", StateObj);
            O.set("params", Params);

            if(SendObject(O))
                Client_->AddEvent(ev_healthcheck, SimulationCoordinator()->GetSimulationInfo().healthCheckInterval);

            return SendObject(O);
        }
        catch(...) {
            Client_->Disconnect(true);
        }
        return false;
    }

    bool LogEvent::Send() {
        try {
            Poco::JSON::Object O;

            SetHeader(O, "log");

            Poco::JSON::Object Params;
            Params.set("serial", Client_->Serial());
            Params.set("severity", Severity_);
            Params.set("log", LogLine_);

            O.set("params", Params);
            Client_->AddEvent(ev_log, 120 + (rand() % 200));

            if(SendObject(O)) {
                Client_->AddEvent(ev_log, 120 + (rand() % 200));
                return true;
            }
            Client_->Disconnect(true);
        }
        catch(...) {
            Client_->Disconnect(true);
        }
        return false;
    };

    bool CrashLogEvent::Send() {
        Poco::JSON::Object  O;

        SetHeader(O,"crashlog");
        return false;
    };

    bool ConfigChangePendingEvent::Send() {
        Poco::JSON::Object  O;

        try {
            SetHeader(O, "cfgpending");
            Poco::JSON::Object Params;

            Params.set("serial", Client_->Serial());
            Params.set("uuid", Client_->UUID());
            Params.set("active", Client_->Active());

            O.set("params", Params);

            if(SendObject(O)) {
                Client_->AddEvent(ev_configpendingchange, SimulationCoordinator()->GetSimulationInfo().clientInterval );
                return true;
            }
        }
        catch (...)
        {

        }
        Client_->Disconnect(true);
        return false;
    }

    bool KeepAliveEvent::Send() {
        try {
            Poco::JSON::Object O;

            SetHeader(O, "ping");
            Poco::JSON::Object Params;

            Params.set("serial", Client_->Serial());
            Params.set("uuid", Client_->UUID());

            O.set("params", Params);

            if(SendObject(O)) {
                Client_->AddEvent(ev_keepalive, SimulationCoordinator()->GetSimulationInfo().keepAlive);
                return true;
            }
        }
        catch(...) {
        }
        Client_->Disconnect(true);
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
            if (Client_->SendWSPing())
                Client_->AddEvent(ev_wsping, 60 * 5);
        }
        catch(...) {

        }
        Client_->Disconnect(true);
        return false;
    }
}