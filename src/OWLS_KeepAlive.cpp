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

    void KeepAlive(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner) {
        std::lock_guard G(Client->Mutex_);

        DEBUG_LINE("start");
        if(Client->Valid_ && Client->Connected_) {
            Runner->Report().ev_keepalive++;
            try {
                nlohmann::json M;

                M["jsonrpc"] = "2.0";
                M["method"] = "ping";
                M["params"]["serial"] = Client->Serial();
                M["params"]["uuid"] = Client->UUID();

                if (Client->Send(to_string(M))) {
                    DEBUG_LINE("sent");
                    Runner->Scheduler().in(std::chrono::seconds(Runner->Details().keepAlive),
                                              OWLSclientEvents::KeepAlive, Client, Runner);
                    return;
                }
            } catch (const Poco::Exception &E) {
                DEBUG_LINE("exception1");
                Client->Logger().log(E);
            } catch (const std::exception &E) {
                DEBUG_LINE("exception2");
            }
            OWLSclientEvents::Disconnect(Client, Runner, "Error while sending keepalive", true);
        }
    }

}