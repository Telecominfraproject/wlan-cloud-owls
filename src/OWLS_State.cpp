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
                nlohmann::json StateDoc;

                StateDoc["jsonrpc"] = "2.0";
                StateDoc["method"] = "state";

                Poco::JSON::Object  Message, TempParams, Params;

                nlohmann::json ParamsObj;
                ParamsObj["serial"] = Client->Serial();
                ParamsObj["uuid"] = Client->UUID();
                ParamsObj["state"] = Client->CreateState();

                TempParams.set("serial", Client->SerialNumber_);
                TempParams.set("uuid", Client->UUID_);
                TempParams.set("state", Client->CreateStatePtr());

                std::ostringstream os;
                TempParams.stringify(os);

                std::cout << "State: " << os.str() << std::endl;

                auto ParamsStr = to_string(ParamsObj);
                unsigned long BufSize = os.str().size() + 4000;
                std::vector<Bytef> Buffer(BufSize);
                compress(&Buffer[0], &BufSize, (Bytef *)os.str().c_str(), os.str().size());
                auto CompressedBase64Encoded = OpenWifi::Utils::base64encode(&Buffer[0], BufSize);

                Params.set("compress_64", CompressedBase64Encoded);
                Params.set("compress_sz", os.str().size());

                OWLSutils::MakeHeader(Message,"state",Params);


                Runner->Scheduler().in(std::chrono::seconds(Client->StatisticsInterval_),
                                       OWLSclientEvents::State, Client, Runner);
                return;

                /*
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