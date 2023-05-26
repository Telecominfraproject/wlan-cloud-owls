//
// Created by stephane bourque on 2021-04-07.
//

#pragma once

#include <framework/SubSystemServer.h>
#include <framework/utils.h>
#include <RESTObjects/RESTAPI_OWLSobjects.h>
#include <RESTObjects/RESTAPI_SecurityObjects.h>

namespace OpenWifi {

	class SimStats : public SubSystemServer {

	  public:
		inline void Connect(const std::string &id) {
			std::lock_guard G(Mutex_);

            auto stats_hint = Status_.find(id);
            if(stats_hint==end(Status_)) {
                return;
            }
            stats_hint->second.liveDevices++;

			if ((stats_hint->second.timeToFullDevices == 0) && (stats_hint->second.liveDevices == stats_hint->second.expectedDevices)) {
				uint64_t Now = Utils::Now();
                stats_hint->second.timeToFullDevices = Now - stats_hint->second.startTime;
			}
		}

		inline void Disconnect(const std::string &id) {
            std::lock_guard G(Mutex_);

            auto stats_hint = Status_.find(id);
            if(stats_hint==end(Status_)) {
                return;
            }

            if (stats_hint->second.liveDevices)
                stats_hint->second.liveDevices--;
		}

		static auto instance() {
			static auto instance_ = new SimStats;
			return instance_;
		}

		inline void AddOutMsg(const std::string &id, int64_t N) {
            std::lock_guard G(Mutex_);
            auto stats_hint = Status_.find(id);
            if(stats_hint==end(Status_)) {
                return;
            }
            stats_hint->second.msgsTx++;
            stats_hint->second.tx += N;
		}

		inline void AddInMsg(const std::string &id, int64_t N) {
            std::lock_guard G(Mutex_);
            auto stats_hint = Status_.find(id);
            if(stats_hint==end(Status_)) {
                return;
            }
            stats_hint->second.rx += N;
            stats_hint->second.msgsRx++;
		}

		inline void GetCurrent(const std::string &id, OWLSObjects::SimulationStatus &S,
                               const SecurityObjects::UserInfo & UInfo) {
			std::lock_guard G(Mutex_);
            auto stats_hint = Status_.find(id);
            if(stats_hint==end(Status_)) {
                return;
            }
            if(UInfo.userRole==SecurityObjects::ROOT || UInfo.email==stats_hint->second.owner)
                S = stats_hint->second;
		}

		inline int Start() final {
			return 0;
		}

		inline void Stop() {

        }

		inline void StartSim(const std::string &id, const std::string &simid, uint64_t Devices,
                             const SecurityObjects::UserInfo & UInfo) {
			std::lock_guard G(Mutex_);
            auto & CurrentStatus = Status_[id];

			CurrentStatus.expectedDevices = Devices;
            CurrentStatus.id = id;
            CurrentStatus.simulationId = simid;
            CurrentStatus.state = "running";
            CurrentStatus.liveDevices = CurrentStatus.endTime = CurrentStatus.rx = CurrentStatus.tx = CurrentStatus.msgsTx =
            CurrentStatus.msgsRx = CurrentStatus.timeToFullDevices = CurrentStatus.errorDevices = 0;
            CurrentStatus.startTime = Utils::Now();
            CurrentStatus.owner = UInfo.email;
		}

		inline void EndSim(const std::string &id) {
            std::lock_guard G(Mutex_);
            auto stats_hint = Status_.find(id);
            if(stats_hint==end(Status_)) {
                return;
            }
			stats_hint->second.state = "completed";
            stats_hint->second.endTime = Utils::Now();
		}

        inline void RemoveSim(const std::string &id) {
            std::lock_guard G(Mutex_);
            Status_.erase(id);
        }

		inline void SetState(const std::string &id, const std::string &S) {
            std::lock_guard G(Mutex_);
            auto stats_hint = Status_.find(id);
            if(stats_hint==end(Status_)) {
                return;
            }
            stats_hint->second.state = S;
		}

		[[nodiscard]] inline std::string GetState(const std::string &id) {
            std::lock_guard G(Mutex_);
            auto stats_hint = Status_.find(id);
            if(stats_hint==end(Status_)) {
                return "";
            }
			return stats_hint->second.state;
		}

		inline void Reset(const std::string &id) {
            std::lock_guard G(Mutex_);
            auto stats_hint = Status_.find(id);
            if(stats_hint==end(Status_)) {
                return;
            }

            stats_hint->second.liveDevices =
            stats_hint->second.rx =
            stats_hint->second.tx =
            stats_hint->second.msgsRx =
            stats_hint->second.msgsTx =
            stats_hint->second.errorDevices =
            stats_hint->second.startTime =
            stats_hint->second.endTime = 0;
            stats_hint->second.state = "idle";
		}

		[[nodiscard]] inline uint64_t GetStartTime(const std::string &id) {
            std::lock_guard G(Mutex_);
            auto stats_hint = Status_.find(id);
            if(stats_hint==end(Status_)) {
                return 0;
            }
            return stats_hint->second.startTime;
        }

		[[nodiscard]] inline uint64_t GetLiveDevices(const std::string &id) {
            std::lock_guard G(Mutex_);
            auto stats_hint = Status_.find(id);
            if(stats_hint==end(Status_)) {
                return 0;
            }
            return stats_hint->second.liveDevices;
        }

        inline void GetAllSimulations(std::vector<OWLSObjects::SimulationStatus> & Statuses, const SecurityObjects::UserInfo & UInfo) {
            Statuses.clear();

            std::lock_guard G(Mutex_);

            for(const auto &[id,status]:Status_) {
                if(UInfo.userRole==SecurityObjects::ROOT || UInfo.email==status.owner) {
                    Statuses.emplace_back(status);
                }
            }
        }

	  private:
        std::map<std::string,OWLSObjects::SimulationStatus>     Status_;

		SimStats() noexcept : SubSystemServer("SimStats", "SIM-STATS", "stats") {}
	};

	inline auto SimStats() { return SimStats::instance(); }
} // namespace OpenWifi
