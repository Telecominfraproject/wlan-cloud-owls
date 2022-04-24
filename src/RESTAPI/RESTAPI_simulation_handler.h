//
// Created by stephane bourque on 2021-11-02.
//

#ifndef OWLS_RESTAPI_SIMULATION_HANDLER_H
#define OWLS_RESTAPI_SIMULATION_HANDLER_H


class RESTAPI_simulation_handler {

};

#include "framework/MicroService.h"

namespace OpenWifi {
    class RESTAPI_simulation_handler : public RESTAPIHandler {
    public:
        RESTAPI_simulation_handler(const RESTAPIHandler::BindingMap &bindings, Poco::Logger &L, RESTAPI_GenericServer & Server, uint64_t TransactionId, bool Internal)
        : RESTAPIHandler(bindings, L,
                         std::vector<std::string>{
            Poco::Net::HTTPRequest::HTTP_POST,
            Poco::Net::HTTPRequest::HTTP_GET,
            Poco::Net::HTTPRequest::HTTP_PUT,
            Poco::Net::HTTPRequest::HTTP_DELETE,
            Poco::Net::HTTPRequest::HTTP_OPTIONS},
            Server,
            TransactionId,
            Internal) {}
        static auto PathName() { return std::list<std::string>{"/api/v1/simulation"};}
        void DoGet() final;
        void DoPost() final;
        void DoPut() final;
        void DoDelete() final;
    private:

    };
}

#endif //OWLS_RESTAPI_SIMULATION_HANDLER_H
