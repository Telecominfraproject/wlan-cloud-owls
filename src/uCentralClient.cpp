//
// Created by stephane bourque on 2021-03-12.
//
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <sys/time.h>

#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/URI.h"
#include "Poco/Net/Context.h"
#include "Poco/NObserver.h"
#include "Poco/Net/SSLException.h"

#include "uCentralClient.h"
#include "SimStats.h"
#include "Simulation.h"
#include <nlohmann/json.hpp>

using namespace std::chrono_literals;

namespace OpenWifi {

    static std::string MakeMac( const char * S, int offset) {
        char b[256];

        int j=0,i=0;
        for(int k=0;k<6;++k) {
            b[j++] = S[i++];
            b[j++] = S[i++];
            b[j++] = ':';
        }
        b[--j] = 0;
        b[--j] = '0'+offset;
        return b;
    }

    uCentralClient::uCentralClient(
            Poco::Net::SocketReactor  & Reactor,
            std::string SerialNumber,
            Poco::Logger & Logger):
            Logger_(Logger),
            Reactor_(Reactor),
            SerialNumber_(std::move(SerialNumber))
            {
                SetFirmware();
                Active_ = UUID_ = std::time(nullptr);
                srand(UUID_);
                CreateClients( clients_lan, SimulationCoordinator()->GetSimulationInfo().minClients, SimulationCoordinator()->GetSimulationInfo().maxClients);
                CreateClients( clients_2g, SimulationCoordinator()->GetSimulationInfo().minAssociations, SimulationCoordinator()->GetSimulationInfo().maxAssociations);
                CreateClients( clients_5g, SimulationCoordinator()->GetSimulationInfo().minAssociations, SimulationCoordinator()->GetSimulationInfo().maxAssociations);

                mac_lan = MakeMac(SerialNumber_.c_str(), 0 );
                mac_2g = MakeMac(SerialNumber_.c_str(), 1 );
                mac_5g = MakeMac(SerialNumber_.c_str(), 2 );
            }

    static std::string RandomMAC() {
        char b[64];
        sprintf(b,"%02x:%02x:%02x:%02x:%02x:%02x",  (int)MicroService::instance().Random(255),
                (int)MicroService::instance().Random(255),
                (int)MicroService::instance().Random(255),
                (int)MicroService::instance().Random(255),
                (int)MicroService::instance().Random(255),
                (int)MicroService::instance().Random(255) );
        return b;
    }

    static std::string RandomIPv4() {
        char b[64];
        sprintf(b,"%d.%d.%d.%d",
                (int)MicroService::instance().Random(255),
                (int)MicroService::instance().Random(255),
                (int)MicroService::instance().Random(255),
                (int)MicroService::instance().Random(255));
        return b;
    }

    static std::string RandomIPv6() {
        char b[128];
        sprintf(b,"%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
                (uint)MicroService::instance().Random(0x0ffff),
                (uint)MicroService::instance().Random(0x0ffff),
                (uint)MicroService::instance().Random(0x0ffff),
                (uint)MicroService::instance().Random(0x0ffff),
                (uint)MicroService::instance().Random(0x0ffff),
                (uint)MicroService::instance().Random(0x0ffff),
                (uint)MicroService::instance().Random(0x0ffff),
                (uint)MicroService::instance().Random(0x0ffff) );
        return b;
    }

    void uCentralClient::CreateClients(Clients &C, uint64_t min, uint64_t max) {
        uint64_t Num = MicroService::instance().Random(min,max);
        for(auto i=0;i<Num;i++) {
            ClientInfo  CI{ .mac = RandomMAC(), .ipv4 = RandomIPv4(), .ipv6 = RandomIPv6() };
            C.push_back(CI);
        }
    }

    static void AddClients( nlohmann::json & ClientArray , const uCentralClient::Clients & clients) {
        for(const auto &i:clients) {
            nlohmann::json c;
            c["ipv6_addresses"].push_back(i.ipv6);
            c["ipv4_addresses"].push_back(i.ipv4);
            c["mac"] = i.mac;
            c["ports"].push_back("eth1");
            ClientArray.push_back(c);
        }
    }

    static void AddCounters(nlohmann::json & d) {
        d["counters"]["collisions"] = 0 ;
        d["counters"]["multicast"] =  MicroService::instance().Random(30);
        d["counters"]["rx_bytes"] = MicroService::instance().Random(25000);
        d["counters"]["rx_dropped"] = 0 ;
        d["counters"]["rx_errors"] = 0 ;
        d["counters"]["rx_packets"] = MicroService::instance().Random(200);
        d["counters"]["tx_bytes"] = MicroService::instance().Random(5000);
        d["counters"]["tx_dropped"] = MicroService::instance().Random(7);
        d["counters"]["tx_errors"] = MicroService::instance().Random(3);
        d["counters"]["tx_packets"] = MicroService::instance().Random(50);
    }

    static void AddAssociations(const uCentralClient::Clients & C, nlohmann::json & J) {
        nlohmann::json Arr;
        for(const auto &i:C) {
            nlohmann::json a;

            a["bssid"] = i.mac;
            a["station"] = i.mac;
            a["connected"] = 7437;
            a["inactive"] = 19;
            a["rssi"] = -60;
            a["rx_bytes"] = MicroService::instance().Random(5000,100000);
            a["rx_packets"] = MicroService::instance().Random(50,600);
            a["tx_bytes"] = MicroService::instance().Random(100,20000);
            a["tx_duration"] = 36;
            a["tx_failed"] = 0;
            a["tx_offset"] = 0;
            a["tx_packets"] = MicroService::instance().Random(200);
            a["tx_retries"] = 0;
            a["rx_rate"]["bitrate"] = 162000;
            a["rx_rate"]["chwidth"] = 40;
            a["rx_rate"]["mcs"] = 8;
            a["rx_rate"]["nss"] = 1;
            a["rx_rate"]["sgi"] = true;
            a["rx_rate"]["vht"] = true;
            a["tx_rate"]["bitrate"] = 200000;
            a["tx_rate"]["chwidth"] = 40;
            a["tx_rate"]["mcs"] = 8;
            a["tx_rate"]["nss"] = 1;
            a["tx_rate"]["sgi"] = true;
            a["tx_rate"]["vht"] = true;

            Arr.push_back(a);
        }
        J["associations"] = Arr;
    }

    nlohmann::json uCentralClient::CreateState() {
        nlohmann::json S;
        uint64_t    total_mem = 973139968;

        // unit
        uint64_t    Now = std::time(nullptr);
        S["unit"]["load"] = std::vector<double>{ (double)(MicroService::instance().Random(75)) /100.0 , (double)(MicroService::instance().Random(50))/100.0 , (double)(MicroService::instance().Random(25))/100.0 };
        S["unit"]["localtime"] = Now;
        S["unit"]["uptime"] = Now - StartTime_;
        S["unit"]["memory"]["total"] = 973139968;
        S["unit"]["memory"]["buffered"] = 10129408;
        S["unit"]["memory"]["cached"] = 29233152;
        S["unit"]["memory"]["free"] = 760164352;

        S["radios"] = {
                {
                    {"active_ms", 7440626},
                    {"busy_ms", 179311},
                    {"channel", 157},
                    {"channel_width", "40"},
                    {"noise", -104},
                    {"phy", "platform/soc/c000000.wifi"},
                    {"receive_ms", 965},
                    {"temperature", 40},
                    {"transmit_ms", 26540},
                    {"tx_power", 25}
                },
                {
                    {"active_ms", 7439624},
                    {"busy_ms", 710792},
                    {"channel", 6},
                    {"channel_width", "20"},
                    {"noise", -99},
                    {"phy", "platform/soc/c000000.wifi+1"},
                    {"receive_ms", 72},
                    {"temperature", 40},
                    {"transmit_ms", 23818},
                    {"tx_power", 23}
                }
        };
        S["link-state"]["lan"]["eth1"]["carrier"] = 0;
        S["link-state"]["lan"]["eth2"]["carrier"] = 0;
        S["link-state"]["wan"]["eth0"]["carrier"] = 1;
        S["link-state"]["wan"]["eth0"]["duplex"] = "full";
        S["link-state"]["wan"]["eth0"]["speed"] = 1000;
        nlohmann::json interface0;

        nlohmann::json RFClients;
        AddClients(RFClients, clients_2g);
        AddClients(RFClients, clients_5g);

        interface0["clients"] = RFClients;
        interface0["location"] = "/interfaces/0";
        interface0["name"] = "up0v0";
        interface0["ipv4"]["addresses"].push_back("192.168.1.1/24");
        interface0["ipv4"]["leasetime"]= 43200;
        interface0["uptime"] = Now - StartTime_;
        AddCounters(interface0);
        interface0["dns_servers"].push_back("192.168.88.1");
        interface0["addresses"].push_back("192.168.88.91/24");

        nlohmann::json ssid_5g;
        ssid_5g["bssid"] = mac_5g;
        AddCounters(ssid_5g);
        ssid_5g["iface"] = "wlan0";
        ssid_5g["mode"] = "ap";
        ssid_5g["phy"] = "platform/soc/c000000.wifi";
        ssid_5g["radio"]["$ref"] = "#/radios/0";
        ssid_5g["ssid"] = "the5Gnetwork";
        AddAssociations(clients_5g,ssid_5g);

        nlohmann::json ssid_2g;
        ssid_2g["bssid"] = mac_2g;
        AddCounters(ssid_2g);
        ssid_2g["iface"] = "wlan0";
        ssid_2g["mode"] = "ap";
        ssid_2g["phy"] = "platform/soc/c000000.wifi+1";
        ssid_2g["radio"]["$ref"] = "#/radios/1";
        ssid_2g["ssid"] = "the2Gnetwork";
        AddAssociations(clients_2g,ssid_2g);

        interface0["ssids"].push_back(ssid_5g);
        interface0["ssids"].push_back(ssid_2g);

        nlohmann::json interface1;
        nlohmann::json LANClients;
        AddClients(LANClients, clients_lan);
        interface1["clients"] = LANClients;

        AddCounters(interface1);
        interface1["location"] = "/interfaces/1";
        interface1["name"] = "down1v0";
        interface1["uptime"] = Now - StartTime_;
        interface1["ipv4"]["addresses"].push_back("192.168.1.1/24");

        S["interfaces"].push_back(interface0);
        S["interfaces"].push_back(interface1);

        return S;
    }

    void uCentralClient::Disconnect( const char * Reason, bool Reconnect ) {
        std::lock_guard G(Mutex_);
        Logger_.debug(Poco::format("DEVICE(%s): disconnecting because '%s'", SerialNumber_, std::string{Reason}));
        if(Connected_) {
            Reactor_.removeEventHandler(*WS_, Poco::NObserver<uCentralClient, Poco::Net::ReadableNotification>(*this, &uCentralClient::OnSocketReadable));
            (*WS_).close();
        }

        Connected_ = false ;
        Commands_.clear();

        if(Reconnect)
            AddEvent(ev_reconnect,SimulationCoordinator()->GetSimulationInfo().reconnectInterval + MicroService::instance().Random(15) );

        SimStats()->Disconnect();
    }

    void uCentralClient::DoCensus( CensusReport & Census ) {
        std::lock_guard G(Mutex_);

        for(const auto i:Commands_)
            switch(i.second)
            {
            case ev_none: Census.ev_none++; break;
            case ev_reconnect: Census.ev_reconnect++; break;
            case ev_connect: Census.ev_connect++; break;
            case ev_state: Census.ev_state++; break;
            case ev_healthcheck: Census.ev_healthcheck++; break;
            case ev_log: Census.ev_log++; break;
            case ev_crashlog: Census.ev_crashlog++; break;
            case ev_configpendingchange: Census.ev_configpendingchange++; break;
            case ev_keepalive: Census.ev_keepalive++; break;
            case ev_reboot: Census.ev_reboot++; break;
            case ev_disconnect: Census.ev_disconnect++; break;
            case ev_wsping: Census.ev_wsping++; break;
            }
    }

    void uCentralClient::OnSocketReadable(const Poco::AutoPtr<Poco::Net::ReadableNotification>& pNf) {
        std::lock_guard G(Mutex_);

        try {
            char        Message[16000];
            int         Flags;

            auto MessageSize = WS_->receiveFrame(Message,sizeof(Message),Flags);
            auto Op = Flags & Poco::Net::WebSocket::FRAME_OP_BITMASK;

            if (MessageSize == 0 && Flags == 0 && Op == 0) {
                Disconnect("Error while waiting for data in WebSocket", true);
                return;
            }

            Message[MessageSize]=0;
            switch (Op) {
                case Poco::Net::WebSocket::FRAME_OP_PING: {
                    WS_->sendFrame("", 0,Poco::Net::WebSocket::FRAME_OP_PONG | Poco::Net::WebSocket::FRAME_FLAG_FIN);
                }
                break;

                case Poco::Net::WebSocket::FRAME_OP_PONG: {
                }
                break;

                case Poco::Net::WebSocket::FRAME_OP_TEXT: {
                    if (MessageSize > 0) {
                        SimStats()->AddRX(MessageSize);
                        SimStats()->AddInMsg();
                        auto Vars = nlohmann::json::parse(Message);

                        if( Vars.contains("jsonrpc") &&
                            Vars.contains("id") &&
                            Vars.contains("method") &&
                            Vars.contains("params"))
                        {
                            ProcessCommand(Vars);
                        } else {
                            Logger_.warning(Poco::format("MESSAGE(%s): invalid incoming message.",SerialNumber_));
                        }
                    }
                }
                break;
                default: {
                }
                break;
            }
            return;
        }
        catch ( const Poco::Net::SSLException & E )
        {
            Logger_.warning(Poco::format("Exception(%s): SSL exception: %s", SerialNumber_,E.displayText()));
        }
        catch ( const Poco::Exception & E )
        {
            Logger_.warning(Poco::format("Exception(%s): Generic exception: %s", SerialNumber_,E.displayText()));
        }
        Disconnect("Exception caught during data reception", true);
    }

    void uCentralClient::ProcessCommand(nlohmann::json & Vars) {

        std::string Method = Vars["method"];

        auto Id = Vars["id"];
        auto Params = Vars["params"];

        if(Method == "configure") {
            DoConfigure(Id,Params);
        } else if(Method =="reboot") {
            DoReboot(Id,Params);
        } else if(Method == "upgrade") {
            DoUpgrade(Id,Params);
        } else if(Method == "factory") {
            DoFactory(Id,Params);
        } else if(Method == "leds") {
            DoLEDs(Id,Params);
        } else if(Method == "perform") {
            DoPerform(Id,Params);
        } else if(Method == "trace") {
            DoTrace(Id,Params);
        } else {
            Logger_.warning(Poco::format("COMMAND(%s): unknown method '%s'",SerialNumber_,Method));
        }
    }

    void  uCentralClient::DoConfigure(uint64_t Id, nlohmann::json & Params) {
        std::lock_guard G(Mutex_);

        try {
            if (Params.contains("serial") &&
            Params.contains("uuid") &&
            Params.contains("config")) {
                uint64_t When = Params.contains("when") ? (uint64_t) Params["when"] : 0;
                auto Serial = Params["serial"];
                uint64_t UUID = Params["uuid"];
                auto Configuration = Params["config"];
                CurrentConfig_ = Configuration;
                UUID_ = Active_ = UUID ;

                //  prepare response...
                nlohmann::json Answer;

                Answer["jsonrpc"] = "2.0";
                Answer["id"] = Id;
                Answer["result"]["serial"] = Serial;
                Answer["result"]["uuid"] = UUID;
                Answer["result"]["status"]["error"] = 0;
                Answer["result"]["status"]["when"] = When;
                Answer["result"]["status"]["text"] = "No errors were found";
                Answer["result"]["status"]["error"] = 0;

                Logger_.information(Poco::format("configure(%s): done.",SerialNumber_));
                SendObject(Answer);
            } else {
                Logger_.warning(Poco::format("configure(%s): Illegal command.",SerialNumber_));
            }
        } catch (const Poco::Exception &E)
        {
            Logger_.warning(Poco::format("configure(%s): Exception. %s",SerialNumber_,E.displayText()));
        }
    }

    void  uCentralClient::DoReboot(uint64_t Id, nlohmann::json & Params) {
        std::lock_guard G(Mutex_);
        try {
            if (Params.contains("serial")) {
                uint64_t When = Params.contains("when") ? (uint64_t) Params["when"] : 0;
                auto Serial = Params["serial"];

                //  prepare response...
                nlohmann::json Answer;

                Answer["jsonrpc"] = "2.0";
                Answer["id"] = Id;
                Answer["result"]["serial"] = Serial;
                Answer["result"]["status"]["error"] = 0;
                Answer["result"]["status"]["when"] = When;
                Answer["result"]["status"]["text"] = "No errors were found";

                SendObject(Answer);

                Logger_.information(Poco::format("reboot(%s): done.",SerialNumber_));
                Disconnect("Rebooting" , true);
            } else {
                Logger_.warning(Poco::format("reboot(%s): Illegal command.",SerialNumber_));
            }
        } catch( const Poco::Exception &E )
        {
            Logger_.warning(Poco::format("reboot(%s): Exception. %s",SerialNumber_,E.displayText()));
        }
    }

    std::string GetFirmware(const std::string & U) {
        Poco::URI   uri(U);

        auto p = uri.getPath();
        auto tokens = Poco::StringTokenizer(p,"-");
        if(tokens.count()>4 && (tokens[2]=="main" || tokens[2]=="next" || tokens[2]=="staging")) {
            return "TIP-devel-" + tokens[3];
        }

        if(tokens.count()>5) {
            return "TIP-" + tokens[2] + "-" + tokens[3] + "-" + tokens[4];
        }

        return p;
    }

    void  uCentralClient::DoUpgrade(uint64_t Id, nlohmann::json & Params) {
        std::lock_guard G(Mutex_);
        try {
            if (Params.contains("serial") &&
                Params.contains("uri")) {

                uint64_t When = Params.contains("when") ? (uint64_t) Params["when"] : 0;
                auto Serial = to_string(Params["serial"]);
                auto URI = to_string(Params["uri"]);

                //  prepare response...
                nlohmann::json Answer;

                Answer["jsonrpc"] = "2.0";
                Answer["id"] = Id;
                Answer["result"]["serial"] = Serial;
                Answer["result"]["status"]["error"] = 0;
                Answer["result"]["status"]["when"] = When;
                Answer["result"]["status"]["text"] = "No errors were found";

                Version_++;
                SetFirmware(GetFirmware(URI));

                SendObject(Answer);
                Logger_.information(Poco::format("upgrade(%s): from URI=%s.",SerialNumber_,URI));
                Disconnect("Doing an upgrade", true);
            } else {
                Logger_.warning(Poco::format("upgrade(%s): Illegal command.",SerialNumber_));
            }
        } catch( const Poco::Exception &E )
        {
            Logger_.warning(Poco::format("upgrade(%s): Exception. %s",SerialNumber_,E.displayText()));
        }
    }

    void  uCentralClient::DoFactory(uint64_t Id, nlohmann::json & Params) {
        std::lock_guard G(Mutex_);
        try {
            if (Params.contains("serial") &&
                Params.contains("keep_redirector")) {

                uint64_t When = Params.contains("when") ? (uint64_t) Params["when"] : 0;
                auto Serial = to_string(Params["serial"]);
                auto KeepRedirector = Params["uri"];

                Version_ = 1;
                SetFirmware();
                KeepRedirector_ = KeepRedirector;

                nlohmann::json Answer;

                Answer["jsonrpc"] = "2.0";
                Answer["id"] = Id;
                Answer["result"]["serial"] = Serial;
                Answer["result"]["status"]["error"] = 0;
                Answer["result"]["status"]["when"] = When;
                Answer["result"]["status"]["text"] = "No errors were found";
                SendObject(Answer);

                Logger_.information(Poco::format("factory(%s): done.",SerialNumber_));
                Disconnect("Factory reset", true);
            } else {
                Logger_.warning(Poco::format("factory(%s): Illegal command.",SerialNumber_));
            }
        } catch( const Poco::Exception &E )
        {
            Logger_.warning(Poco::format("factory(%s): Exception. %s",SerialNumber_,E.displayText()));
        }
    }

    void  uCentralClient::DoLEDs(uint64_t Id, nlohmann::json & Params) {
        std::lock_guard G(Mutex_);
        try {
            if (Params.contains("serial") &&
            Params.contains("pattern")) {

                uint64_t    When = Params.contains("when") ? (uint64_t) Params["when"] : 0;
                auto        Serial = to_string(Params["serial"]);
                auto        Pattern = to_string(Params["pattern"]);
                uint64_t    Duration = Params.contains("duration") ? (uint64_t )Params["durarion"] : 10;

                //  prepare response...
                nlohmann::json Answer;

                Answer["jsonrpc"] = "2.0";
                Answer["id"] = Id;
                Answer["result"]["serial"] = Serial;
                Answer["result"]["status"]["error"] = 0;
                Answer["result"]["status"]["when"] = When;
                Answer["result"]["status"]["text"] = "No errors were found";
                SendObject(Answer);

                Logger_.information(Poco::format("LEDs(%s): pattern set to: %s for %Lu ms.",SerialNumber_,Duration,Pattern));
            } else {
                Logger_.warning(Poco::format("LEDs(%s): Illegal command.",SerialNumber_));
            }
        } catch( const Poco::Exception &E )
        {
            Logger_.warning(Poco::format("LEDs(%s): Exception. %s",SerialNumber_,E.displayText()));
        }
    }

    void  uCentralClient::DoPerform(uint64_t Id, nlohmann::json & Params) {
        std::lock_guard G(Mutex_);
        try {
            if (Params.contains("serial") &&
            Params.contains("command") &&
            Params.contains("payload")) {

                uint64_t    When = Params.contains("when") ? (uint64_t) Params["when"] : 0;
                auto        Serial = to_string(Params["serial"]);
                auto        Command = to_string(Params["command"]);
                auto        Payload = Params["payload"];

                //  prepare response...
                nlohmann::json Answer;

                Answer["jsonrpc"] = "2.0";
                Answer["id"] = Id;
                Answer["result"]["serial"] = Serial;
                Answer["result"]["status"]["error"] = 0;
                Answer["result"]["status"]["when"] = When;
                Answer["result"]["status"]["text"] = "No errors were found";
                Answer["result"]["status"]["text"]["resultCode"] = 0 ;
                Answer["result"]["status"]["text"]["resultText"] = "no return status" ;
                SendObject(Answer);
                Logger_.information(Poco::format("perform(%s): command=%s.",SerialNumber_,Command));
            } else {
                Logger_.warning(Poco::format("perform(%s): Illegal command.",SerialNumber_));
            }
        } catch( const Poco::Exception &E )
        {
            Logger_.warning(Poco::format("perform(%s): Exception. %s",SerialNumber_,E.displayText()));
        }
    }

    void  uCentralClient::DoTrace(uint64_t Id, nlohmann::json & Params) {
        std::lock_guard G(Mutex_);
        try {
            if (Params.contains("serial") &&
            Params.contains("duration") &&
            Params.contains("network") &&
            Params.contains("interface") &&
            Params.contains("packets") &&
            Params.contains("uri")) {

                uint64_t    When = Params.contains("when") ? (uint64_t) Params["when"] : 0;
                auto        Serial = to_string(Params["serial"]);
                auto        Network = to_string(Params["network"]);
                auto        Interface = to_string(Params["interface"]);
                uint64_t    Duration = Params["duration"];
                uint64_t    Packets = Params["packets"];
                auto        URI = to_string(Params["uri"]);

                //  prepare response...
                nlohmann::json Answer;

                Answer["jsonrpc"] = "2.0";
                Answer["id"] = Id;
                Answer["result"]["serial"] = Serial;
                Answer["result"]["status"]["error"] = 0;
                Answer["result"]["status"]["when"] = When;
                Answer["result"]["status"]["text"] = "No errors were found";
                Answer["result"]["status"]["text"]["resultCode"] = 0 ;
                Answer["result"]["status"]["text"]["resultText"] = "no return status" ;
                SendObject(Answer);

                Logger_.information(Poco::format("trace(%s): network=%s interface=%s packets=%Lu duration=%Lu URI=%s.",
                                                 SerialNumber_, Network,
                                                 Interface, Packets, Duration, URI));
            } else {
                Logger_.warning(Poco::format("trace(%s): Illegal command.",SerialNumber_));
            }
        } catch( const Poco::Exception &E )
        {
            Logger_.warning(Poco::format("trace(%s): Exception. %s",SerialNumber_,E.displayText()));
        }
    }

    void uCentralClient::EstablishConnection() {
        Poco::URI   uri(SimulationCoordinator()->GetSimulationInfo().gateway);

        Poco::Net::Context::Params  P;

        P.verificationMode = Poco::Net::Context::VERIFY_STRICT;
        P.verificationDepth = 9;
        P.caLocation = SimulationCoordinator()->GetCasLocation();
        P.loadDefaultCAs = false;
        P.certificateFile = SimulationCoordinator()->GetCertFileName();
        P.privateKeyFile = SimulationCoordinator()->GetKeyFileName();
        P.cipherList = "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH";
        P.dhUse2048Bits = true;

        auto Context = new Poco::Net::Context( Poco::Net::Context::CLIENT_USE,P);
        Poco::Crypto::X509Certificate Cert(SimulationCoordinator()->GetCertFileName());
        Poco::Crypto::X509Certificate Root(SimulationCoordinator()->GetRootCAFileName());

        Context->useCertificate(Cert);
        Context->addChainCertificate(Root);

        Context->addCertificateAuthority(Root);

        if (SimulationCoordinator()->GetLevel() == Poco::Net::Context::VERIFY_STRICT) {
        }

        Poco::Crypto::RSAKey Key("", SimulationCoordinator()->GetKeyFileName(), "");
        Context->usePrivateKey(Key);

        SSL_CTX *SSLCtx = Context->sslContext();
        if (!SSL_CTX_check_private_key(SSLCtx)) {
            std::cout << "Wrong Certificate: " << SimulationCoordinator()->GetCertFileName() << " for " << SimulationCoordinator()->GetKeyFileName() << std::endl;
        }

        if(SimulationCoordinator()->GetLevel()==Poco::Net::Context::VERIFY_STRICT) {
        }

        Poco::Net::HTTPSClientSession Session(  uri.getHost(), uri.getPort(), Context);
        Poco::Net::HTTPRequest Request(Poco::Net::HTTPRequest::HTTP_GET, "/?encoding=text",Poco::Net::HTTPMessage::HTTP_1_1);
        Request.set("origin", "http://www.websocket.org");
        Poco::Net::HTTPResponse Response;

        Logger_.information(Poco::format("connecting(%s): host=%s port=%Lu",SerialNumber_,uri.getHost(),(uint64_t )uri.getPort()));

        std::lock_guard guard(Mutex_);

        try {
            WS_ = std::make_unique<Poco::Net::WebSocket>(Session, Request, Response);
            (*WS_).setKeepAlive(true);
            (*WS_).setReceiveTimeout(Poco::Timespan());
            (*WS_).setSendTimeout(Poco::Timespan(20,0));
            (*WS_).setNoDelay(true);
            Reactor_.addEventHandler(*WS_, Poco::NObserver<uCentralClient,
                                     Poco::Net::ReadableNotification>(*this, &uCentralClient::OnSocketReadable));
            Connected_ = true ;

            AddEvent(ev_connect,1);
            SimStats()->Connect();
        }
        catch ( const Poco::Exception & E )
        {
            Logger_.warning(Poco::format("connecting(%s): exception. %s",SerialNumber_,E.displayText()));
            AddEvent(ev_reconnect,SimulationCoordinator()->GetSimulationInfo().reconnectInterval + MicroService::instance().Random(15));
        }
        catch ( const std::exception & E )
        {
            Logger_.warning(Poco::format("connecting(%s): std::exception. %s",SerialNumber_,E.what()));
            AddEvent(ev_reconnect,SimulationCoordinator()->GetSimulationInfo().reconnectInterval + MicroService::instance().Random(15));
        }
        catch ( ... )
        {
            Logger_.warning(Poco::format("connecting(%s): unknown exception. %s",SerialNumber_));
            AddEvent(ev_reconnect,SimulationCoordinator()->GetSimulationInfo().reconnectInterval + MicroService::instance().Random(15));
        }
    }

    bool uCentralClient::Send(const std::string & Cmd) {
        std::lock_guard guard(Mutex_);

        try {
            uint32_t BytesSent = WS_->sendFrame(Cmd.c_str(), Cmd.size());
            if (BytesSent == Cmd.size()) {
                SimStats()->AddTX(Cmd.size());
                SimStats()->AddOutMsg();
                return true;
            } else {
                Logger_.warning(Poco::format("SEND(%s): incomplete send. Sent: %l", SerialNumber_, BytesSent));
            }
        } catch(const Poco::Exception &E) {
            Logger_.log(E);
        }
        return false;
    }

    bool uCentralClient::SendWSPing() {
        std::lock_guard guard(Mutex_);

        try {
            WS_->sendFrame("", 0, Poco::Net::WebSocket::FRAME_OP_PING | Poco::Net::WebSocket::FRAME_FLAG_FIN);
            return true;
        }
        catch(const Poco::Exception &E) {
            Logger_.log(E);
        }
        return false;
    }

    bool uCentralClient::SendObject(nlohmann::json &O) {
        std::lock_guard guard(Mutex_);

        try {
            auto M = to_string(O);
            uint32_t BytesSent = WS_->sendFrame(M.c_str(), M.size());
            if (BytesSent == M.size()) {
                SimStats()->AddTX(BytesSent);
                SimStats()->AddOutMsg();
                return true;
            } else {
                Logger_.warning(Poco::format("SEND(%s): incomplete send object. Sent: %l", SerialNumber_, BytesSent));
            }
        }
        catch(const Poco::Exception &E) {
            Logger_.log(E);
        }
        return false;
    }

    static const uint64_t million = 1000000;

    void uCentralClient::AddEvent(uCentralEventType E, uint64_t InSeconds) {
        std::lock_guard guard(Mutex_);

        timeval curTime{0};
        gettimeofday(&curTime, nullptr);
        uint64_t NextCommand = (InSeconds * million) + (curTime.tv_sec * million) + curTime.tv_usec;

        //  we need to make sure we do not possibly overwrite other commands at the same time
        while(Commands_.find(NextCommand)!=Commands_.end())
            NextCommand++;

        Commands_[NextCommand] = E;
    }

    uCentralEventType uCentralClient::NextEvent(bool Remove) {
        std::lock_guard guard(Mutex_);

        if(Commands_.empty())
            return ev_none;

        auto EventTime = Commands_.begin()->first;
        timeval curTime{0};
        gettimeofday(&curTime, nullptr);
        uint64_t Now = (curTime.tv_sec * million) + curTime.tv_usec;

        if(EventTime<Now) {
            uCentralEventType E = Commands_.begin()->second;
            if(Remove)
                Commands_.erase(Commands_.begin());
            return E;
        }

        return ev_none;
    }
}
