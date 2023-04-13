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
#include "OWLSevent.h"

namespace OpenWifi::OWLSclientEvents {

    void CrashLog(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner) {
        std::lock_guard G(Client->Mutex_);

        if(Client->Valid_) {
            Runner->Report().ev_crashlog++;
        }
    }

}