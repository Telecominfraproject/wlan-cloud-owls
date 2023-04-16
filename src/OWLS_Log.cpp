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

    void Log(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner, std::uint64_t Severity, const std::string & LogLine) {
        std::lock_guard G(Client->Mutex_);

        if(Client->Valid_ && Client->Connected_ ) {
            Runner->Report().ev_log++;
            try {
                nlohmann::json M;

                M["jsonrpc"] = "2.0";
                M["method"] = "log";
                M["params"]["serial"] = Client->Serial();
                M["params"]["uuid"] = Client->UUID();
                M["params"]["severity"] = Severity;
                M["params"]["log"] = LogLine;

                if (Client->Send(to_string(M))) {
                    return;
                }
            } catch (const Poco::Exception &E) {
                Client->Logger().log(E);
            }
            OWLSclientEvents::Disconnect(Client, Runner, "Error while sending a Log event", true);
        }
    }

}