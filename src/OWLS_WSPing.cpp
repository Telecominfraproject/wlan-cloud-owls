//
// Created by stephane bourque on 2023-04-12.
//
#include <Poco/NObserver.h>

#include "OWLSclient.h"
#include "SimulationRunner.h"
#include "SimStats.h"
#include "OWLSclientEvents.h"

namespace OpenWifi::OWLSClientEvents {

    void WSPing(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner) {
        if(!Runner->Running()) {
            return;
        }
        std::lock_guard<std::mutex> ClientGuard(Client->Mutex_);
        if(Client->Valid_ && Client->Connected_) {
            Runner->Report().ev_wsping++;
            try {
                Client->WS_->sendFrame(
                        "", 0, Poco::Net::WebSocket::FRAME_OP_PING | Poco::Net::WebSocket::FRAME_FLAG_FIN);
                Runner->Scheduler().in(std::chrono::seconds(60 * 3),
                                          OWLSClientEvents::WSPing, Client, Runner);
                return;
            } catch (const Poco::Exception &E) {
                poco_warning(Client->Logger_,fmt::format("WSPing({}): exception {}", Client->SerialNumber_, E.displayText()));
            } catch (const std::exception &E) {
                poco_warning(Client->Logger_,fmt::format("WSPing({}): std::exception {}", Client->SerialNumber_, E.what()));
            }
            OWLSClientEvents::Disconnect(__func__, ClientGuard, Client, Runner, "Error in WSPing", true);
        }
    }

}