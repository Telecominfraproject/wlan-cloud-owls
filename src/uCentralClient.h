//
// Created by stephane bourque on 2021-03-12.
//

#pragma once

#include <mutex>
#include <map>
#include <tuple>
#include <random>

#include "Poco/Thread.h"
#include "Poco/Net/SocketReactor.h"
#include "Poco/Net/SocketNotification.h"
#include "Poco/AutoPtr.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/Logger.h"
#include "Poco/JSON/Object.h"

#include "framework/utils.h"

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

    enum class radio_bands {
                        band_2g,
                        band_5g,
                        band_6g
    };

    enum ap_interface_types {
        upstream,
        downstream
    };

    inline std::int64_t local_random(std::int64_t min, std::int64_t max) {
        static std::random_device          rd;
        static std::mt19937_64             gen{rd()};
        if(min>max)
            std::swap(min,max);
        std::uniform_int_distribution<> dis(min, max);
        return dis(gen);
    }

    inline auto local_random(std::int64_t max) {
        return local_random(0,max);
    }

    struct FakeLanClient {
        std::vector<std::string>    ipv6_addresses,
                                    ipv4_addresses,
                                    ports;
        std::string                 mac;

        [[nodiscard]] nlohmann::json to_json() const {
            nlohmann::json res;
            res["ipv4_addresses"] = ipv4_addresses;
            res["ipv6_addresses"] = ipv6_addresses;
            res["ports"] = ports;
            res["mac"] = mac;
            return res;
        }

        void next() {

        }
    };
    typedef std::vector<FakeLanClient>   FakeLanClients;

    struct FakeAssociation {
        std::string     station;
        std::string     bssid;
        std::string     ipaddr_v4;
        std::string     ipaddr_v6;
        uint64_t        tx_bytes=0,
                        rx_bytes=0,
                        rx_duration=0,
                        rx_packets=0,
                        connected=0,
                        inactive=0,
                        tx_duration=0,
                        tx_failed=0,
                        tx_packets=0,
                        tx_retries=0;
        int64_t         ack_signal=0,
                        ack_signal_avg=local_random(-65,-75),
                        rssi=local_random(-40,-90);

        [[nodiscard]] nlohmann::json  to_json() const {
            nlohmann::json res;
            res["ack_signal"] = ack_signal;
            res["ack_signal_avg"] = ack_signal_avg;
            res["bssid"] = bssid;
            res["station"] = station;
            res["connected"] = connected;
            res["inactive"] = inactive;
            res["ipaddr_v4"] = ipaddr_v4;
            res["rssi"] = rssi;
            res["rx_bytes"] = rx_bytes;
            res["rx_duration"] = rx_duration;
            res["rx_packets"] = rx_packets;
            res["rx_rate"]["bitrate"] = 200000;
            res["rx_rate"]["chwidth"] = 40;
            res["rx_rate"]["mcs"] = 9;
            res["rx_rate"]["nss"] = 9;
            res["rx_rate"]["sgi"] = true;
            res["rx_rate"]["vht"] = true;

            res["tx_bytes"] = tx_bytes;
            res["tx_duration"] = tx_duration;
            res["tx_failed"] = tx_failed;
            res["tx_packets"] = tx_packets;
            res["tx_retries"] = tx_retries;

            res["tx_rate"]["bitrate"] = 200000;
            res["tx_rate"]["chwidth"] = 40;
            res["tx_rate"]["mcs"] = 9;
            res["tx_rate"]["sgi"] = true;
            res["tx_rate"]["ht"] = true;

            nlohmann::json tid_stats;
            nlohmann::json tid_stat;
            tid_stat["rx_msdu"] = 0;
            tid_stat["tx_msdu"] = 0;
            tid_stat["tx_msdu_failed"] = 0;
            tid_stat["tx_msdu_retries"] = 0;
            tid_stats.push_back(tid_stat);
            tid_stats.push_back(tid_stat);
            tid_stats.push_back(tid_stat);
            tid_stats.push_back(tid_stat);
            tid_stats.push_back(tid_stat);

            res["tid_stats"] = tid_stats;

            return res;
        }

        void next() {
            ack_signal = ack_signal_avg + local_random(-5,5);
            connected += local_random(100,500);
            inactive += local_random(100,500);
            rssi += local_random(-2,2);

            auto new_rx_packets = local_random(200,2000);
            rx_packets += new_rx_packets;
            rx_bytes += new_rx_packets * local_random(500,1000);
            rx_duration += local_random(400,1750);

            auto new_tx_packets = local_random(100,300);
            tx_packets += new_tx_packets;
            tx_bytes += new_tx_packets * local_random(500,1000);
            tx_duration += local_random(400,1750);
            tx_failed += local_random(3) * local_random(800,1200);
            tx_retries += local_random(3);
        }

        void reset() {
            ack_signal = ack_signal_avg ;
            connected = 0;
            inactive = 0;

            rx_packets = 0;
            rx_bytes = 0;
            rx_duration = 0;

            tx_packets = 0;
            tx_bytes = 0;
            tx_duration = 0;
            tx_failed = 0;
            tx_retries = 0;
        }

    };
    typedef std::vector<FakeAssociation>   FakeAssociations;

    struct FakeRadio {
        std::uint64_t   active_ms=0,
                        busy_ms=0,
                        channel=0,
                        channel_width=40,
                        receive_ms=0,
                        transmit_ms=0,
                        tx_power=23;
        std::int64_t    noise=-100,
                        temperature=40;
        std::string     phy;
        uint64_t        index=0;

        [[nodiscard]] nlohmann::json to_json() const {
            nlohmann::json res;
            res["active_ms"] = active_ms;
            res["busy_ms"] = busy_ms;
            res["receive_ms"] = receive_ms;
            res["transmit_ms"] = transmit_ms;
            res["noise"] = noise;
            res["temperature"] = temperature;
            res["channel"] = channel;
            res["channel_width"] = std::to_string(channel_width);
            res["tx_power"] = tx_power;
            res["phy"] = phy;
            return res;
        }

        void next() {
            temperature = 40 + local_random(-7,7);
            noise = -95 + local_random(-3,3);
            active_ms += local_random(100,2000);
            busy_ms += local_random(200,3000);
            receive_ms += local_random(500,1500);
            transmit_ms += local_random(250,100);
        }

        void reset() {
            active_ms = 0;
            busy_ms = 0;
            receive_ms = 0;
            transmit_ms = 0;
        }

    };

    struct FakeCounters {
        std::uint64_t   collisions=0,
                multicast=0,
                rx_bytes=0,
                rx_dropped=0,
                rx_errors=0,
                rx_packets=0,
                tx_bytes=0,
                tx_dropped=0,
                tx_errors=0,
                tx_packets=0;

        [[nodiscard]] nlohmann::json to_json() const {
            nlohmann::json res;

            res["collisions"] = collisions;
            res["multicast"] = multicast;
            res["rx_bytes"] = rx_bytes;
            res["rx_dropped"] = rx_dropped;
            res["rx_errors"] = rx_errors;
            res["rx_packets"] = rx_packets;
            res["tx_bytes"] = tx_bytes;
            res["tx_dropped"] = tx_dropped;
            res["tx_errors"] = tx_errors;
            res["tx_packets"] = tx_packets;

            return res;
        }

        void next() {
            multicast += local_random(10,100);

            collisions += local_random(1);
            rx_dropped += local_random(2);
            rx_errors += local_random(3);
            auto new_rx_packets = local_random(300,2000);
            rx_packets += new_rx_packets;
            rx_bytes += new_rx_packets * local_random(900,1400);

            tx_dropped += local_random(2);
            tx_errors += local_random(3);
            auto new_tx_packets = local_random(300,2000);
            tx_packets += new_tx_packets;
            tx_bytes += new_tx_packets * local_random(900,1400);
        }

        void reset() {
            multicast =0;
            collisions =0;
            rx_dropped =0;
            rx_errors =0;
            rx_packets =0;
            rx_bytes =0;
            tx_dropped =0;
            tx_errors =0;
            tx_packets =0;
            tx_bytes =0;
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

        static FakeAssociations CreateAssociations(const std::string &bssid, uint64_t min, uint64_t max);
        static FakeLanClients CreateLanClients(uint64_t min, uint64_t max);

        nlohmann::json CreateState();
        nlohmann::json CreateLinkState();

        Poco::Logger & Logger() { return Logger_; };

        [[nodiscard]] uint64_t GetStateInterval() { return StatisticsInterval_; }
        [[nodiscard]] uint64_t GetHealthInterval() { return HealthInterval_; }

        void UpdateConfiguration();

        bool FindInterfaceRole(const std::string &role, ap_interface_types & interface);

        void Reset();

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
        uint64_t                    StartTime_ = Utils::Now();
        std::string                 mac_lan;
        std::atomic_uint64_t        HealthInterval_=60;
        std::atomic_uint64_t        StatisticsInterval_=60;
        uint64_t                    bssid_index=1;

        using interface_location = std::tuple<ap_interface_types, std::string, radio_bands>;

        FakeLanClients                                      AllLanClients_;
        std::map<interface_location, FakeAssociations >     AllAssociations_;
        std::map<radio_bands,FakeRadio>                     AllRadios_;
        std::map<ap_interface_types,FakeCounters>           AllCounters_;
        std::map<ap_interface_types,std::string>            AllInterfaceNames_;
        std::map<ap_interface_types,std::string>            AllInterfaceRoles_;
        std::map<ap_interface_types,std::string>            AllPortNames_;


        // outstanding commands are marked with a time and the event itself
        std::map< uint64_t , uCentralEventType >    Commands_;
    };
}

