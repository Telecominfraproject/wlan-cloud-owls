//
// Created by stephane bourque on 2021-11-02.
//

#ifndef OWLS_RESTAPI_RESULTS_HANDLER_H
#define OWLS_RESTAPI_RESULTS_HANDLER_H

#include "framework/MicroService.h"

namespace OpenWifi {
    class RESTAPI_results_handler : public RESTAPIHandler {
    public:
        RESTAPI_results_handler(const RESTAPIHandler::BindingMap &bindings, Poco::Logger &L, RESTAPI_GenericServer & Server, bool Internal)
        : RESTAPIHandler(bindings, L,
                         std::vector<std::string>{
            Poco::Net::HTTPRequest::HTTP_GET,
            Poco::Net::HTTPRequest::HTTP_DELETE,
            Poco::Net::HTTPRequest::HTTP_OPTIONS},
            Server,
            Internal) {}
            static const std::list<const char *> PathName() { return std::list<const char *>{"/api/v1/results"};}
        void DoGet() final;
        void DoPost() final {};
        void DoPut() final {};
        void DoDelete() final ;
    private:

    };

}

#endif //OWLS_RESTAPI_RESULTS_HANDLER_H
