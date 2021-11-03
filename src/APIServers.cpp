//
// Created by stephane bourque on 2021-10-23.
//

#include "framework/MicroService.h"
#include "RESTAPI/RESTAPI_deviceDashboardHandler.h"

namespace OpenWifi {

    Poco::Net::HTTPRequestHandler * RESTAPI_external_server(const char *Path, RESTAPIHandler::BindingMap &Bindings,
                                                            Poco::Logger & L, RESTAPI_GenericServer & S) {
        return  RESTAPI_Router<
                    RESTAPI_system_command,
                    RESTAPI_deviceDashboardHandler
                >(Path,Bindings,L, S);
    }

    Poco::Net::HTTPRequestHandler * RESTAPI_internal_server(const char *Path, RESTAPIHandler::BindingMap &Bindings,
                                                            Poco::Logger & L, RESTAPI_GenericServer & S) {
        return RESTAPI_Router_I<
                RESTAPI_system_command
            >(Path, Bindings, L, S);
    }
}