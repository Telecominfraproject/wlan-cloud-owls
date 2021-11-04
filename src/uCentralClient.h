//
// Created by stephane bourque on 2021-03-12.
//

#ifndef UCENTRAL_CLNT_UCENTRALCLIENT_H
#define UCENTRAL_CLNT_UCENTRALCLIENT_H

#include <mutex>

#include "Poco/Thread.h"
#include "Poco/Net/SocketReactor.h"
#include "Poco/Net/SocketNotification.h"
#include "Poco/AutoPtr.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/Logger.h"
#include "Poco/JSON/Object.h"

#include "uCentralEventTypes.h"

namespace OpenWifi {
    struct CensusReport {
        uint32_t ev_none,
        ev_reconnect,
        ev_connect,
        ev_state,
        ev_healthcheck,
        ev_log,
        ev_crashlog,
        ev_configpendingchange,
        ev_keepalive,
        ev_reboot,
        ev_disconnect,
        ev_wsping;

        void Reset() {
            ev_none = ev_reconnect = ev_connect = ev_state =
                    ev_healthcheck = ev_log = ev_crashlog = ev_configpendingchange =
                            ev_keepalive = ev_reboot = ev_disconnect = ev_wsping = 0 ;
        }
    };

    class uCentralClient {
    public:

        uCentralClient(
                Poco::Net::SocketReactor  & Reactor,
                std::string SerialNumber,
                Poco::Logger & Logger);

        bool Send(const std::string &Cmd);
        bool SendWSPing();
        bool SendObject(Poco::JSON::Object O);
        void OnSocketReadable(const Poco::AutoPtr<Poco::Net::ReadableNotification>& pNf);

        void EstablishConnection();
        void Disconnect(bool Reconnect);

        void ProcessCommand(Poco::DynamicStruct Vars);

        void SetFirmware() { Firmware_ = "sim-firmware-1." + std::to_string(Version_); }

        [[nodiscard]] const std::string & Serial() const { return SerialNumber_; }
        [[nodiscard]] uint64_t UUID() const { return UUID_; }
        [[nodiscard]] uint64_t Active() const { return Active_;}
        [[nodiscard]] const std::string & Firmware() const { return Firmware_; }
        [[nodiscard]] bool Connected() const { return Connected_; }
        [[nodiscard]] inline uint64_t GetStartTime() const { return StartTime_;}

        void AddEvent(uCentralEventType E, uint64_t InSeconds);
        uCentralEventType NextEvent(bool Remove);

        void  DoConfigure(uint64_t Id, Poco::DynamicStruct Params);
        void  DoReboot(uint64_t Id, Poco::DynamicStruct Params);
        void  DoUpgrade(uint64_t Id, Poco::DynamicStruct Params);
        void  DoFactory(uint64_t Id, Poco::DynamicStruct Params);
        void  DoLEDs(uint64_t Id, Poco::DynamicStruct Params);
        void  DoPerform(uint64_t Id, Poco::DynamicStruct Params);
        void  DoTrace(uint64_t Id, Poco::DynamicStruct Params);

        void DoCensus( CensusReport & Census );

    private:
        std::recursive_mutex        Mutex_;
        Poco::Net::SocketReactor    &Reactor_;
        Poco::Logger                &Logger_;
        std::string                 CurrentConfig_;
        std::string                 SerialNumber_;
        std::string                 Firmware_;
        std::unique_ptr<Poco::Net::WebSocket>   WS_;
        uint64_t                    Active_=0;
        uint64_t                    UUID_=0;
        bool                        Connected_=false;
        bool                        KeepRedirector_=false;
        uint64_t                    Version_=0;
        uint64_t                    StartTime_ = std::time(nullptr);

        // outstanding commands are marked with a time and the event itself
        std::map< uint64_t , uCentralEventType >    Commands_;
    };
}

#endif //UCENTRAL_CLNT_UCENTRALCLIENT_H
