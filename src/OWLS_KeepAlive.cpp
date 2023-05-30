//
// Created by stephane bourque on 2023-04-12.
//
#include <Poco/NObserver.h>

#include "OWLSclient.h"
#include "SimulationRunner.h"
#include "SimStats.h"
#include "OWLSclientEvents.h"

namespace OpenWifi::OWLSClientEvents {

    void KeepAlive(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner) {
        if(!Runner->Running()) {
            return;
        }
        std::lock_guard     ClientGuard(Client->Mutex_);
        if(Client->Valid_ && Client->Connected_) {
            Runner->Report().ev_keepalive++;
            try {

                Poco::JSON::Object::Ptr Message{new Poco::JSON::Object},
                                        Params{new Poco::JSON::Object};
                Params->set("serial", Client->SerialNumber_);
                Params->set("uuid", Client->UUID_);
                OWLSutils::MakeHeader(Message,"ping",Params);

                if (Client->SendObject(__func__, Message)) {
                    Runner->Scheduler().in(std::chrono::seconds(Runner->Details().keepAlive),
                                              OWLSClientEvents::KeepAlive, Client, Runner);
                    return;
                }
            } catch (const Poco::Exception &E) {
                poco_warning(Client->Logger_,fmt::format("KeepAlive({}): exception {}", Client->SerialNumber_, E.displayText()));
            } catch (const std::exception &E) {
                poco_warning(Client->Logger_,fmt::format("KeepAlive({}): std::exception {}", Client->SerialNumber_, E.what()));
            }
            OWLSClientEvents::Disconnect(__func__, ClientGuard,Client, Runner, "Error while sending keepalive", true);
        }
    }

}