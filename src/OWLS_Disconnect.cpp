//
// Created by stephane bourque on 2023-04-12.
//

#include <fmt/format.h>
#include <Poco/NObserver.h>

#include "OWLSclient.h"
#include "SimulationRunner.h"
#include "SimStats.h"
#include "OWLSclientEvents.h"
#include "OWLS_utils.h"

namespace OpenWifi::OWLSClientEvents {

    void Disconnect(std::lock_guard<std::mutex> &ClientGuard, const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner,
                    const std::string &Reason, bool Reconnect) {

        if(Client->Valid_) {
            Client->Disconnect(ClientGuard);
            poco_debug(Client->Logger(),fmt::format("{}: disconnecting. Reason: {}", Client->SerialNumber_, Reason));
            if(Reconnect) {
                std::cout << "Reconnecting: " << Client->SerialNumber_ << std::endl;
                Runner->Scheduler().in(std::chrono::seconds(Client->Backoff()),
                                          OWLSClientEvents::EstablishConnection, Client, Runner);
            } else {
//                DEBUG_LINE("not reconnecting");
            }
        }
    }

}