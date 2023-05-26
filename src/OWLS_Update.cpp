//
// Created by stephane bourque on 2023-04-12.
//
#include "OWLSclient.h"
#include "SimulationRunner.h"
#include "SimStats.h"
#include <Poco/NObserver.h>
#include "OWLSclientEvents.h"

namespace OpenWifi::OWLSClientEvents {

    void Update(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner) {
        std::lock_guard ClientGuard(Client->Mutex_);

        try {
            if(Client->Valid_ && Client->Connected_) {
                Runner->Report().ev_update++;
                Client->Update();
                Runner->Scheduler().in(std::chrono::seconds(30),
                                       OWLSClientEvents::Update, Client, Runner);
            }
        } catch (const Poco::Exception &E) {
            DEBUG_LINE("exception1");
            Client->Logger().log(E);
        } catch (const std::exception &E) {
            DEBUG_LINE("exception2");
        }
    }

}