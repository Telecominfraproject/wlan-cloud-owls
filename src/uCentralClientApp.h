//
// Created by stephane bourque on 2021-03-12.
//

#ifndef UCENTRAL_CLNT_UCENTRALCLIENTAPP_H
#define UCENTRAL_CLNT_UCENTRALCLIENTAPP_H

#include <string>
#include <mutex>
#include <vector>

#include "uCentralClient.h"
#include "Simulator.h"

#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/ErrorHandler.h"

class MyErrorHandler : public Poco::ErrorHandler {
public:
    void exception(const Poco::Exception & E) override;
    void exception(const std::exception & E) override;
    void exception() override;
private:

};

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
    void defineOptions(Poco::Util::OptionSet &options) override;

    void handleHelp(const std::string &name, const std::string &value);
    void handleDebug(const std::string &name, const std::string &value);
    void handleLogs(const std::string &name, const std::string &value);
    void handleConfig(const std::string &name, const std::string &value);
    void handleNumClients(const std::string &name, const std::string &value);
    void displayHelp();

    [[nodiscard]] uint64_t GetStateInterval() const { return StateInterval_; }
    [[nodiscard]] uint64_t GetHealthCheckInterval() const { return HealthCheckInterval_; }
    [[nodiscard]] uint64_t GetReconnectInterval() const { return ReconnectInterval_; }
    [[nodiscard]] uint64_t GetKeepAliveInterval() const { return KeepAliveInterval_; }
    [[nodiscard]] uint64_t GetConfigChangePendingInterval() const { return ConfigChangePendingInterval_; }
    [[nodiscard]] uint64_t GetMaxThreads() const { return MaxThreads_; }

    [[nodiscard]] const std::string & GetURI() { return URI_; }
    [[nodiscard]] const std::string & GetCertFileName() { return CertFileName_; }
    [[nodiscard]] const std::string & GetKeyFileName() { return KeyFileName_; }
    [[nodiscard]] const std::string & GetCA() { return CAFileName_; }
    [[nodiscard]] const std::string & GetSerialNumberBase() { return SerialNumberBase_; }

    [[nodiscard]] uint64_t GetNumClients() const { return NumClients_; }

private:
    void StartSimulators();
    void StopSimulators();

    struct SimThread {
        Poco::Thread    Thread;
        Simulator       Sim;
        SimThread(uint Index, std::string SerialBase, uint NumClients):
            Sim(Index,std::move(SerialBase),NumClients) {};
    };

    std::vector<std::unique_ptr<SimThread>>   SimThreads_;
    bool                        helpRequested_ = false;
    bool                        DebugMode_ = false ;
    std::string                 URI_;
    std::string                 CertFileName_;
    std::string                 KeyFileName_;
    std::string                 CAFileName_;
    std::string                 SerialNumberBase_;
    std::string                 ConfigFileName_;
    std::string                 LogDir_;
    uint64_t                    NumClients_=0;
    uint64_t                    HealthCheckInterval_=0;
    uint64_t                    StateInterval_=0;
    uint64_t                    ReconnectInterval_=0;
    uint64_t                    KeepAliveInterval_=0;
    uint64_t                    ConfigChangePendingInterval_=0;
    uint64_t                    MaxThreads_=3;
    MyErrorHandler              AppErrorHandler_;
};

uCentralClientApp * App();


#endif //UCENTRAL_CLNT_UCENTRALCLIENTAPP_H
