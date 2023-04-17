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
#include "OWLS_utils.h"

namespace OpenWifi::OWLSclientEvents {

    void Reconnect(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner) {
        std::lock_guard G(Client->Mutex_);

        try {
            if(Client->Valid_) {
                Runner->Report().ev_reconnect++;
                Client->Connected_ = false;
                Runner->Scheduler().in(std::chrono::seconds(OWLSutils::local_random(3,15)), OWLSclientEvents::EstablishConnection, Client, Runner);
            }
        } catch (const Poco::Exception &E) {
            DEBUG_LINE("exception1");
            Client->Logger().log(E);
        } catch (const std::exception &E) {
            DEBUG_LINE("exception2");
        }
    }

}