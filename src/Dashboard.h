//
// Created by stephane bourque on 2021-07-21.
//

#ifndef UCENTRALGW_DASHBOARD_H
#define UCENTRALGW_DASHBOARD_H

#include "OpenWifiTypes.h"
#include "RESTAPI_OWLSobjects.h"

namespace OpenWifi {
	class OWLSDashboard {
	  public:
			void Create();
			const OWLSObjects::Dashboard & Report() const { return DB_;}
	  private:
	        OWLSObjects::Dashboard 	DB_;
			uint64_t 				LastRun_=0;
			inline void Reset() { DB_.reset(); }
	};
}

#endif // UCENTRALGW_DASHBOARD_H
