//
// Created by stephane bourque on 2023-04-12.
//

#include "OWLSclient.h"
#include "SimulationRunner.h"
#include "SimulationCoordinator.h"
#include <fmt/format.h>
#include "OWLSscheduler.h"
#include "SimStats.h"
#include <Poco/NObserver.h>

#include "OWLSclientEvents.h"

namespace OpenWifi::OWLSclientEvents {

    void State(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner) {
        std::lock_guard G(Client->Mutex_);

        if(Client->Valid_ && Client->Connected_) {
            Runner->Report().ev_state++;
            try {
                nlohmann::json M;

                M["jsonrpc"] = "2.0";
                M["method"] = "state";

                nlohmann::json ParamsObj;
                ParamsObj["serial"] = Client->Serial();
                ParamsObj["uuid"] = Client->UUID();
                ParamsObj["state"] = Client->CreateState();

                auto ParamsStr = to_string(ParamsObj);

                unsigned long BufSize = ParamsStr.size() + 4000;
                std::vector<Bytef> Buffer(BufSize);
                compress(&Buffer[0], &BufSize, (Bytef *)ParamsStr.c_str(), ParamsStr.size());

                auto CompressedBase64Encoded = OpenWifi::Utils::base64encode(&Buffer[0], BufSize);

                M["params"]["compress_64"] = CompressedBase64Encoded;
                M["params"]["compress_sz"] = ParamsStr.size();

                if (Client->Send(to_string(M))) {
                    OWLSscheduler()->Ref().in(std::chrono::seconds(Client->StatisticsInterval_),
                                              OWLSclientEvents::State, Client, Runner);
                    return;
                }
            } catch (const Poco::Exception &E) {
                Client->Logger().log(E);
            }
            OWLSclientEvents::Disconnect(Client, Runner, "Error sending stats event", true);
        }
    }

}