//
// Created by stephane bourque on 2023-04-12.
//

#pragma once

#include <memory>

namespace OpenWifi {
    class OWLSclient;
    class SimulationRunner;
}

namespace OpenWifi::OWLSClientEvents {
    void EstablishConnection(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner);
    void Reconnect(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner);
    void Connect(std::lock_guard<std::mutex> & ClientGuard, const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner);
    void State(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner);
    void HealthCheck(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner);
    void Log(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner, std::uint64_t Severity, const std::string & LogLine);
    void WSPing(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner);
    void Update(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner);
    void KeepAlive(const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner);
    void Disconnect(std::lock_guard<std::mutex> &g, const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner, const std::string &Reason, bool Reconnect);
    void CrashLog(std::lock_guard<std::mutex> &g, const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner);
    void PendingConfig(std::lock_guard<std::mutex> &g, const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner);
};
