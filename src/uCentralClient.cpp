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
#include "Poco/JSON/Stringifier.h"
#include "Poco/JSON/Parser.h"

#include "uCentralClient.h"
#include "SimStats.h"
#include "Simulation.h"
#include <nlohmann/json.hpp>

using namespace std::chrono_literals;

namespace OpenWifi {

    uCentralClient::uCentralClient(
            Poco::Net::SocketReactor  & Reactor,
            std::string SerialNumber,
            Poco::Logger & Logger):
            Logger_(Logger),
            Reactor_(Reactor),
            SerialNumber_(std::move(SerialNumber))
            {
        std::cout << __func__ << " : " << __LINE__ << std::endl;
                SetFirmware();
                std::cout << __func__ << " : " << __LINE__ << std::endl;
                Active_ = UUID_ = std::time(nullptr);
                std::cout << __func__ << " : " << __LINE__ << std::endl;
                CurrentConfig_ = SimulationCoordinator()->GetSimConfiguration(UUID_);
                std::cout << __func__ << " : " << __LINE__ << std::endl;
            }

    void uCentralClient::Disconnect( bool Reconnect ) {
        std::lock_guard G(Mutex_);
        if(Connected_) {
            Reactor_.removeEventHandler(*WS_, Poco::NObserver<uCentralClient, Poco::Net::ReadableNotification>(*this, &uCentralClient::OnSocketReadable));
            (*WS_).close();
        }

        Connected_ = false ;
        Commands_.clear();

        if(Reconnect)
            AddEvent(ev_reconnect,SimulationCoordinator()->GetSimulationInfo().reconnectInterval + (rand() % 15) );

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
                Disconnect(true);
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
                        Poco::JSON::Parser  Parser;

                        SimStats()->AddRX(MessageSize);
                        SimStats()->AddInMsg();

                        auto ParsedMessage = Parser.parse(Message);
                        auto Result = ParsedMessage.extract<Poco::JSON::Object::Ptr>();
                        Poco::DynamicStruct Vars = *Result;

                        if( Vars.contains("jsonrpc") &&
                        Vars.contains("id") &&
                        Vars.contains("method"))
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
        Disconnect(true);
    }

    void uCentralClient::ProcessCommand(Poco::DynamicStruct Vars) {

        auto ParamsObj = Vars["params"];
        auto Method = Vars["method"].toString();

        if(!ParamsObj.isStruct())
        {
            Logger_.warning(Poco::format("COMMAND(%s): command '%s' does not have proper parameters",SerialNumber_,Method));
            return;
        }

        const auto & Params = ParamsObj.extract<Poco::DynamicStruct>();

        auto Id = Vars["id"];

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

    void  uCentralClient::DoConfigure(uint64_t Id, Poco::DynamicStruct Params) {
        std::lock_guard G(Mutex_);

        try {
            if (Params.contains("serial") &&
            Params.contains("uuid") &&
            Params.contains("config")) {
                uint64_t When = Params.contains("when") ? (uint64_t) Params["when"] : 0;
                auto Serial = Params["serial"].toString();
                uint64_t UUID = Params["uuid"];
                auto Configuration = Params["config"];

                //  We need to store the configuration  in the object...
                std::stringstream OS;
                Poco::JSON::Stringifier::stringify(Configuration, OS);

                CurrentConfig_ = OS.str();

                UUID_ = Active_ = UUID ;

                //  prepare response...
                Poco::JSON::Object Answer;

                Answer.set("jsonrpc", "2.0");
                Answer.set("id", Id);

                Poco::JSON::Object Result;

                Result.set("serial", Serial);
                Result.set("uuid", UUID);

                Poco::JSON::Object Status;
                Status.set("error", 0);
                Status.set("when", When);
                Status.set("text", "No errors were found");
                Result.set("status", Status);
                Answer.set("result", Result);

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

    void  uCentralClient::DoReboot(uint64_t Id, Poco::DynamicStruct Params) {
        std::lock_guard G(Mutex_);
        try {
            if (Params.contains("serial")) {
                uint64_t When = Params.contains("when") ? (uint64_t) Params["when"] : 0;
                auto Serial = Params["serial"].toString();

                //  prepare response...
                Poco::JSON::Object Answer;

                Answer.set("jsonrpc", "2.0");
                Answer.set("id", Id);

                Poco::JSON::Object Result;

                Result.set("serial", Serial);

                Poco::JSON::Object Status;
                Status.set("error", 0);
                Status.set("when", When);
                Status.set("text", "No errors were found");
                Result.set("status", Status);
                Answer.set("result", Result);
                SendObject(Answer);
                Logger_.information(Poco::format("reboot(%s): done.",SerialNumber_));
                Disconnect(true);
            } else {
                Logger_.warning(Poco::format("reboot(%s): Illegal command.",SerialNumber_));
            }
        } catch( const Poco::Exception &E )
        {
            Logger_.warning(Poco::format("reboot(%s): Exception. %s",SerialNumber_,E.displayText()));
        }
    }

    void  uCentralClient::DoUpgrade(uint64_t Id, Poco::DynamicStruct Params) {
        std::lock_guard G(Mutex_);
        try {
            if (Params.contains("serial") &&
            Params.contains("uri")) {

                uint64_t When = Params.contains("when") ? (uint64_t) Params["when"] : 0;
                auto Serial = Params["serial"].toString();
                auto URI = Params["uri"].toString();

                //  prepare response...
                Poco::JSON::Object Answer;

                Answer.set("jsonrpc", "2.0");
                Answer.set("id", Id);

                Version_++;
                SetFirmware();

                Poco::JSON::Object Result;

                Result.set("serial", Serial);

                Poco::JSON::Object Status;
                Status.set("error", 0);
                Status.set("when", When);
                Status.set("text", "No errors were found");
                Result.set("status", Status);
                Answer.set("result", Result);
                SendObject(Answer);
                Logger_.information(Poco::format("upgrade(%s): from URI=%s.",SerialNumber_,URI));
                Disconnect(true);
            } else {
                Logger_.warning(Poco::format("upgrade(%s): Illegal command.",SerialNumber_));
            }
        } catch( const Poco::Exception &E )
        {
            Logger_.warning(Poco::format("upgrade(%s): Exception. %s",SerialNumber_,E.displayText()));
        }
    }

    void  uCentralClient::DoFactory(uint64_t Id, Poco::DynamicStruct Params) {
        std::lock_guard G(Mutex_);
        try {
            if (Params.contains("serial") &&
            Params.contains("keep_redirector")) {

                uint64_t When = Params.contains("when") ? (uint64_t) Params["when"] : 0;
                auto Serial = Params["serial"].toString();
                auto KeepRedirector = Params["uri"];

                //  prepare response...
                Poco::JSON::Object Answer;

                Answer.set("jsonrpc", "2.0");
                Answer.set("id", Id);

                Version_ = 1;
                SetFirmware();
                KeepRedirector_ = KeepRedirector;

                Poco::JSON::Object Result;

                Result.set("serial", Serial);

                Poco::JSON::Object Status;
                Status.set("error", 0);
                Status.set("when", When);
                Status.set("text", "No errors were found");
                Result.set("status", Status);
                Answer.set("result", Result);
                SendObject(Answer);
                Logger_.information(Poco::format("factory(%s): done.",SerialNumber_));
                Disconnect(true);
            } else {
                Logger_.warning(Poco::format("factory(%s): Illegal command.",SerialNumber_));
            }
        } catch( const Poco::Exception &E )
        {
            Logger_.warning(Poco::format("factory(%s): Exception. %s",SerialNumber_,E.displayText()));
        }
    }

    void  uCentralClient::DoLEDs(uint64_t Id, Poco::DynamicStruct Params) {
        std::lock_guard G(Mutex_);
        try {
            if (Params.contains("serial") &&
            Params.contains("pattern")) {

                uint64_t    When = Params.contains("when") ? (uint64_t) Params["when"] : 0;
                auto        Serial = Params["serial"].toString();
                auto        Pattern = Params["pattern"].toString();
                uint64_t    Duration = Params.contains("duration") ? (uint64_t )Params["durarion"] : 10;

                //  prepare response...
                Poco::JSON::Object Answer;

                Answer.set("jsonrpc", "2.0");
                Answer.set("id", Id);

                Poco::JSON::Object Result;

                Result.set("serial", Serial);

                Poco::JSON::Object Status;
                Status.set("error", 0);
                Status.set("when", When);
                Status.set("text", "No errors were found");
                Result.set("status", Status);
                Answer.set("result", Result);
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

    void  uCentralClient::DoPerform(uint64_t Id, Poco::DynamicStruct Params) {
        std::lock_guard G(Mutex_);
        try {
            if (Params.contains("serial") &&
            Params.contains("command") &&
            Params.contains("payload")) {

                uint64_t    When = Params.contains("when") ? (uint64_t) Params["when"] : 0;
                auto        Serial = Params["serial"].toString();
                auto        Command = Params["command"].toString();
                auto        Payload = Params["payload"];

                //  prepare response...
                Poco::JSON::Object Answer;

                Answer.set("jsonrpc", "2.0");
                Answer.set("id", Id);

                Poco::JSON::Object Result;

                Result.set("serial", Serial);

                Poco::JSON::Object Status;
                Status.set("error", 0);
                Status.set("when", When);
                Status.set("text", "No errors were found");
                Status.set("resultCode",0);
                Status.set("resultText","no return status");
                Result.set("status", Status);
                Answer.set("result", Result);
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

    void  uCentralClient::DoTrace(uint64_t Id, Poco::DynamicStruct Params) {
        std::lock_guard G(Mutex_);
        try {
            if (Params.contains("serial") &&
            Params.contains("duration") &&
            Params.contains("network") &&
            Params.contains("interface") &&
            Params.contains("packets") &&
            Params.contains("uri")) {

                uint64_t    When = Params.contains("when") ? (uint64_t) Params["when"] : 0;
                auto        Serial = Params["serial"].toString();
                auto        Network = Params["network"].toString();
                auto        Interface = Params["interface"].toString();
                uint64_t    Duration = Params["duration"];
                uint64_t    Packets = Params["packets"];
                auto        URI = Params["uri"].toString();

                //  prepare response...
                Poco::JSON::Object Answer;

                Answer.set("jsonrpc", "2.0");
                Answer.set("id", Id);

                Poco::JSON::Object Result;

                Result.set("serial", Serial);

                Poco::JSON::Object Status;
                Status.set("error", 0);
                Status.set("when", When);
                Status.set("text", "No errors were found");
                Status.set("resultCode",0);
                Status.set("resultText","no return status");
                Result.set("status", Status);
                Answer.set("result", Result);
                SendObject(Answer);
                Logger_.information(Poco::format("trace(%s): network=%s interface=%s packets=%Lu duration=%Lu URI=%s.",SerialNumber_,Network,
                                                 Interface, Packets, Duration, URI));
            } else {
                Logger_.warning(Poco::format("trace(%s): Illegal command.",SerialNumber_));
            }
        } catch( const Poco::Exception &E )
        {
            Logger_.warning(Poco::format("trace(%s): Exception. %s",SerialNumber_,E.displayText()));
        }
    }

    void uCentralClient::Terminate() {
        std::lock_guard G(Mutex_);
        Disconnect(false);
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
        /*    Poco::Crypto::X509Certificate   Cert(App()->GetCertFileName());
            Poco::Crypto::RSAKey            Key("",App()->GetKeyFileName(),"");

            std::cout << "Name: " << Key.name() << "Size: " << Key.size() << std::endl;
            std::cout << " Issuer:" << Cert.issuerName() << std::endl;
            Context->useCertificate(Cert);
            Context->usePrivateKey(Key);
            Context->disableStatelessSessionResumption();
            Context->enableExtendedCertificateVerification();
        */

        Poco::Crypto::X509Certificate Cert(SimulationCoordinator()->GetCertFileName());
        Poco::Crypto::X509Certificate Root(SimulationCoordinator()->GetRootCAFileName());

        Context->useCertificate(Cert);
        Context->addChainCertificate(Root);

        Context->addCertificateAuthority(Root);

        if (SimulationCoordinator()->GetLevel() == Poco::Net::Context::VERIFY_STRICT) {
            //        Poco::Crypto::X509Certificate Issuing(App()->GetIssuerFileName());
            //        Context->addChainCertificate(Issuing);
            //        Context->addCertificateAuthority(Issuing);
        }

        Poco::Crypto::RSAKey Key("", SimulationCoordinator()->GetKeyFileName(), "");
        Context->usePrivateKey(Key);

        SSL_CTX *SSLCtx = Context->sslContext();
        if (!SSL_CTX_check_private_key(SSLCtx)) {
            std::cout << "Wrong Certificate: " << SimulationCoordinator()->GetCertFileName() << " for " << SimulationCoordinator()->GetKeyFileName() << std::endl;
        }

        //    SSL_CTX_set_verify(SSLCtx, SSL_VERIFY_PEER, NULL);

        if(SimulationCoordinator()->GetLevel()==Poco::Net::Context::VERIFY_STRICT) {
            // SSL_CTX_set_client_CA_list(SSLCtx, SSL_load_client_CA_file(App()->GetClientCASFileName().c_str()));
        }
        //    SSL_CTX_enable_ct(SSLCtx, SSL_CT_VALIDATION_STRICT);
        //    SSL_CTX_dane_enable(SSLCtx);

        //    Context->enableSessionCache();
        //    Context->setSessionCacheSize(0);
        //    Context->setSessionTimeout(10);
        //    Context->enableExtendedCertificateVerification(true);
        //    Context->disableStatelessSessionResumption();

        std::cout << "URI: " << SimulationCoordinator()->GetSimulationInfo().gateway << std::endl;
        std::cout << "Host: " << uri.getHost() << " : " << uri.getPort() << std::endl;

        Poco::Net::HTTPSClientSession Session(  uri.getHost(), uri.getPort(), Context);
        Poco::Net::HTTPRequest Request(Poco::Net::HTTPRequest::HTTP_GET, "/?encoding=text",Poco::Net::HTTPMessage::HTTP_1_1);
        Request.set("origin", "http://www.websocket.org");
        Poco::Net::HTTPResponse Response;

        Logger_.information(Poco::format("connecting(%s): host=%s port=%Lu",SerialNumber_,uri.getHost(),(uint64_t )uri.getPort()));

        my_guard guard(Mutex_);

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
            AddEvent(ev_reconnect,SimulationCoordinator()->GetSimulationInfo().reconnectInterval+(rand()%15));
        }
        catch ( const std::exception & E )
        {
            Logger_.warning(Poco::format("connecting(%s): std::exception. %s",SerialNumber_,E.what()));
            AddEvent(ev_reconnect,SimulationCoordinator()->GetSimulationInfo().reconnectInterval+(rand()%15));
        }
        catch ( ... )
        {
            Logger_.warning(Poco::format("connecting(%s): unknown exception. %s",SerialNumber_));
            AddEvent(ev_reconnect,SimulationCoordinator()->GetSimulationInfo().reconnectInterval+(rand()%15));
        }
    }

    bool uCentralClient::Send(const std::string & Cmd) {
        my_guard guard(Mutex_);

        try {
            uint32_t BytesSent = WS_->sendFrame(Cmd.c_str(), Cmd.size());
            if (BytesSent == Cmd.size()) {
                SimStats()->AddTX(Cmd.size());
                SimStats()->AddOutMsg();
                return true;
            } else {
                Logger_.warning(Poco::format("SEND(%s): incomplete send. Sent: %l", SerialNumber_, BytesSent));
            }
        } catch(...) {

        }
        return false;
    }

    bool uCentralClient::SendWSPing() {
        my_guard guard(Mutex_);

        try {
            WS_->sendFrame("", 0, Poco::Net::WebSocket::FRAME_OP_PING | Poco::Net::WebSocket::FRAME_FLAG_FIN);
            return true;
        }
        catch(...) {

        }
        return false;
    }

    bool uCentralClient::SendObject(Poco::JSON::Object O) {
        my_guard guard(Mutex_);

        try {
            std::stringstream OS;
            Poco::JSON::Stringifier::stringify(O, OS);
            uint32_t BytesSent = WS_->sendFrame(OS.str().c_str(), OS.str().size());
            if (BytesSent == OS.str().size()) {
                SimStats()->AddTX(BytesSent);
                SimStats()->AddOutMsg();
                return true;
            } else {
                Logger_.warning(Poco::format("SEND(%s): incomplete send object. Sent: %l", SerialNumber_, BytesSent));
            }
        } catch (...) {

        }
        return false;
    }

    static const uint64_t million = 1000000;

    void uCentralClient::AddEvent(uCentralEventType E, uint64_t InSeconds) {
        my_guard guard(Mutex_);

        timeval curTime{0};
        gettimeofday(&curTime, nullptr);
        uint64_t NextCommand = (InSeconds * million) + (curTime.tv_sec * million) + curTime.tv_usec;

        //  we need to make sure we do not possibly overwrite other commands at the same time
        while(Commands_.find(NextCommand)!=Commands_.end())
            NextCommand++;

        Commands_[NextCommand] = E;
    }

    uCentralEventType uCentralClient::NextEvent(bool Remove) {
        my_guard guard(Mutex_);

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
