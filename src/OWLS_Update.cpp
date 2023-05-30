//
// Created by stephane bourque on 2023-04-12.
//
#include <Poco/NObserver.h>

#include "OWLSclient.h"
#include "SimulationRunner.h"
#include "SimStats.h"
#include "OWLSclientEvents.h"

namespace OpenWifi::OWLSClientEvents {

    void Update(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner) {
        if(!Runner->Running()) {
            return;
        }

        std::lock_guard ClientGuard(Client->Mutex_);

        try {
            if(Client->Valid_ && Client->Connected_) {
                Runner->Report().ev_update++;
                Client->Update();
                Runner->Scheduler().in(std::chrono::seconds(30),
                                       OWLSClientEvents::Update, Client, Runner);
            }
        } catch (const Poco::Exception &E) {
            poco_warning(Client->Logger_,fmt::format("Update({}): exception {}", Client->SerialNumber_, E.displayText()));
        } catch (const std::exception &E) {
            poco_warning(Client->Logger_,fmt::format("Update({}): std::exception {}", Client->SerialNumber_, E.what()));
        }
    }

}