//
// Created by stephane bourque on 2021-03-12.
//

#ifndef UCENTRAL_CLNT_UCENTRALCLIENT_H
#define UCENTRAL_CLNT_UCENTRALCLIENT_H

#include "Poco/Thread.h"
#include "Poco/Net/SocketReactor.h"
#include "Poco/Net/SocketNotification.h"
#include "Poco/AutoPtr.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/Logger.h"

/*
 * States initialized -> connecting -> connected -> sending_hello -> waiting_for_hello -> running -> terminating
 *
 */

class uCentralClient {
public:

    enum ClientStates {
        initialized,
        connecting,
        connected,
        sending_hello,
        running,
        closing
    };

    uCentralClient(
              Poco::Net::SocketReactor  & Reactor,
              const std::string & SerialNumber,
              const std::string & URI,
              const std::string & KeyFileName,
              const std::string & CertFileName,
                    Poco::Logger & Logger):
                    Logger_(Logger),
                    Reactor_(Reactor),
                    SerialNumber_(SerialNumber),
                    URI_( URI ),
                    KeyFileName_(KeyFileName),
                    CertFileName_(CertFileName),
                    State_(initialized),
                    NextState_(0),
                    NextCheck_(0),
                    NextConnect_(0),
                    Connected_(false)
    {
        DefaultConfiguration(CurrentConfig_,CurrentConfigUUID_);
    }

    static std::string DefaultCapabilities();
    static std::string DefaultState();
    static void DefaultConfiguration( std::string & Config, uint64_t & UUID );

    bool SendCommand(const std::string &Cmd);
    void OnSocketReadable(const Poco::AutoPtr<Poco::Net::ReadableNotification>& pNf);
    void OnSocketShutdown(const Poco::AutoPtr<Poco::Net::ShutdownNotification>& pNf);

    void Connect1();
    void Connect();
    void SendState();
    void Terminate();
    void Disconnect();
    void SendHealthCheck();
    void SendConnection();
    void SendClosing();

    void SetState(ClientStates C) { State_ = C; };
    ClientStates GetState() { std::lock_guard<std::mutex> guard(mutex_); return State_; }
    uint64_t GetNextState() { return NextState_; }
    uint64_t GetNextCheck() { return NextCheck_; }
    uint64_t GetNextConnect() { return NextConnect_; }

    bool Connected() { return Connected_; }

private:
    std::mutex                  mutex_;
    Poco::Net::SocketReactor    & Reactor_;
    Poco::Logger                & Logger_;
    std::string                 CurrentConfig_;
    uint64_t                    CurrentConfigUUID_;
    std::string                 SerialNumber_;
    std::string                 URI_;
    std::string                 KeyFileName_;
    std::string                 CertFileName_;
    std::shared_ptr<Poco::Net::WebSocket>   WS_;
    ClientStates                State_;
    uint64_t                    NextState_;
    uint64_t                    NextCheck_;
    uint64_t                    NextConnect_;
    bool                        Connected_;
};


#endif //UCENTRAL_CLNT_UCENTRALCLIENT_H
