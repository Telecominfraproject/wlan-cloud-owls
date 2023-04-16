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

    void Connect(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner) {
        std::lock_guard G(Client->Mutex_);

        DEBUG_LINE("start");
        if(Client->Valid_) {
            try {
                Runner->Report().ev_connect++;

/*                nlohmann::json M;
                M["jsonrpc"] = "2.0";
                M["method"] = "connect";
                M["params"]["serial"] = Client->Serial();
                M["params"]["uuid"] = Client->UUID();
                M["params"]["firmware"] = Client->Firmware();
                auto TmpCapabilities2 = SimulationCoordinator()->GetSimCapabilities();
                auto LabelMac = Utils::SerialNumberToInt(Client->Serial());
                auto LabelMacFormatted = Utils::SerialToMAC(Utils::IntToSerialNumber(LabelMac));
                auto LabelLanMacFormatted = Utils::SerialToMAC(Utils::IntToSerialNumber(LabelMac + 1));
                TmpCapabilities2["label_macaddr"] = LabelMac;
                TmpCapabilities2["macaddr"]["wan"] = LabelMac;
                TmpCapabilities2["macaddr"]["lan"] = LabelLanMacFormatted;
                M["params"]["capabilities"] = TmpCapabilities2;
*/

                Poco::JSON::Object  ConnectMessage, Params, TmpCapabilities, Capabilities, MacAddr;
                auto LabelMac = Utils::SerialNumberToInt(Client->SerialNumber_);
                Params.set("serial", Client->SerialNumber_);
                Params.set("uuid", Client->UUID_);
                Params.set("firmware", Client->Firmware_);
                MacAddr.set("wan", Client->SerialNumber_);
                MacAddr.set("lan", Utils::SerialToMAC(Utils::IntToSerialNumber(LabelMac + 1)));
                TmpCapabilities = *SimulationCoordinator()->GetSimCapabilitiesPtr();
                TmpCapabilities.set("label_macaddr", Client->SerialNumber_);
                TmpCapabilities.set("macaddr", MacAddr);
                Params.set("capabilities", TmpCapabilities);

                OWLSutils::MakeHeader(ConnectMessage,"connect",Params);

                if (Client->SendObject(ConnectMessage)) {
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