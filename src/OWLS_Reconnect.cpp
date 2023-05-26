//
// Created by stephane bourque on 2023-04-12.
//

#include <Poco/NObserver.h>

#include "OWLSclient.h"
#include "SimulationRunner.h"
#include "SimStats.h"
#include "OWLSclientEvents.h"
#include "OWLS_utils.h"

namespace OpenWifi::OWLSClientEvents {

    void Reconnect(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner) {
        if(!Runner->Running()) {
            return;
        }

        std::lock_guard     ClientGuard(Client->Mutex_);
        try {
            if(Client->Valid_) {
                Runner->Report().ev_reconnect++;
                Client->Connected_ = false;
                Runner->Scheduler().in(std::chrono::seconds(OWLSutils::local_random(3,15)), OWLSClientEvents::EstablishConnection, Client, Runner);
            }
        } catch (const Poco::Exception &E) {
            DEBUG_LINE("exception1");
            Client->Logger().log(E);
        } catch (const std::exception &E) {
            DEBUG_LINE("exception2");
        }
    }

}