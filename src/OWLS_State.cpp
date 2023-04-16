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

        DEBUG_LINE;
        if(Client->Valid_ && Client->Connected_) {

            Runner->Report().ev_state++;
            try {
                nlohmann::json StateDoc;

                DEBUG_LINE;
                StateDoc["jsonrpc"] = "2.0";
                StateDoc["method"] = "state";

                DEBUG_LINE;

                nlohmann::json ParamsObj;
                ParamsObj["serial"] = Client->Serial();
                DEBUG_LINE;
                ParamsObj["uuid"] = Client->UUID();
                DEBUG_LINE;
                ParamsObj["state"] = Client->CreateState();
                DEBUG_LINE;

                auto ParamsStr = to_string(ParamsObj);
                DEBUG_LINE;


                unsigned long BufSize = ParamsStr.size() + 4000;
                std::vector<Bytef> Buffer(BufSize);
                compress(&Buffer[0], &BufSize, (Bytef *)ParamsStr.c_str(), ParamsStr.size());
                DEBUG_LINE;

                auto CompressedBase64Encoded = OpenWifi::Utils::base64encode(&Buffer[0], BufSize);

                DEBUG_LINE;
                StateDoc["params"]["compress_64"] = CompressedBase64Encoded;
                DEBUG_LINE;
                StateDoc["params"]["compress_sz"] = ParamsStr.size();
                DEBUG_LINE;

                if (Client->SendObject(StateDoc)) {
                    DEBUG_LINE;
                    Runner->Scheduler().in(std::chrono::seconds(Client->StatisticsInterval_),
                                              OWLSclientEvents::State, Client, Runner);
                    return;
                }
                DEBUG_LINE;
            } catch (const Poco::Exception &E) {
                DEBUG_LINE;
                Client->Logger().log(E);
            } catch (const std::exception &E) {
                std::cout << "Exception in state: " << E.what() << std::endl;
            }
            DEBUG_LINE;
            OWLSclientEvents::Disconnect(Client, Runner, "Error sending stats event", true);
        }
    }

}