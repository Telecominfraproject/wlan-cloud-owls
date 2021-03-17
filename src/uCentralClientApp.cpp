//
// Created by stephane bourque on 2021-03-12.
//
#include "uCentralClientApp.h"

#include "Poco/Path.h"
#include "Simulator.h"

int uCentralClientApp::main(const ArgVec &args) {

    Poco::Thread    SimThr;
    Simulator       Sim;

    SimThr.start(Sim);

    waitForTerminationRequest();

    Sim.stop();

    SimThr.join();

    return 0;
}

void uCentralClientApp::initialize(Application &self) {
    ServerApplication::initialize(self);
    logger().information("Starting...");
    std::string ConfigFileName = Poco::Path::expand( "$UCENTRAL_CLIENT_ROOT/ucentral-clnt.properties");
    loadConfiguration(ConfigFileName);
}

void uCentralClientApp::uninitialize() {
    // add your own uninitialization code here
    Application::uninitialize();
}

void uCentralClientApp::reinitialize(Application &self) {
    Application::reinitialize(self);
    // add your own reinitialization code here
}