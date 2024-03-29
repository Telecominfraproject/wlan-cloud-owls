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

    void Disconnect(const char *context, std::lock_guard<std::mutex> &ClientGuard, const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner,
                    const std::string &Reason, bool Reconnect) {

        if(!Runner->Running()) {
            return;
        }

        try {
            if (Client->Valid_) {
                Client->Disconnect(context, ClientGuard);
                poco_debug(Client->Logger(),
                           fmt::format("Disconnecting({}): Reason: {}", Client->SerialNumber_, Reason));
                if (Reconnect) {
                    poco_debug(Client->Logger_, fmt::format( "Reconnecting({}): {}", context, Client->SerialNumber_ ));
                    Runner->Scheduler().in(std::chrono::seconds(Client->Backoff()),
                                           OWLSClientEvents::EstablishConnection, Client, Runner);
                } else {
//                DEBUG_LINE("not reconnecting");
                }
            }
        } catch (const Poco::Exception &E) {
            poco_warning(Client->Logger_,fmt::format("Disconnect({}): exception {}", Client->SerialNumber_, E.displayText()));
        } catch (const std::exception &E) {
            poco_warning(Client->Logger_,fmt::format("Disconnect({}): std::exception {}", Client->SerialNumber_, E.what()));
        }
    }

}