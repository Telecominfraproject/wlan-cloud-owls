//
// Created by stephane bourque on 2021-03-12.
//
#include <utility>
#include "uCentralClientApp.h"

#include "Poco/Path.h"
#include "Poco/Logger.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Net/Context.h"

#include "SimStats.h"
#include "StatsDisplay.h"

uCentralClientApp * App() { return dynamic_cast<uCentralClientApp *>(&uCentralClientApp::instance()); } ;

void MyErrorHandler::exception(const Poco::Exception & E) {
    Poco::Thread * CurrentThread = Poco::Thread::current();
    App()->logger().warning(Poco::format("generic Poco exception on %s",CurrentThread->getName()));
}

void MyErrorHandler::exception(const std::exception & E) {
    Poco::Thread * CurrentThread = Poco::Thread::current();
    App()->logger().warning(Poco::format("std::exception on %s",CurrentThread->getName()));
}

void MyErrorHandler::exception() {
    Poco::Thread * CurrentThread = Poco::Thread::current();
    App()->logger().warning(Poco::format("exception on %s",CurrentThread->getName()));
}

void uCentralClientApp::defineOptions(Poco::Util::OptionSet &options) {

    ServerApplication::defineOptions(options);

    options.addOption(
            Poco::Util::Option("help", "", "display help information on command line arguments")
                    .required(false)
                    .repeatable(false)
                    .callback(Poco::Util::OptionCallback<uCentralClientApp>(this, &uCentralClientApp::handleHelp)));

    options.addOption(
            Poco::Util::Option("file", "", "specify the configuration file")
                    .required(false)
                    .repeatable(false)
                    .argument("file")
                    .callback(Poco::Util::OptionCallback<uCentralClientApp>(this, &uCentralClientApp::handleConfig)));

    options.addOption(
            Poco::Util::Option("debug", "", "to run in debug, set to true")
                    .required(false)
                    .repeatable(false)
                    .callback(Poco::Util::OptionCallback<uCentralClientApp>(this, &uCentralClientApp::handleDebug)));

    options.addOption(
            Poco::Util::Option("logs", "", "specify the log directory and file (i.e. dir/file.log)")
                    .required(false)
                    .repeatable(false)
                    .argument("dir")
                    .callback(Poco::Util::OptionCallback<uCentralClientApp>(this, &uCentralClientApp::handleLogs)));

    options.addOption(
            Poco::Util::Option("clients", "", "The number of clients to run.")
                    .required(false)
                    .repeatable(false)
                    .argument("number")
                    .callback(Poco::Util::OptionCallback<uCentralClientApp>(this, &uCentralClientApp::handleNumClients)));
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
    Poco::Util::HelpFormatter helpFormatter(options());
    helpFormatter.setCommand(commandName());
    helpFormatter.setUsage("OPTIONS");
    helpFormatter.setHeader("A uCentral gateway implementation for TIP.");
    helpFormatter.format(std::cout);
}

void uCentralClientApp::StartSimulators() {

    logger().notice("Starting simulation threads...");
    for(const auto &i:SimThreads_)
        i->Thread.start(i->Sim);
}

void uCentralClientApp::StopSimulators() {
    logger().notice("Stopping simulation threads...");
    for(const auto &i:SimThreads_) {
        i->Sim.stop();
        i->Thread.join();
    }
}

int uCentralClientApp::main(const ArgVec &args) {

    Poco::ErrorHandler::set(&AppErrorHandler_);

    if (!helpRequested_) {

        Poco::Logger &logger = Poco::Logger::get("uCentral");

        //  How many reactors do we need??
        uint64_t NumThreads = 1;
        auto ClientCount = NumClients_;
        auto NumClientsPerThread = NumClients_;

        if(NumClients_>250) {
            NumThreads = MaxThreads_;
            if(NumClients_ % NumThreads == 0)
            {
                NumClientsPerThread = NumClients_ / NumThreads;
            }
            else
            {
                NumClientsPerThread = NumClients_ / (NumThreads+1);
            }
        }

        Poco::Logger    & ClientLogger = Poco::Logger::get("WS-CLIENT");
        ClientLogger.setLevel(Poco::Message::PRIO_WARNING);
        for(auto i=0;ClientCount;i++)
        {
            auto Clients = std::min(ClientCount,NumClientsPerThread);
            auto NewSimThread = std::make_unique<SimThread>(i,SerialNumberBase_,Clients);
            NewSimThread->Sim.Initialize(ClientLogger);
            SimThreads_.push_back(std::move(NewSimThread));
            ClientCount -= Clients;
        }

        StartSimulators();

        StatsDisplay    StatsReporting;
        Poco::Thread    Display;

        Display.start(StatsReporting);

        waitForTerminationRequest();

        StatsReporting.Stop();
        Display.join();

        StopSimulators();
        logger.notice("Simulation done...");
    }

    return Application::EXIT_OK;
}

static Poco::Net::Context::VerificationMode ConvertStringToLevel(const std::string &L) {
    if (L == "strict") {
        return Poco::Net::Context::VERIFY_STRICT;
    } else if (L == "none") {
        return Poco::Net::Context::VERIFY_NONE;
    } else if (L == "relaxed") {
        return Poco::Net::Context::VERIFY_RELAXED;
    } else if (L == "once")
        return Poco::Net::Context::VERIFY_ONCE;

    return Poco::Net::Context::VERIFY_STRICT;
}

void uCentralClientApp::initialize(Application &self) {
    std::string ConfigFileName = Poco::Path::expand( "$UCENTRAL_CLIENT_ROOT/owls.properties");
    Poco::Path ConfigFile = ConfigFileName_.empty() ? ConfigFileName : ConfigFileName_;

    if(!ConfigFile.isFile())
    {
        std::cout << "Configuration " << ConfigFile.toString() << " does not seem to exist. Please set $UCENTRAL_CLIENT_ROOT env variable the path of the owls.properties file." << std::endl;
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

    /*
    ucentral.websocket.clientcas = $UCENTRAL_ROOT/certs/clientcas.pem
    ucentral.websocket.key.password = mypassword
    */

    RootCAFileName_ = Poco::Path::expand(App()->config().getString("ucentral.websocket.rootca"));
    CertFileName_ = Poco::Path::expand(App()->config().getString("ucentral.websocket.cert"));
    KeyFileName_ = Poco::Path::expand(App()->config().getString("ucentral.websocket.key"));
    CASLocation_ = Poco::Path::expand(App()->config().getString("ucentral.websocket.cas"));
    ClientCASFileName_ = Poco::Path::expand(App()->config().getString("ucentral.websocket.clientcas"));
    IssuerFileName_ = Poco::Path::expand(App()->config().getString("ucentral.websocket.issuer"));
    std::string LevelS = Poco::Path::expand(App()->config().getString("ucentral.websocket.issuer"));
    Level_ = ConvertStringToLevel(LevelS);

    URI_ = App()->config().getString("ucentral.simulation.uri");
    if(NumClients_==0)
        NumClients_ = App()->config().getInt64("ucentral.simulation.maxclients");
    SerialNumberBase_ = App()->config().getString("ucentral.simulation.serialbase");
    HealthCheckInterval_ = App()->config().getInt64("ucentral.simulation.healthcheckinterval");
    StateInterval_ =App()->config().getInt64("ucentral.simulation.stateinterval");
    ReconnectInterval_ = App()->config().getInt64("ucentral.simulation.reconnect");
    KeepAliveInterval_ = App()->config().getInt64("ucentral.simulation.keepalive");
    ConfigChangePendingInterval_ = uCentralClientApp::instance().config().getInt64("ucentral.simulation.configchangepending");
    MaxThreads_ = App()->config().getInt64("ucentral.simulation.maxthreads",3);
}

void uCentralClientApp::uninitialize() {
    // add your own uninitialization code here
    Application::uninitialize();
}

void uCentralClientApp::reinitialize(Application &self) {
    Application::reinitialize(self);
    // add your own reinitialization code here
}