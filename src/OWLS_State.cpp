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

    void State(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner) {
        std::lock_guard G(Client->Mutex_);

        std::cout << "State 1" << std::endl;
        if(Client->Valid_ && Client->Connected_) {

            Runner->Report().ev_state++;
            try {
                nlohmann::json StateDoc;

                std::cout << "State 2" << std::endl;
                StateDoc["jsonrpc"] = "2.0";
                StateDoc["method"] = "state";

                std::cout << __LINE__ << std::endl;

                nlohmann::json ParamsObj;
                ParamsObj["serial"] = Client->Serial();
                std::cout << __LINE__ << std::endl;
                ParamsObj["uuid"] = Client->UUID();
                std::cout << __LINE__ << std::endl;
                ParamsObj["state"] = Client->CreateState();
                std::cout << __LINE__ << std::endl;

                auto ParamsStr = to_string(ParamsObj);
                std::cout << __LINE__ << std::endl;


                unsigned long BufSize = ParamsStr.size() + 4000;
                std::vector<Bytef> Buffer(BufSize);
                compress(&Buffer[0], &BufSize, (Bytef *)ParamsStr.c_str(), ParamsStr.size());
                std::cout << __LINE__ << std::endl;

                auto CompressedBase64Encoded = OpenWifi::Utils::base64encode(&Buffer[0], BufSize);

                std::cout << __LINE__ << std::endl;
                StateDoc["params"]["compress_64"] = CompressedBase64Encoded;
                StateDoc["params"]["compress_sz"] = ParamsStr.size();
                std::cout << __LINE__ << ":" << to_string(StateDoc) << std::endl;

                if (Client->Send(StateDoc)) {
                    std::cout << __LINE__ << std::endl;
                    Runner->Scheduler().in(std::chrono::seconds(Client->StatisticsInterval_),
                                              OWLSclientEvents::State, Client, Runner);
                    return;
                }
                std::cout << __LINE__ << std::endl;
            } catch (const Poco::Exception &E) {
                std::cout << __LINE__ << std::endl;
                Client->Logger().log(E);
            }
            std::cout << __LINE__ << std::endl;
            OWLSclientEvents::Disconnect(Client, Runner, "Error sending stats event", true);
        }
    }

}