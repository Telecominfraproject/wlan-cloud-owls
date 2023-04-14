//
// Created by stephane bourque on 2021-11-03.
//

#pragma once

#include <chrono>
#include <random>

#include "RESTObjects/RESTAPI_OWLSobjects.h"
#include "SimulationRunner.h"
#include "framework/SubSystemServer.h"

namespace OpenWifi {

    struct SimulationRecord {
        SimulationRecord(const OWLSObjects::SimulationDetails & details,Poco::Logger &L, const std::string &id) :
                Runner(details, L, id) {

        }
        std::atomic_bool                SimRunning = false;
        OWLSObjects::SimulationDetails  Details;
        SimulationRunner                Runner;
    };

	class SimulationCoordinator : public SubSystemServer, Poco::Runnable {
	  public:
		static auto instance() {
			static auto instance_ = new SimulationCoordinator;
			return instance_;
		}

		int Start() final;
		void Stop() final;
		void run() final;

		bool StartSim(std::string &SimId, const std::string &Id,
                      std::string &Error, const std::string &Owner);
		bool StopSim(const std::string &Id, std::string &Error);
		bool CancelSim(const std::string &Id, std::string &Error);

		[[nodiscard]] inline bool GetSimulationInfo( OWLSObjects::SimulationDetails & Details , const std::string &uuid = "" ) {
            std::lock_guard G(Mutex_);

            if(Simulations_.empty())
                return false;
            if(uuid.empty()) {
                Details = Simulations_.begin()->second->Details;
                return true;
            }
            auto sim_hint = Simulations_.find(uuid);
            if(sim_hint==end(Simulations_))
                return false;
			Details = sim_hint->second->Details;
            return true;
		}

		[[nodiscard]] inline const std::string &GetCasLocation() { return CASLocation_; }
		[[nodiscard]] inline const std::string &GetCertFileName() { return CertFileName_; }
		[[nodiscard]] inline const std::string &GetKeyFileName() { return KeyFileName_; }
		[[nodiscard]] inline const std::string &GetRootCAFileName() { return RootCAFileName_; }
		[[nodiscard]] inline int GetLevel() const { return Level_; }
		[[nodiscard]] const nlohmann::json &GetSimCapabilities() { return DefaultCapabilities_; }
		[[nodiscard]] nlohmann::json GetSimConfiguration(uint64_t uuid);



	  private:
		Poco::Thread Worker_;
		std::atomic_bool Running_ = false;
		std::map<std::string,std::unique_ptr<SimulationRecord>> Simulations_;
		std::string CASLocation_;
		std::string CertFileName_;
		std::string KeyFileName_;
		std::string RootCAFileName_;
		nlohmann::json DefaultCapabilities_;
		int Level_ = 0;

		SimulationCoordinator() noexcept
			: SubSystemServer("SimulationCoordinator", "SIM-COORDINATOR", "coordinator") {}

		void StopSimulations();
		void CancelSimulations();
	};

	inline auto SimulationCoordinator() {
		return SimulationCoordinator::instance();
	}
} // namespace OpenWifi
