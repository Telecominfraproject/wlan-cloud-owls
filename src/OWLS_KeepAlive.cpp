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

    void KeepAlive(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner) {
        std::lock_guard G(Client->Mutex_);

        if(Client->Valid_ && Client->Connected_) {
            Runner->Report().ev_keepalive++;
            try {
                nlohmann::json M;

                M["jsonrpc"] = "2.0";
                M["method"] = "ping";
                M["params"]["serial"] = Client->Serial();
                M["params"]["uuid"] = Client->UUID();

                if (Client->Send(to_string(M))) {
                    OWLSscheduler()->Ref().in(std::chrono::seconds(Runner->Details().keepAlive),
                                              OWLSclientEvents::KeepAlive, Client, Runner);
                    return;
                }
            } catch (const Poco::Exception &E) {
                Client->Logger().log(E);
            }
            OWLSclientEvents::Disconnect(Client, Runner, "Error while sending keepalive", true);
        }
    }

}