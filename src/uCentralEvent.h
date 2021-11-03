//
// Created by stephane bourque on 2021-04-03.
//

#ifndef UCENTRAL_CLNT_UCENTRALEVENT_H
#define UCENTRAL_CLNT_UCENTRALEVENT_H

#include "uCentralClient.h"
#include "Poco/JSON/Object.h"

#include "uCentralEventTypes.h"
#include "uCentralClient.h"

namespace OpenWifi {
    class uCentralEvent {
    public:
        explicit uCentralEvent(std::shared_ptr<uCentralClient> Client ) : Client_(std::move(Client)) {}
        virtual bool Send() = 0;

    protected:
        bool SendObject( Poco::JSON::Object & Obj);
        std::shared_ptr<uCentralClient> Client_;
    };

    class ConnectEvent : public uCentralEvent {
    public:
        explicit ConnectEvent(std::shared_ptr<uCentralClient> Client )
        :uCentralEvent(std::move(Client)) {};
        bool Send() override;
    private:
    };

    class StateEvent : public uCentralEvent {
    public:
        explicit StateEvent(std::shared_ptr<uCentralClient> Client )
        :uCentralEvent(std::move(Client)) {};
        bool Send() override;
    private:
    };

    class HealthCheckEvent : public uCentralEvent {
    public:
        explicit HealthCheckEvent(std::shared_ptr<uCentralClient> Client )
        :uCentralEvent(std::move(Client)) {};
        bool Send() override;
    private:
    };

    class LogEvent : public uCentralEvent {
    public:
        LogEvent(std::shared_ptr<uCentralClient> Client,std::string LogLine, uint64_t Severity)
        : LogLine_(std::move(LogLine)), Severity_(Severity), uCentralEvent(std::move(Client)) {};
        bool Send() override;
    private:
        std::string     LogLine_;
        uint64_t        Severity_;
    };

    class CrashLogEvent : public uCentralEvent {
    public:
        explicit CrashLogEvent(std::shared_ptr<uCentralClient> Client )
        :uCentralEvent(std::move(Client)) {};
        bool Send() override;
    private:
    };

    class ConfigChangePendingEvent : public uCentralEvent {
    public:
        explicit ConfigChangePendingEvent(std::shared_ptr<uCentralClient> Client )
        :uCentralEvent(std::move(Client)) {};
        bool Send() override;
    private:
    };

    class KeepAliveEvent : public uCentralEvent {
    public:
        explicit KeepAliveEvent(std::shared_ptr<uCentralClient> Client )
        :uCentralEvent(std::move(Client)) {};
        bool Send() override;
    private:
    };

    class RebootEvent : public uCentralEvent {
    public:
        explicit RebootEvent(std::shared_ptr<uCentralClient> Client )
        :uCentralEvent(std::move(Client)) {};
        bool Send() override;
    private:
    };

    class DisconnectEvent : public uCentralEvent {
    public:
        explicit DisconnectEvent(std::shared_ptr<uCentralClient> Client )
        :uCentralEvent(std::move(Client)) {};
        bool Send() override;
    private:
    };

    class WSPingEvent : public uCentralEvent {
    public:
        explicit WSPingEvent(std::shared_ptr<uCentralClient> Client )
        :uCentralEvent(std::move(Client)) {};
        bool Send() override;
    private:
    };

}

#endif //UCENTRAL_CLNT_UCENTRALEVENT_H
