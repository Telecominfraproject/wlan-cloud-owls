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
#include "OWLSevent.h"

namespace OpenWifi::OWLSclientEvents {

    void Connect(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner) {
        std::lock_guard G(Client->Mutex_);

        DEBUG_LINE("start");
        if(Client->Valid_) {
            try {
                Runner->Report().ev_connect++;
                nlohmann::json M;
                M["jsonrpc"] = "2.0";
                M["method"] = "connect";
                M["params"]["serial"] = Client->Serial();
                M["params"]["uuid"] = Client->UUID();
                M["params"]["firmware"] = Client->Firmware();
                auto TmpCapabilities = SimulationCoordinator()->GetSimCapabilities();
                auto LabelMac = Utils::SerialNumberToInt(Client->Serial());
                auto LabelMacFormatted = Utils::SerialToMAC(Utils::IntToSerialNumber(LabelMac));
                auto LabelLanMacFormatted = Utils::SerialToMAC(Utils::IntToSerialNumber(LabelMac + 1));
                TmpCapabilities["label_macaddr"] = LabelMac;
                TmpCapabilities["macaddr"]["wan"] = LabelMac;
                TmpCapabilities["macaddr"]["lan"] = LabelLanMacFormatted;
                M["params"]["capabilities"] = TmpCapabilities;
                if (Client->Send(to_string(M))) {
                    Client->Reset();
                    Runner->Scheduler().in(std::chrono::seconds(Client->StatisticsInterval_),
                                              OWLSclientEvents::State, Client, Runner);
                    Runner->Scheduler().in(std::chrono::seconds(Client->HealthInterval_),
                                              OWLSclientEvents::HealthCheck, Client, Runner);
                    Runner->Scheduler().in(std::chrono::seconds(MicroServiceRandom(120, 200)),
                                              OWLSclientEvents::Log, Client, Runner, 1, "Device started");
                    Runner->Scheduler().in(std::chrono::seconds(60 * 4),
                                              OWLSclientEvents::WSPing, Client, Runner);
                    Runner->Scheduler().in(std::chrono::seconds(30),
                                              OWLSclientEvents::Update, Client, Runner);
                    std::cout << Client->SerialNumber_ << ":Fully connected" << std::endl;
                    return;
                }
            } catch (const Poco::Exception &E) {
                Client->Logger().log(E);
            }
            OWLSclientEvents::Disconnect(Client, Runner, "Error occurred during connection", true);
        }
    }

}