//
// Created by stephane bourque on 2021-04-07.
//

#ifndef UCENTRALSIM_SIMSTATS_H
#define UCENTRALSIM_SIMSTATS_H

#include "framework/MicroService.h"
#include "RESTObjects/RESTAPI_OWLSobjects.h"

namespace OpenWifi {

    class SimStats : public SubSystemServer {

    public:
        inline void Connect() {
            std::lock_guard G(Mutex_);
            Status_.liveDevices++;
        }

        inline void Disconnect() {
            std::lock_guard G(Mutex_);
            Status_.liveDevices--;
        }

        static SimStats * instance() {
            if(instance_ == nullptr)
                instance_ = new SimStats;
            return instance_;
        }

        inline void AddRX(uint64_t N) {
            std::lock_guard G(Mutex_);
            Status_.rx += N;
        }

        inline void AddOutMsg() {
            std::lock_guard G(Mutex_);
            Status_.msgsTx++;
        }

        inline void AddInMsg() {
            std::lock_guard G(Mutex_);
            Status_.msgsRx++;
        }

        inline void AddTX(uint64_t N) {
            std::lock_guard G(Mutex_);
            Status_.tx += N;
        }

        inline void GetCurrent( OWLSObjects::SimulationStatus & S) {
            std::lock_guard G(Mutex_);
            S = Status_;
        }

        inline int Start() final {
            Reset();
            return 0;
        }

        inline void Stop() {

        }

        inline void StartSim(const std::string &id , const std::string & simid) {
            std::lock_guard G(Mutex_);
            Status_.id = id;
            Status_.simulationId = simid;
            Status_.state = "running";
            Status_.liveDevices = Status_.endTime = Status_.rx =
            Status_.tx = Status_.msgsTx = Status_.msgsRx = Status_.timeToFullDevices =
            Status_.errorDevices = 0;
            Status_.startTime = std::time(nullptr);
        }

        inline void EndSim() {
            std::lock_guard G(Mutex_);
            Status_.state = "done";
            Status_.endTime = std::time(nullptr);
        }

        inline void SetState(const std::string &S) {
            std::lock_guard G(Mutex_);
            Status_.state = S;
        }

        [[nodiscard]] const std::string & GetState() {
            std::lock_guard G(Mutex_);
            return Status_.state;
        }

        inline void Reset() {
            Status_.liveDevices = Status_.tx = Status_.msgsRx = Status_.msgsTx = Status_.rx =
                    Status_.endTime = Status_.errorDevices = Status_.startTime = Status_.timeToFullDevices = 0;
            Status_.simulationId = Status_.id = Status_.state = "";
        }

    private:
        static SimStats                 * instance_;
        OWLSObjects::SimulationStatus   Status_;

        SimStats() noexcept:
        SubSystemServer("SimStats", "SIM-STATS", "stats")
            {
            }
    };

    inline SimStats * SimStats() { return SimStats::instance(); }
    inline class SimStats * SimStats::instance_= nullptr;
}

#endif //UCENTRALSIM_SIMSTATS_H
