//
// Created by stephane bourque on 2023-04-12.
//
#include "OWLSclient.h"
#include "SimulationRunner.h"
#include "SimulationCoordinator.h"
#include <fmt/format.h>
#include "SimStats.h"
#include <Poco/NObserver.h>

#include "OWLSclientEvents.h"

namespace OpenWifi::OWLSclientEvents {

    void HealthCheck(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner) {
        std::lock_guard G(Client->Mutex_);

        DEBUG_LINE("start");
        if(Client->Valid_ && Client->Connected_) {
            Runner->Report().ev_healthcheck++;
            try {
/*                nlohmann::json M, P;
                P["memory"] = 23;
                M["jsonrpc"] = "2.0";
                M["method"] = "healthcheck";
                M["params"]["serial"] = Client->Serial();
                M["params"]["uuid"] = Client->UUID();
                M["params"]["sanity"] = 100;
                M["params"]["data"] = P;
*/
                Poco::JSON::Object  Message, Params, Data, Memory;
                Memory.set("memory", 23);
                Data.set("data", Memory);
                Params.set("data", Data);
                Params.set("serial", Client->SerialNumber_);
                Params.set("uuid", Client->UUID_);
                Params.set("sanity", 100);
                OWLSutils::MakeHeader(Message,"healthcheck",Params);

                if (Client->SendObject(Message)) {
                    DEBUG_LINE("sent");
                    Runner->Scheduler().in(std::chrono::seconds(Client->HealthInterval_),
                                              OWLSclientEvents::HealthCheck, Client, Runner);
                    return;
                }
            } catch (const Poco::Exception &E) {
                DEBUG_LINE("exception1");
                Client->Logger().log(E);
            } catch (const std::exception &E) {
                DEBUG_LINE("exception2");
            }
            OWLSclientEvents::Disconnect(Client, Runner, "Error while sending HealthCheck", true);
        }
    }

}