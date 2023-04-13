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

    void WSPing(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner) {
        std::lock_guard G(Client->Mutex_);

        if(Client->Valid_ && Client->Connected_) {
            Runner->Report().ev_wsping++;
            try {
                if (Client->SendWSPing()) {
                    OWLSscheduler()->Ref().in(std::chrono::seconds(60 * 4),
                                              OWLSclientEvents::WSPing, Client, Runner);
                    return;
                }
            } catch (const Poco::Exception &E) {
                Client->Logger().log(E);
            }
            OWLSclientEvents::Disconnect(Client, Runner, "Error in WSPing", true);
        }
    }

}