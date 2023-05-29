//
// Created by stephane bourque on 2023-04-12.
//
#include <Poco/NObserver.h>

#include "OWLSclient.h"
#include "SimulationRunner.h"
#include "SimStats.h"
#include "OWLSclientEvents.h"

namespace OpenWifi::OWLSClientEvents {

    void Log(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner, std::uint64_t Severity, const std::string & LogLine) {
        if(!Runner->Running()) {
            return;
        }

        std::lock_guard     ClientGuard(Client->Mutex_);
        if(Client->Valid_ && Client->Connected_ ) {
            Runner->Report().ev_log++;
            try {
                Poco::JSON::Object::Ptr  Message{new Poco::JSON::Object}, Params{new Poco::JSON::Object};
                Params->set("serial", Client->SerialNumber_);
                Params->set("uuid", Client->UUID_);
                Params->set("severity", Severity);
                Params->set("log", LogLine);
                OWLSutils::MakeHeader(Message,"log",Params);

                if (Client->SendObject(__func__,Message)) {
                    return;
                }
            } catch (const Poco::Exception &E) {
                DEBUG_LINE("exception1");
                Client->Logger().log(E);
            } catch (const std::exception &E) {
                DEBUG_LINE("exception2");
            }
            OWLSClientEvents::Disconnect(__func__, ClientGuard,Client, Runner, "Error while sending a Log event", true);
        }
    }

}