//
// Created by stephane bourque on 2023-04-12.
//
#include <Poco/NObserver.h>

#include "OWLSclient.h"
#include "SimulationRunner.h"
#include "SimStats.h"
#include "OWLSclientEvents.h"

namespace OpenWifi::OWLSClientEvents {

    void CrashLog([[
    maybe_unused]] std::lock_guard<std::mutex> &ClientGuard, const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner) {
        if(!Runner->Running()) {
            return;
        }
        if(Client->Valid_) {
            Runner->Report().ev_crashlog++;
        }
    }

}