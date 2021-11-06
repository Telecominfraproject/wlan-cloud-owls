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
#include "nlohmann/json.hpp"

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

        struct ClientInfo {
            std::string     mac;
            std::string     ipv4;
            std::string     ipv6;
        };
        typedef std::vector<ClientInfo>   Clients;

        uCentralClient(
                Poco::Net::SocketReactor  & Reactor,
                std::string SerialNumber,
                Poco::Logger & Logger);

        bool Send(const std::string &Cmd);
        bool SendWSPing();
        bool SendObject(nlohmann::json &O);
        void OnSocketReadable(const Poco::AutoPtr<Poco::Net::ReadableNotification>& pNf);
        void EstablishConnection();
        void Disconnect( const char * Reason, bool Reconnect );
        void ProcessCommand(nlohmann::json & Vars);

        void SetFirmware( const std::string & S = "sim-firmware-1" ) { Firmware_ = S; }

        [[nodiscard]] const std::string & Serial() const { return SerialNumber_; }
        [[nodiscard]] uint64_t UUID() const { return UUID_; }
        [[nodiscard]] uint64_t Active() const { return Active_;}
        [[nodiscard]] const std::string & Firmware() const { return Firmware_; }
        [[nodiscard]] bool Connected() const { return Connected_; }
        [[nodiscard]] inline uint64_t GetStartTime() const { return StartTime_;}

        void AddEvent(uCentralEventType E, uint64_t InSeconds);
        uCentralEventType NextEvent(bool Remove);

        void  DoConfigure(uint64_t Id, nlohmann::json & Params);
        void  DoReboot(uint64_t Id, nlohmann::json & Params);
        void  DoUpgrade(uint64_t Id, nlohmann::json & Params);
        void  DoFactory(uint64_t Id, nlohmann::json & Params);
        void  DoLEDs(uint64_t Id, nlohmann::json & Params);
        void  DoPerform(uint64_t Id, nlohmann::json & Params);
        void  DoTrace(uint64_t Id, nlohmann::json & Params);
        void  DoCensus( CensusReport & Census );

        static void CreateClients( Clients & C, uint64_t min, uint64_t max);

        nlohmann::json CreateState();

        Poco::Logger & Logger() { return Logger_; };

    private:
        std::recursive_mutex        Mutex_;
        Poco::Net::SocketReactor    &Reactor_;
        Poco::Logger                &Logger_;
        nlohmann::json              CurrentConfig_;
        std::string                 SerialNumber_;
        std::string                 Firmware_;
        std::unique_ptr<Poco::Net::WebSocket>   WS_;
        uint64_t                    Active_=0;
        uint64_t                    UUID_=0;
        bool                        Connected_=false;
        bool                        KeepRedirector_=false;
        uint64_t                    Version_=0;
        uint64_t                    StartTime_ = std::time(nullptr);
        uint64_t                    NumClients_=0;
        uint64_t                    NumAssociations_=0;
        Clients                     clients_lan, clients_2g, clients_5g;
        std::string                 mac_lan, mac_2g, mac_5g;

        // outstanding commands are marked with a time and the event itself
        std::map< uint64_t , uCentralEventType >    Commands_;
    };
}

#endif //UCENTRAL_CLNT_UCENTRALCLIENT_H
