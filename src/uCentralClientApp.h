//
// Created by stephane bourque on 2021-03-12.
//

#ifndef UCENTRAL_CLNT_UCENTRALCLIENTAPP_H
#define UCENTRAL_CLNT_UCENTRALCLIENTAPP_H
#include <string>
#include <map>
#include <mutex>
#include "uCentralClient.h"

#include "Poco/Util/ServerApplication.h"

class uCentralClientApp : public Poco::Util::ServerApplication {

public:
    void initialize(Application &self) override;
    void uninitialize() override;
    void reinitialize(Application &self) override;
    int main(const ArgVec &args) override;
    void Reconnect(const std::string & Serial);

private:
};


#endif //UCENTRAL_CLNT_UCENTRALCLIENTAPP_H
