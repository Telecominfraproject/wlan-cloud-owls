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
#include "Simulator.h"
#include "Poco/Util/OptionSet.h"
using Poco::Util::OptionSet;


class uCentralClientApp : public Poco::Util::ServerApplication {

public:
    uCentralClientApp()
    : helpRequested_(false),
    DebugMode_(false),
    NumClients_(0) {

    }

    void initialize(Application &self) override;
    void uninitialize() override;
    void reinitialize(Application &self) override;
    int main(const ArgVec &args) override;
    void defineOptions(OptionSet &options) override;

    void handleHelp(const std::string &name, const std::string &value);
    void handleDebug(const std::string &name, const std::string &value);
    void handleLogs(const std::string &name, const std::string &value);
    void handleConfig(const std::string &name, const std::string &value);
    void handleNumClients(const std::string &name, const std::string &value);
    void displayHelp();

    Simulator & GetSimulator() { return Sim_; }
    uint64_t GetStateInterval() const { return StateInterval_; }
    uint64_t GetHealthCheckInterval() const { return HealthCheckInterval_; }
    uint64_t GetReconnectInterval() const { return ReconnectInterval_; }
    const std::string & GetURI() { return URI_; }
    const std::string & GetCertFileName() { return CertFileName_; }
    const std::string & GetKeyFileName() { return KeyFileName_; }
    const std::string & GetSerialNumberBase() { return SerialNumberBase_; }
    uint64_t GetNumClients() const { return NumClients_; }

private:
    Poco::Thread    SimThr;
    Simulator       Sim_;
    bool                        helpRequested_;
    bool                        DebugMode_;
    std::string                                             URI_;
    std::string                                             CertFileName_;
    std::string                                             KeyFileName_;
    std::string                                             SerialNumberBase_;
    uint64_t                                                NumClients_;
    uint64_t                                                HealthCheckInterval_;
    uint64_t                                                StateInterval_;
    uint64_t                                                ReconnectInterval_;
    std::string                 ConfigFileName_;
    std::string                 LogDir_;
};

uCentralClientApp * Service();


#endif //UCENTRAL_CLNT_UCENTRALCLIENTAPP_H
