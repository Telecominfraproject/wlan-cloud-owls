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

class uCentralClient {
public:
    enum Protocol {
        legacy,
        jsonrpc
    };

    uCentralClient(
              Poco::Net::SocketReactor  & Reactor,
              const std::string & SerialNumber,
              const std::string & URI,
              const std::string & KeyFileName,
              const std::string & CertFileName,
              Protocol Proto)
      : Reactor_(Reactor),
      SerialNumber_(SerialNumber),
      URI_( URI ),
      KeyFileName_(KeyFileName),
      CertFileName_(CertFileName),
      Protocol_(Proto),
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

    void Connect();
    void SendState();
    void Terminate();
    void Disconnect();
    void SendHeartBeat();

private:
    Poco::Net::SocketReactor    & Reactor_;
    std::string             CurrentConfig_;
    uint64_t                CurrentConfigUUID_;
    std::string             SerialNumber_;
    std::string             URI_;
    std::string             KeyFileName_;
    std::string             CertFileName_;
    std::shared_ptr<Poco::Net::WebSocket>   WS_;
    Protocol                Protocol_;
    bool                    Connected_;

};


#endif //UCENTRAL_CLNT_UCENTRALCLIENT_H
