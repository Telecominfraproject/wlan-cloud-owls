//
// Created by stephane bourque on 2021-07-21.
//

#ifndef UCENTRALGW_RESTAPI_DEVICEDASHBOARDHANDLER_H
#define UCENTRALGW_RESTAPI_DEVICEDASHBOARDHANDLER_H

#include "framework/MicroService.h"

namespace OpenWifi {
    class RESTAPI_deviceDashboardHandler : public RESTAPIHandler {
        public:
        RESTAPI_deviceDashboardHandler(const RESTAPIHandler::BindingMap &bindings, Poco::Logger &L, RESTAPI_GenericServer & Server, uint64_t TransactionId, bool Internal)
                : RESTAPIHandler(bindings, L,
                                 std::vector<std::string>{
                                     Poco::Net::HTTPRequest::HTTP_GET, Poco::Net::HTTPRequest::HTTP_POST,
                                     Poco::Net::HTTPRequest::HTTP_OPTIONS},
                                     Server,
                                     TransactionId,
                                     Internal) {}
            static const std::list<const char *> PathName() { return std::list<const char *>{"/api/v1/owlsDashboard"};}
            void DoGet();
            void DoPost() {};
            void DoPut() {};
            void DoDelete() {};
        private:

    };
}

#endif // UCENTRALGW_RESTAPI_DEVICEDASHBOARDHANDLER_H
