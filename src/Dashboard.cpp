//
// Created by stephane bourque on 2021-07-21.
//

#include "Dashboard.h"

namespace OpenWifi {
    void OWLSDashboard::Create() {
		uint64_t Now = OpenWifi::Now();

		if(LastRun_==0 || (Now-LastRun_)>120) {
			DB_.reset();
			LastRun_ = Now;
		}
	}
}
