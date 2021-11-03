//
// Created by stephane bourque on 2021-11-03.
//

#include "Simulation.h"

namespace OpenWifi {

    int SimulationCoordinator::Start() {
        return 0;
    }

    void SimulationCoordinator::Stop() {

    }

    void SimulationCoordinator::run() {

    }

    bool SimulationCoordinator::StartSim(const std::string &SimId, std::string & Id, std::string &Error) {
        if(SimRunning_) {
            Error = "Another simulation is already running.";
            return false;
        }
        SimRunning_ = true ;
        Status_.id = MicroService::instance().CreateUUID();
        Status_.simulationId = SimId;
        Status_.state = "running";
        return true;
    }

    bool SimulationCoordinator::StopSim(const std::string &Id, std::string &Error) {
        if(!SimRunning_) {
            Error = "No simulation is running.";
            return false;
        }
        SimRunning_ = false;
        Status_.state = "stopped";
        return true;
    }

    bool SimulationCoordinator::PauseSim(const std::string &Id, std::string &Error) {
        if(!SimRunning_) {
            Error = "No simulation is running.";
            return false;
        }
        Status_.state = "paused";
        return true;
    }

    bool SimulationCoordinator::CancelSim(const std::string &Id, std::string &Error) {
        if(!SimRunning_) {
            Error = "No simulation is running.";
            return false;
        }

        SimRunning_ = false;
        Status_.id.clear();
        Status_.simulationId.clear();
        Status_.state = "none";

        return true;
    }

    bool SimulationCoordinator::ResumeSim(const std::string &Id, std::string &Error) {
        if(!SimRunning_) {
            Error = "No simulation is running.";
            return false;
        }

        if(Status_.state!="paused") {
            Error = "Simulation must be paused first.";
            return false;
        }

        Status_.state = "running";
        return true;
    }

}