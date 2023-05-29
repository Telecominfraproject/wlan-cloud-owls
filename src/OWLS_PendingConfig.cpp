//
// Created by stephane bourque on 2023-04-12.
//

#include <Poco/NObserver.h>

#include "OWLSclient.h"
#include "SimulationRunner.h"
#include "SimStats.h"
#include "OWLSclientEvents.h"

namespace OpenWifi::OWLSClientEvents {

    void PendingConfig(std::lock_guard<std::mutex> &ClientGuard, const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner) {
        if(!Runner->Running()) {
            return;
        }

        if(Client->Valid_ && Client->Connected_) {
            Runner->Report().ev_configpendingchange++;
            try {
                Poco::JSON::Object::Ptr  Message{new Poco::JSON::Object}, Params{new Poco::JSON::Object};
                Params->set("serial", Client->SerialNumber_);
                Params->set("uuid", Client->UUID_);
                Params->set("active", Client->Active_);
                OWLSutils::MakeHeader(Message,"cfgpending",Params);

                if (Client->SendObject(__func__,Message)) {
                    return;
                }
            } catch (const Poco::Exception &E) {
                DEBUG_LINE("exception1");
                Client->Logger().log(E);
            } catch (const std::exception &E) {
                DEBUG_LINE("exception2");
            }
            OWLSClientEvents::Disconnect(ClientGuard, Client, Runner, "Error while sending ConfigPendingEvent", true);
        }
    }

}