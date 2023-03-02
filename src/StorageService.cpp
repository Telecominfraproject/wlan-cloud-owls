//
//	License type: BSD 3-Clause License
//	License copy: https://github.com/Telecominfraproject/wlan-cloud-ucentralgw/blob/master/LICENSE
//
//	Created by Stephane Bourque on 2021-03-04.
//	Arilia Wireless Inc.
//

#include "StorageService.h"

namespace OpenWifi {

	int Storage::Start() {
		std::lock_guard Guard(Mutex_);
		Logger().notice("Starting.");

		StorageClass::Start();

		SimulationDB_ = std::make_unique<OpenWifi::SimulationDB>(dbType_, *Pool_, Logger());
		SimulationDB_->Create();
		SimulationResultsDB_ =
			std::make_unique<OpenWifi::SimulationResultsDB>(dbType_, *Pool_, Logger());
		SimulationResultsDB_->Create();

		return 0;
	}

	void Storage::Stop() {
		std::lock_guard Guard(Mutex_);
		Logger().notice("Stopping.");

		StorageClass::Stop();
	}
} // namespace OpenWifi

// namespace