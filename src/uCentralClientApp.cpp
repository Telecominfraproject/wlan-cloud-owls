//
// Created by stephane bourque on 2021-03-12.
//
#include "uCentralClientApp.h"

#include "Poco/Path.h"
#include "Poco/Logger.h"
#include "Simulator.h"

uCentralClientApp * Service() { return reinterpret_cast<uCentralClientApp *>(&uCentralClientApp::instance()); } ;

int uCentralClientApp::main(const ArgVec &args) {

    Poco::Logger &logger = Poco::Logger::get("uCentral");

    SimThr.start(Sim_);

    waitForTerminationRequest();

    logger.information("Waiting for simulation to stop...");

    Sim_.stop();

    SimThr.join();

    logger.information("Simulation done...");

    return 0;
}

void uCentralClientApp::initialize(Application &self) {
    std::string ConfigFileName = Poco::Path::expand( "$UCENTRAL_CLIENT_ROOT/ucentral-clnt.properties");
    loadConfiguration(ConfigFileName);

    char LogFilePathKey[] = "logging.channels.c2.path";

    loadConfiguration(ConfigFileName);

    std::string OriginalLogFileValue = config().getString(LogFilePathKey);
    std::string RealLogFileValue = Poco::Path::expand(OriginalLogFileValue);
    config().setString(LogFilePathKey, RealLogFileValue);

    ServerApplication::initialize(self);
    logger().information("Starting...");

    CertFileName_ = Poco::Path::expand(uCentralClientApp::instance().config().getString("ucentral.simulation.certfile"));
    KeyFileName_ = Poco::Path::expand(uCentralClientApp::instance().config().getString("ucentral.simulation.keyfile"));
    URI_ = uCentralClientApp::instance().config().getString("ucentral.simulation.uri");
    NumClients_ = uCentralClientApp::instance().config().getInt64("ucentral.simulation.maxclients");
    SerialNumberBase_ = uCentralClientApp::instance().config().getString("ucentral.simulation.serialbase");
    HealthCheckInterval_ = uCentralClientApp::instance().config().getInt64("ucentral.simulation.healthcheckinterval");
    StateInterval_ = uCentralClientApp::instance().config().getInt64("ucentral.simulation.stateinterval");
    ReconnectInterval_ = uCentralClientApp::instance().config().getInt64("ucentral.simulation.reconnect");
}

void uCentralClientApp::uninitialize() {
    // add your own uninitialization code here
    Application::uninitialize();
}

void uCentralClientApp::reinitialize(Application &self) {
    Application::reinitialize(self);
    // add your own reinitialization code here
}