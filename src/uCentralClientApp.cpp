//
// Created by stephane bourque on 2021-03-12.
//
#include "uCentralClientApp.h"

#include "Poco/Path.h"
#include "Poco/Logger.h"
#include "Simulator.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"

using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;
using Poco::Util::OptionCallback;

uCentralClientApp * Service() { return reinterpret_cast<uCentralClientApp *>(&uCentralClientApp::instance()); } ;

void uCentralClientApp::defineOptions(OptionSet &options) {

    ServerApplication::defineOptions(options);

    options.addOption(
            Option("help", "", "display help information on command line arguments")
                    .required(false)
                    .repeatable(false)
                    .callback(OptionCallback<uCentralClientApp>(this, &uCentralClientApp::handleHelp)));

    options.addOption(
            Option("file", "", "specify the configuration file")
                    .required(false)
                    .repeatable(false)
                    .argument("file")
                    .callback(OptionCallback<uCentralClientApp>(this, &uCentralClientApp::handleConfig)));

    options.addOption(
            Option("debug", "", "to run in debug, set to true")
                    .required(false)
                    .repeatable(false)
                    .callback(OptionCallback<uCentralClientApp>(this, &uCentralClientApp::handleDebug)));

    options.addOption(
            Option("logs", "", "specify the log directory and file (i.e. dir/file.log)")
                    .required(false)
                    .repeatable(false)
                    .argument("dir")
                    .callback(OptionCallback<uCentralClientApp>(this, &uCentralClientApp::handleLogs)));

    options.addOption(
            Option("clients", "", "The number of clients to run.")
                    .required(false)
                    .repeatable(false)
                    .argument("number")
                    .callback(OptionCallback<uCentralClientApp>(this, &uCentralClientApp::handleNumClients)));
}

void uCentralClientApp::handleHelp(const std::string &name, const std::string &value) {
    helpRequested_ = true;
    displayHelp();
    stopOptionsProcessing();
}

void uCentralClientApp::handleDebug(const std::string &name, const std::string &value) {
    if(value == "true")
        DebugMode_ = true ;
}

void uCentralClientApp::handleLogs(const std::string &name, const std::string &value) {
    LogDir_ = value;
}

void uCentralClientApp::handleConfig(const std::string &name, const std::string &value) {
    ConfigFileName_ = value;
}

void uCentralClientApp::handleNumClients(const std::string &name, const std::string &value) {
    NumClients_ = std::stoi(value);
}

void uCentralClientApp::displayHelp() {
    HelpFormatter helpFormatter(options());
    helpFormatter.setCommand(commandName());
    helpFormatter.setUsage("OPTIONS");
    helpFormatter.setHeader("A uCentral gateway implementation for TIP.");
    helpFormatter.format(std::cout);
}

int uCentralClientApp::main(const ArgVec &args) {

    if (!helpRequested_) {

        Poco::Logger &logger = Poco::Logger::get("uCentral");

        SimThr.start(Sim_);

        waitForTerminationRequest();

        logger.information("Waiting for simulation to stop...");

        Sim_.stop();

        SimThr.join();

        logger.information("Simulation done...");
    }

    return Application::EXIT_OK;
}

void uCentralClientApp::initialize(Application &self) {
    std::string ConfigFileName = Poco::Path::expand( "$UCENTRAL_CLIENT_ROOT/ucentral-clnt.properties");
    Poco::Path ConfigFile = ConfigFileName_.empty() ? ConfigFileName : ConfigFileName_;

    if(!ConfigFile.isFile())
    {
        std::cout << "Configuration " << ConfigFile.toString() << " does not seem to exist. Please set $UCENTRAL_CLIENT_ROOT env variable the path of the ucentral-clnt.properties file." << std::endl;
        std::exit(EXIT_CONFIG);
    }

    loadConfiguration(ConfigFileName);

    char LogFilePathKey[] = "logging.channels.c2.path";

    loadConfiguration(ConfigFileName);

    if(LogDir_.empty()) {
        std::string OriginalLogFileValue = config().getString(LogFilePathKey);
        std::string RealLogFileValue = Poco::Path::expand(OriginalLogFileValue);
        config().setString(LogFilePathKey, RealLogFileValue);
    } else {
        config().setString(LogFilePathKey, LogDir_);
    }

    ServerApplication::initialize(self);
    logger().information("Starting...");

    CertFileName_ = Poco::Path::expand(uCentralClientApp::instance().config().getString("ucentral.simulation.certfile"));
    KeyFileName_ = Poco::Path::expand(uCentralClientApp::instance().config().getString("ucentral.simulation.keyfile"));
    URI_ = uCentralClientApp::instance().config().getString("ucentral.simulation.uri");
    if(NumClients_==0)
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