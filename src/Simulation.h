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

        private:
        static SimulationCoordinator 		    *instance_;
            Poco::Thread                Worker_;
            std::atomic_bool            Running_ = false;

            SimulationCoordinator() noexcept:
                SubSystemServer("SimulationCoordinator", "SIM-COORDINATOR", "coordinator")
            {
            }
    };

    inline SimulationCoordinator * SimulationCoordinator() { return SimulationCoordinator::instance(); }
    inline class SimulationCoordinator * SimulationCoordinator::instance_= nullptr;

}

#endif //OWLS_SIMULATION_H
