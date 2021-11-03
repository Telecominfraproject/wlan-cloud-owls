//
//	License type: BSD 3-Clause License
//	License copy: https://github.com/Telecominfraproject/wlan-cloud-ucentralgw/blob/master/LICENSE
//
//	Created by Stephane Bourque on 2021-03-04.
//	Arilia Wireless Inc.
//

#ifndef UCENTRAL_UCENTRAL_H
#define UCENTRAL_UCENTRAL_H

#include "Dashboard.h"
#include "framework/MicroService.h"

namespace OpenWifi {

	static const char * vDAEMON_PROPERTIES_FILENAME = "owls.properties";
	static const char * vDAEMON_ROOT_ENV_VAR = "OWLS_ROOT";
	static const char * vDAEMON_CONFIG_ENV_VAR = "OWLS_CONFIG";
	static const char * vDAEMON_APP_NAME = uSERVICE_OWLS.c_str();
	static const uint64_t vDAEMON_BUS_TIMER = 10000;

    class Daemon : public MicroService {
		public:
			explicit Daemon(const std::string & PropFile,
							const std::string & RootEnv,
							const std::string & ConfigEnv,
							const std::string & AppName,
						  	uint64_t 	BusTimer,
							const SubSystemVec & SubSystems) :
				MicroService( PropFile, RootEnv, ConfigEnv, AppName, BusTimer, SubSystems) {};

			void initialize();
			static Daemon *instance();
			inline OWLSDashboard	& GetDashboard() { return DB_; }
	  	private:
			static Daemon 				*instance_;
			bool                        AutoProvisioning_ = false;
			Types::StringMapStringSet   DeviceTypeIdentifications_;
			OWLSDashboard				DB_{};
    };

	inline Daemon * Daemon() { return Daemon::instance(); }
}

#endif //UCENTRAL_UCENTRAL_H
