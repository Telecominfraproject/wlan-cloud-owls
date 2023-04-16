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
#include "OWLSevent.h"

namespace OpenWifi::OWLSclientEvents {

    void PendingConfig(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner) {
        std::lock_guard G(Client->Mutex_);

        DEBUG_LINE;
        if(Client->Valid_ && Client->Connected_) {
            Runner->Report().ev_configpendingchange++;
            try {
                nlohmann::json M;

                M["jsonrpc"] = "2.0";
                M["method"] = "cfgpending";
                M["params"]["serial"] = Client->Serial();
                M["params"]["uuid"] = Client->UUID();
                M["params"]["active"] = Client->Active();

                if (Client->Send(to_string(M))) {
                    return;
                }
            } catch (const Poco::Exception &E) {
                Client->Logger().log(E);
            }
            OWLSclientEvents::Disconnect(Client, Runner, "Error while sending ConfigPendingEvent", true);
        }
    }

}