//
// Created by stephane bourque on 2021-06-29.
//

#include "RESTAPI_InternalServer.h"

#include "Poco/URI.h"

#include "Utils.h"
#include "RESTAPI_handler.h"

namespace OpenWifi {

    class RESTAPI_InternalServer *RESTAPI_InternalServer::instance_ = nullptr;

    RESTAPI_InternalServer::RESTAPI_InternalServer() noexcept: SubSystemServer("RESTAPIInternalServer", "REST-ISRV", "ucentral.internal.restapi")
    {
    }

    int RESTAPI_InternalServer::Start() {
        Logger_.information("Starting.");

        for(const auto & Svr: ConfigServersList_) {
            Logger_.information(Poco::format("Starting: %s:%s Keyfile:%s CertFile: %s", Svr.Address(), std::to_string(Svr.Port()),
                                             Svr.KeyFile(),Svr.CertFile()));

            auto Sock{Svr.CreateSecureSocket(Logger_)};

            Svr.LogCert(Logger_);
            if(!Svr.RootCA().empty())
                Svr.LogCas(Logger_);
            auto Params = new Poco::Net::HTTPServerParams;
            Params->setMaxThreads(50);
            Params->setMaxQueued(200);
            Params->setKeepAlive(true);

            auto NewServer = std::make_unique<Poco::Net::HTTPServer>(new InternalRequestHandlerFactory, Pool_, Sock, Params);
            NewServer->start();
            RESTServers_.push_back(std::move(NewServer));
        }

        return 0;
    }

    void RESTAPI_InternalServer::Stop() {
        Logger_.information("Stopping ");
        for( const auto & svr : RESTServers_ )
            svr->stop();
    }

    Poco::Net::HTTPRequestHandler *InternalRequestHandlerFactory::createRequestHandler(const Poco::Net::HTTPServerRequest & Request) {

        Logger_.debug(Poco::format("REQUEST(%s): %s %s", Utils::FormatIPv6(Request.clientAddress().toString()), Request.getMethod(), Request.getURI()));

        Poco::URI uri(Request.getURI());
        const auto & Path = uri.getPath();
        RESTAPIHandler::BindingMap Bindings;

        // return RESTAPI_Router_I<RESTAPI_BlackList>(Path,Bindings,Logger_);    }
        return nullptr;
    }
}