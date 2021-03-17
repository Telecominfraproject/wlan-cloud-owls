//
// Created by stephane bourque on 2021-03-13.
//

#ifndef UCENTRAL_CLNT_SIMULATOR_H
#define UCENTRAL_CLNT_SIMULATOR_H
#include <string>
#include <map>
#include <set>

#include "Poco/Thread.h"

#include "uCentralClient.h"

class Simulator : public Poco::Runnable {
public:

    enum Command {
        connect,
        send_state,
        reconnect,
        send_heartbeat
    };

    Simulator() :
        stop_(false)
    {

    }

    void run();
    void Reconnect(const std::string & Serial);
    void HeartBeat(const std::string & Serial);
    void SendState(const std::string & Serial);

    void initialize();

    static Simulator * instance() {
        if(instance_== nullptr)
            instance_ = new Simulator;
        return instance_;
    }

    void stop() { stop_ = true; }

private:
    static Simulator * instance_;
    volatile bool stop_;
    std::mutex mutex_;
    std::map<std::string,std::shared_ptr<uCentralClient>>   Clients_;
    std::map<std::string,std::pair<uint64_t,Command>>       CommandList_;
    std::string                                             URI_;
    std::string                                             CertFileName_;
    std::string                                             KeyFileName_;
    std::string                                             SerialNumberBase_;
    uint64_t                                                NumClients_;
    Poco::Net::SocketReactor                                Reactor_;
    Poco::Thread                                            SocketReactorThread_;
};


#endif //UCENTRAL_CLNT_SIMULATOR_H
