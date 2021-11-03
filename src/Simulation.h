//
// Created by stephane bourque on 2021-11-03.
//

#ifndef OWLS_SIMULATION_H
#define OWLS_SIMULATION_H

#include "framework/MicroService.h"
#include "RESTObjects/RESTAPI_OWLSobjects.h"

namespace OpenWifi {
    class SimulationCoordinator : public SubSystemServer, Poco::Runnable {
        public:
        static SimulationCoordinator *instance() {
                if(instance_== nullptr)
                    instance_ = new SimulationCoordinator;
                return instance_;
            }

            int Start() final;
            void Stop() final;
            void run() final;

            bool StartSim(const std::string &SimId, std::string & Id, std::string &Error);
            bool StopSim(const std::string &Id, std::string &Error);
            bool PauseSim(const std::string &Id, std::string &Error);
            bool CancelSim(const std::string &Id, std::string &Error);
            bool ResumeSim(const std::string &Id, std::string &Error);

            inline bool GetStatus( OWLSObjects::SimulationStatus & S) {
                std::lock_guard G(Mutex_);
                S = Status_;
                return true;
            }

        private:
        static SimulationCoordinator 		*instance_;
            Poco::Thread                    Worker_;
            std::atomic_bool                SimRunning_ = false;
            OWLSObjects::SimulationStatus   Status_;

            SimulationCoordinator() noexcept:
                SubSystemServer("SimulationCoordinator", "SIM-COORDINATOR", "coordinator")
            {
            }
    };

    inline SimulationCoordinator * SimulationCoordinator() { return SimulationCoordinator::instance(); }
    inline class SimulationCoordinator * SimulationCoordinator::instance_= nullptr;

}

#endif //OWLS_SIMULATION_H
