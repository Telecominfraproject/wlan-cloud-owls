//
// Created by stephane bourque on 2021-11-03.
//

#include "Simulation.h"
#include "StorageService.h"

namespace OpenWifi {

    int SimulationCoordinator::Start() {
        CASLocation_ = MicroService::instance().ConfigGetString("ucentral.cas");
        KeyFileName_ = MicroService::instance().ConfigGetString("ucentral.key");
        CertFileName_ = MicroService::instance().ConfigGetString("ucentral.cert");
        RootCAFileName_ = MicroService::instance().ConfigGetString("ucentral.rootca");
        std::string L = MicroService::instance().ConfigGetString("ucentral.security");
        if (L == "strict") {
            Level_ = Poco::Net::Context::VERIFY_STRICT;
        } else if (L == "none") {
            Level_ = Poco::Net::Context::VERIFY_NONE;
        } else if (L == "relaxed") {
            Level_ = Poco::Net::Context::VERIFY_RELAXED;
        } else if (L == "once")
            Level_ = Poco::Net::Context::VERIFY_ONCE;
        Worker_.start(*this);
        return 0;
    }

    void SimulationCoordinator::Stop() {
        if(Running_) {
            Running_ = false;
            Worker_.wakeUp();
            Worker_.join();
        }
    }

    void SimulationCoordinator::run() {
        Running_ = true ;

        while(Running_) {
            Poco::Thread::trySleep(2000);
            if(!Running_)
                break;
        }
    }

    void SimulationCoordinator::StartSimulators() {
        Logger_.notice("Starting simulation threads...");
        for(const auto &i:SimThreads_)
            i->Thread.start(i->Sim);
    }

    void SimulationCoordinator::PauseSimulators() {
        Logger_.notice("Starting simulation threads...");
        for(const auto &i:SimThreads_)
            i->Sim.Pause();
    }

    void SimulationCoordinator::ResumeSimulators() {
        Logger_.notice("Starting simulation threads...");
        for(const auto &i:SimThreads_)
            i->Sim.Resume();
    }

    void SimulationCoordinator::CancelSimulators() {
        Logger_.notice("Starting simulation threads...");
        for(const auto &i:SimThreads_)
            i->Sim.Cancel();
    }

    void SimulationCoordinator::StopSimulators() {
        Logger_.notice("Stopping simulation threads...");
        for(const auto &i:SimThreads_) {
            i->Sim.stop();
            i->Thread.join();
        }
    }

    bool SimulationCoordinator::StartSim(const std::string &SimId, std::string & Id, std::string &Error) {
        if(SimRunning_) {
            Error = "Another simulation is already running.";
            return false;
        }

        if(!StorageService()->SimulationDB().GetRecord("id",SimId,CurrentSim_)) {
            Error = "Simulation ID specified does not exist.";
            return false;
        }

        auto ClientCount = CurrentSim_.devices;
        auto NumClientsPerThread = CurrentSim_.devices;

        // create the actual simulation...
        if(CurrentSim_.threads==0) {
            CurrentSim_.threads = Poco::Environment::processorCount() * 4;
        }
        if(CurrentSim_.devices>250) {
            if(CurrentSim_.devices % CurrentSim_.threads == 0)
            {
                NumClientsPerThread = CurrentSim_.devices / CurrentSim_.threads;
            }
            else
            {
                NumClientsPerThread = CurrentSim_.devices / (CurrentSim_.threads+1);
            }
        }

        Poco::Logger    & ClientLogger = Poco::Logger::get("WS-CLIENT");
        ClientLogger.setLevel(Poco::Message::PRIO_WARNING);
        for(auto i=0;ClientCount;i++)
        {
            auto Clients = std::min(ClientCount,NumClientsPerThread);
            auto NewSimThread = std::make_unique<SimThread>(i,CurrentSim_.macPrefix,Clients, Logger_);
            NewSimThread->Sim.Initialize(ClientLogger);
            SimThreads_.push_back(std::move(NewSimThread));
            ClientCount -= Clients;
        }

        StartSimulators();

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

        StopSimulators();

        SimRunning_ = false;
        Status_.state = "stopped";
        return true;
    }

    bool SimulationCoordinator::PauseSim(const std::string &Id, std::string &Error) {
        if(!SimRunning_) {
            Error = "No simulation is running.";
            return false;
        }
        PauseSimulators();
        Status_.state = "paused";
        return true;
    }

    bool SimulationCoordinator::CancelSim(const std::string &Id, std::string &Error) {
        if(!SimRunning_) {
            Error = "No simulation is running.";
            return false;
        }

        CancelSimulators();
        StopSimulators();

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

        ResumeSimulators();

        Status_.state = "running";
        return true;
    }

}