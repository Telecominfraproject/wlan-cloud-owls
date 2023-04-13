//
// Created by stephane bourque on 2023-04-12.
//
#include "OWLSclient.h"
#include "SimulationRunner.h"
#include "SimulationCoordinator.h"
#include <fmt/format.h>
#include "OWLSscheduler.h"
#include "SimStats.h"
#include <Poco/NObserver.h>

#include "OWLSclientEvents.h"

namespace OpenWifi::OWLSclientEvents {

    void HealthCheck(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner) {
        std::lock_guard G(Client->Mutex_);

        if(Client->Valid_ && Client->Connected_) {
            Runner->Report().ev_healthcheck++;
            try {
                nlohmann::json M, P;
                P["memory"] = 23;
                M["jsonrpc"] = "2.0";
                M["method"] = "healthcheck";
                M["params"]["serial"] = Client->Serial();
                M["params"]["uuid"] = Client->UUID();
                M["params"]["sanity"] = 100;
                M["params"]["data"] = P;

                if (Client->Send(to_string(M))) {
                    OWLSscheduler()->Ref().in(std::chrono::seconds(Client->HealthInterval_),
                                              OWLSclientEvents::HealthCheck, Client, Runner);
                    return;
                }
            } catch (const Poco::Exception &E) {
                Client->Logger().log(E);
            }
            OWLSclientEvents::Disconnect(Client, Runner, "Error while sending HealthCheck", true);
        }
    }

}