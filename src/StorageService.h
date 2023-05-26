//
//	License type: BSD 3-Clause License
//	License copy: https://github.com/Telecominfraproject/wlan-cloud-ucentralgw/blob/master/LICENSE
//
//	Created by Stephane Bourque on 2021-03-04.
//	Arilia Wireless Inc.
//

#pragma once

#include <framework/StorageClass.h>
#include <storage/storage_results.h>
#include <storage/storage_simulations.h>

namespace OpenWifi {

	class Storage : public StorageClass {
	  public:
		static Storage *instance() {
			static auto *instance_ = new Storage;
			return instance_;
		}

		OpenWifi::SimulationDB &SimulationDB() { return *SimulationDB_; }
		OpenWifi::SimulationResultsDB &SimulationResultsDB() { return *SimulationResultsDB_; }

		int Start() override;
		void Stop() override;

	  private:
		std::unique_ptr<OpenWifi::SimulationDB> SimulationDB_;
		std::unique_ptr<OpenWifi::SimulationResultsDB> SimulationResultsDB_;
	};

	inline class Storage *StorageService() { return Storage::instance(); }

} // namespace OpenWifi
