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

namespace OpenWifi::OWLSClientEvents {

    void Connect(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner) {
        std::lock_guard     ClientGuard(Client->Mutex_);
        if(Client->Valid_) {
            try {
                Runner->Report().ev_connect++;

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
                                              OWLSClientEvents::State, Client, Runner);
                    Runner->Scheduler().in(std::chrono::seconds(Client->HealthInterval_),
                                              OWLSClientEvents::HealthCheck, Client, Runner);
                    Runner->Scheduler().in(std::chrono::seconds(MicroServiceRandom(120, 200)),
                                              OWLSClientEvents::Log, Client, Runner, 1, "Device started");
                    Runner->Scheduler().in(std::chrono::seconds(60 * 4),
                                              OWLSClientEvents::WSPing, Client, Runner);
                    Runner->Scheduler().in(std::chrono::seconds(30),
                                              OWLSClientEvents::Update, Client, Runner);
                    Client->Logger_.information(fmt::format("connect({}): completed.", Client->SerialNumber_));
                    return;
                }
            } catch (const Poco::Exception &E) {
                Client->Logger().log(E);
            }
            OWLSClientEvents::Disconnect(ClientGuard,Client, Runner, "Error occurred during connection", true);
        }
    }

}