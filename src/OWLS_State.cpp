//
// Created by stephane bourque on 2023-04-12.
//

#include <fmt/format.h>
#include <Poco/NObserver.h>

#include "OWLSclient.h"
#include "SimulationRunner.h"
#include "SimStats.h"
#include "OWLSdefinitions.h"
#include "OWLSclientEvents.h"

namespace OpenWifi::OWLSClientEvents {

    void State(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner) {
        if(!Runner->Running()) {
            return;
        }

        std::lock_guard ClientGuard(Client->Mutex_);

        if(Client->Valid_ && Client->Connected_) {

            Runner->Report().ev_state++;
            try {
                Poco::JSON::Object::Ptr  Message{new Poco::JSON::Object}, TempParams{new Poco::JSON::Object}, Params{new Poco::JSON::Object};

                TempParams->set(uCentralProtocol::SERIAL, Client->SerialNumber_);
                TempParams->set(uCentralProtocol::UUID, Client->UUID_);
                TempParams->set(uCentralProtocol::STATE, Client->CreateStatePtr());

                std::ostringstream os;
                TempParams->stringify(os);

                unsigned long BufSize = os.str().size() + 4000;
                std::vector<Bytef> Buffer(BufSize);
                compress(&Buffer[0], &BufSize, (Bytef *)os.str().c_str(), os.str().size());
                auto CompressedBase64Encoded = OpenWifi::Utils::base64encode(&Buffer[0], BufSize);

                Params->set("compress_64", CompressedBase64Encoded);
                Params->set("compress_sz", os.str().size());

                OWLSutils::MakeHeader(Message,uCentralProtocol::STATE,Params);

                if (Client->SendObject(Message)) {
                    Runner->Scheduler().in(std::chrono::seconds(Client->StatisticsInterval_),
                                           OWLSClientEvents::State, Client, Runner);
                    return;
                }
            } catch (const Poco::Exception &E) {
                std::cout << "E:" << E.name() << " | " << E.what() << " | " << E.message() << std::endl;
                Client->Logger().log(E);
            } catch (const std::exception &E) {
                DEBUG_LINE("exception2");
            }
            DEBUG_LINE("failed");
            OWLSClientEvents::Disconnect(ClientGuard,Client, Runner, "Error sending stats event", true);
        }
    }

}