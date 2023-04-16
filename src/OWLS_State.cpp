//
// Created by stephane bourque on 2023-04-12.
//

#include "OWLSclient.h"
#include "SimulationRunner.h"
#include "SimulationCoordinator.h"
#include <fmt/format.h>
#include "SimStats.h"
#include <Poco/NObserver.h>
#include "OWLSdefinitions.h"
#include "OWLSclientEvents.h"

namespace OpenWifi::OWLSclientEvents {

    void State(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner) {

        std::lock_guard G(Client->Mutex_);

        DEBUG_LINE("start");
        if(Client->Valid_ && Client->Connected_) {

            Runner->Report().ev_state++;
            try {


                Runner->Scheduler().in(std::chrono::seconds(Client->StatisticsInterval_),
                                       OWLSclientEvents::State, Client, Runner);
                return;
/*

                nlohmann::json StateDoc;

                StateDoc["jsonrpc"] = "2.0";
                StateDoc["method"] = "state";

                nlohmann::json ParamsObj;
                ParamsObj["serial"] = Client->Serial();
                ParamsObj["uuid"] = Client->UUID();
                ParamsObj["state"] = Client->CreateState();

                auto ParamsStr = to_string(ParamsObj);
                unsigned long BufSize = ParamsStr.size() + 4000;
                std::vector<Bytef> Buffer(BufSize);
                compress(&Buffer[0], &BufSize, (Bytef *)ParamsStr.c_str(), ParamsStr.size());
                auto CompressedBase64Encoded = OpenWifi::Utils::base64encode(&Buffer[0], BufSize);

                StateDoc["params"]["compress_64"] = CompressedBase64Encoded;
                StateDoc["params"]["compress_sz"] = ParamsStr.size();

                if (Client->SendObject(StateDoc)) {
                    DEBUG_LINE("Sent");
                    Runner->Scheduler().in(std::chrono::seconds(Client->StatisticsInterval_),
                                              OWLSclientEvents::State, Client, Runner);
                    return;
                }
                */
            } catch (const Poco::Exception &E) {
                DEBUG_LINE("exception1");
                Client->Logger().log(E);
            } catch (const std::exception &E) {
                DEBUG_LINE("exception2");
            }
            DEBUG_LINE("failed");
            OWLSclientEvents::Disconnect(Client, Runner, "Error sending stats event", true);
        }
    }

}