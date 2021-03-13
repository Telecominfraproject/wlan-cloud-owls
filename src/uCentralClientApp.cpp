//
// Created by stephane bourque on 2021-03-12.
//

#include "uCentralClientApp.h"

int uCentralClientApp::main(const ArgVec &args) {

    std::string URI = "ws://localhost:15002";
    std::string Cert = "cert.pem";
    std::string Key = "key.pem";

    uint64_t    NumClients = 5;
    std::string SerialBase{ "220000300000"};

    for(auto i=0;i<NumClients;i++)
    {
        std::string Serial = SerialBase + std::to_string(i);

        auto Client = std::shared_ptr<uCentralClient>(new uCentralClient(Serial,URI,Key,Cert));

        Clients_.push_back(Client);
        Poco::ThreadPool::defaultPool().start(*Client);
    }

    waitForTerminationRequest();

    Poco::ThreadPool::defaultPool().joinAll();

    return 0;
}

void uCentralClientApp::initialize(Application &self) {
    ServerApplication::initialize(self);
    logger().information("Starting...");
    loadConfiguration();
}

void uCentralClientApp::uninitialize() {
    // add your own uninitialization code here
    Application::uninitialize();
}

void uCentralClientApp::reinitialize(Application &self) {
    Application::reinitialize(self);
    // add your own reinitialization code here
}