//
// Created by stephane bourque on 2021-07-21.
//

#include <framework/utils.h>
#include <Dashboard.h>

namespace OpenWifi {
	void OWLSDashboard::Create() {
		uint64_t Now = Utils::Now();

		if (LastRun_ == 0 || (Now - LastRun_) > 120) {
			DB_.reset();
			LastRun_ = Now;
		}
	}
} // namespace OpenWifi
