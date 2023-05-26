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

namespace OpenWifi::OWLSClientEvents {

    void HealthCheck(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner) {
        std::lock_guard ClientGuard(Client->Mutex_);

//        DEBUG_LINE("start");
        if(Client->Valid_ && Client->Connected_) {
            Runner->Report().ev_healthcheck++;
            try {

                Poco::JSON::Object  Message, Params, Data, Memory;
                Memory.set("memory", 23);
                Data.set("data", Memory);
                Params.set(uCentralProtocol::SERIAL, Client->SerialNumber_);
                Params.set(uCentralProtocol::UUID, Client->UUID_);
                Params.set(uCentralProtocol::SANITY, 100);
                Params.set(uCentralProtocol::DATA, Data);
                OWLSutils::MakeHeader(Message, uCentralProtocol::HEALTHCHECK, Params);

//                std::cout << Client->SerialNumber_ << "  H: " << Client->UUID_ << std::endl;

                if (Client->SendObject(Message)) {
                    Runner->Scheduler().in(std::chrono::seconds(Client->HealthInterval_),
                                              OWLSClientEvents::HealthCheck, Client, Runner);
                    return;
                }
            } catch (const Poco::Exception &E) {
                DEBUG_LINE("exception1");
                Client->Logger().log(E);
            } catch (const std::exception &E) {
                DEBUG_LINE("exception2");
            }
            OWLSClientEvents::Disconnect(ClientGuard, Client, Runner, "Error while sending HealthCheck", true);
        }
    }

}