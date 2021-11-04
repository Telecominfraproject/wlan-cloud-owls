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

namespace OpenWifi {
    class Simulator : public Poco::Runnable {
    public:

        Simulator(uint64_t Index,std::string SerialStart, uint64_t NumClients, Poco::Logger &L) :
        Index_(Index),
        SerialStart_(std::move(SerialStart)),
        NumClients_(NumClients),
        Logger_(L)
        {

        }

        void run() override;
        void stop();
        void Initialize( Poco::Logger & ClientLogger);

        void Cancel() { State_ = "cancel"; SocketReactorThread_.wakeUp(); }
        void Resume() { State_ = "running"; SocketReactorThread_.wakeUp(); }
        void Pause()  { State_ = "paused"; SocketReactorThread_.wakeUp(); }

    private:
        Poco::Logger                                            &Logger_;
        my_mutex                                                Mutex_;
        Poco::Net::SocketReactor                                Reactor_;
        std::map<std::string,std::shared_ptr<uCentralClient>>   Clients_;
        Poco::Thread                                            SocketReactorThread_;
        std::atomic_bool                                        Running_ = false;
        uint64_t                                                Index_;
        std::string                                             SerialStart_;
        uint64_t                                                NumClients_;
        CensusReport                                            CensusReport_;
        std::string                                             State_{"stopped"};
    };
}

#endif //UCENTRAL_CLNT_SIMULATOR_H
