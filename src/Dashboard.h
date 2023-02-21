//
// Created by stephane bourque on 2021-07-21.
//

#ifndef UCENTRALGW_DASHBOARD_H
#define UCENTRALGW_DASHBOARD_H

#include "RESTObjects/RESTAPI_OWLSobjects.h"
#include "framework/OpenWifiTypes.h"

namespace OpenWifi {
	class OWLSDashboard {
	  public:
		void Create();
		const OWLSObjects::Dashboard &Report() const { return DB_; }

	  private:
		OWLSObjects::Dashboard DB_;
		uint64_t LastRun_ = 0;
		inline void Reset() { DB_.reset(); }
	};
} // namespace OpenWifi

#endif // UCENTRALGW_DASHBOARD_H
