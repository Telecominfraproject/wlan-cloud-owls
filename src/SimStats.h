//
// Created by stephane bourque on 2021-04-07.
//

#pragma once

#include "framework/SubSystemServer.h"
#include "framework/utils.h"

#include "RESTObjects/RESTAPI_OWLSobjects.h"

namespace OpenWifi {

    class SimStats : public SubSystemServer {

    public:
        inline void Connect() {
            std::lock_guard G(Mutex_);
            Status_.liveDevices++;

            if( (Status_.timeToFullDevices == 0) && (Status_.liveDevices == ExpectedDevices_) ) {
                uint64_t Now = Utils::Now();
                Status_.timeToFullDevices = Now - Status_.startTime;
            }
        }

        inline void Disconnect() {
            if(CollectInfo_) {
                std::lock_guard G(Mutex_);
                if(Status_.liveDevices)
                    Status_.liveDevices--;
            }
        }

        static auto instance() {
            static auto instance_ = new SimStats;
            return instance_;
        }

        inline void AddRX(uint64_t N) {
            if(CollectInfo_) {
                std::lock_guard G(Mutex_);
                Status_.rx += N;
            }
        }

        inline void AddOutMsg() {
            if(CollectInfo_) {
                std::lock_guard G(Mutex_);
                Status_.msgsTx++;
            }
        }

        inline void AddInMsg() {
            if(CollectInfo_) {
                std::lock_guard G(Mutex_);
                Status_.msgsRx++;
            }
        }

        inline void AddTX(uint64_t N) {
            if(CollectInfo_) {
                std::lock_guard G(Mutex_);
                Status_.tx += N;
            }
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

        inline void StartSim(const std::string &id , const std::string & simid, uint64_t Devices, const std::string & owner) {
            std::lock_guard G(Mutex_);
            CollectInfo_ = true;
            ExpectedDevices_ = Devices;
            Status_.id = id;
            Status_.simulationId = simid;
            Status_.state = "running";
            Status_.liveDevices = Status_.endTime = Status_.rx =
            Status_.tx = Status_.msgsTx = Status_.msgsRx = Status_.timeToFullDevices =
            Status_.errorDevices = 0;
            Status_.startTime = Utils::Now();
            Status_.owner = owner;
        }

        inline void EndSim() {
            std::lock_guard G(Mutex_);
            CollectInfo_ = false;
            Status_.state = "completed";
            Status_.endTime = Utils::Now();
        }

        inline void SetState(const std::string &S) {
            std::lock_guard G(Mutex_);
            Status_.state = S;
        }

        [[nodiscard]] inline const std::string & GetState() {
            std::lock_guard G(Mutex_);
            return Status_.state;
        }

        [[nodiscard]] inline const std::string & Id() const {
            return Status_.id;
        }

        inline void Reset() {
            ExpectedDevices_ = Status_.liveDevices = Status_.tx = Status_.msgsRx = Status_.msgsTx = Status_.rx =
                    Status_.endTime = Status_.errorDevices = Status_.startTime = Status_.timeToFullDevices = 0;
            Status_.simulationId = Status_.id = Status_.state = "";
        }

        [[nodiscard]] inline uint64_t GetStartTime() const { return Status_.startTime; }

        [[nodiscard]] inline uint64_t GetLiveDevices() const { return Status_.liveDevices; }

    private:
        OWLSObjects::SimulationStatus   Status_;
        uint64_t                        ExpectedDevices_=0;
        std::atomic_bool                CollectInfo_=false;

        SimStats() noexcept:
        SubSystemServer("SimStats", "SIM-STATS", "stats")
            {
            }
    };

    inline SimStats * SimStats() { return SimStats::instance(); }
}

