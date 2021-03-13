//
// Created by stephane bourque on 2021-03-12.
//

#ifndef UCENTRAL_CLNT_UCENTRALCLIENT_H
#define UCENTRAL_CLNT_UCENTRALCLIENT_H

#include "Poco/Thread.h"

class uCentralClient : public Poco::Runnable {
public:
    uCentralClient( const std::string & SerialNumber,
              const std::string & URI,
              const std::string & KeyFileName,
              const std::string & CertFileName)
      : SerialNumber_(SerialNumber),
      URI_( URI ),
      KeyFileName_(KeyFileName),
      CertFileName_(CertFileName)
    {

    }

    [[nodiscard]] std::string DefaultCapabilities() const ;
    [[nodiscard]] std::string DefaultConfiguration() const ;

    void run() override;

private:
    std::string     SerialNumber_;
    std::string     URI_;
    std::string     KeyFileName_;
    std::string     CertFileName_;
};


#endif //UCENTRAL_CLNT_UCENTRALCLIENT_H
