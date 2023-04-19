//
// Created by stephane bourque on 2023-04-12.
//

#pragma once

#include <memory>

namespace OpenWifi {
    class OWLSclient;
    class SimulationRunner;
}

namespace OpenWifi::OWLSclientEvents {
    void EstablishConnection(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner);
    void Reconnect(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner);
    void Connect(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner);
    void State(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner);
    void HealthCheck(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner);
    void Log(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner, std::uint64_t Severity, const std::string & LogLine);
    void WSPing(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner);
    void Update(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner);
    void KeepAlive(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner);
    void Disconnect(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner, const std::string &Reason, bool Reconnect);
    void CrashLog(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner);
    void PendingConfig(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner);
};
