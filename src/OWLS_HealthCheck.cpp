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

namespace OpenWifi::OWLSclientEvents {

    void HealthCheck(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner) {
        std::lock_guard G(Client->Mutex_);

//        DEBUG_LINE("start");
        if(Client->Valid_ && Client->Connected_) {
            Runner->Report().ev_healthcheck++;
            try {

                Poco::JSON::Object  Message, Params, Data, Memory;
                Memory.set("memory", 23);
                Data.set("data", Memory);
                Params.set(uCentralProtocol::DATA, Data);
                Params.set(uCentralProtocol::SERIAL, Client->SerialNumber_);
                Params.set(uCentralProtocol::REQUEST_UUID, Client->UUID_);
                Params.set(uCentralProtocol::SANITY, 100);
                Message.set(uCentralProtocol::UUID, Client->UUID_);
                OWLSutils::MakeHeader(Message, uCentralProtocol::HEALTHCHECK, Params);

                if (Client->SendObject(Message)) {
                    Runner->Scheduler().in(std::chrono::seconds(Client->HealthInterval_),
                                              OWLSclientEvents::HealthCheck, Client, Runner);
                    return;
                }
            } catch (const Poco::Exception &E) {
                DEBUG_LINE("exception1");
                Client->Logger().log(E);
            } catch (const std::exception &E) {
                DEBUG_LINE("exception2");
            }
            OWLSclientEvents::Disconnect(Client, Runner, "Error while sending HealthCheck", true);
        }
    }

}