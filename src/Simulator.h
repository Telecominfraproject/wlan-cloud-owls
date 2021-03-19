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

    Simulator() :
        Stop_(false)
    {

    }

    void run() override;
    void stop() { Stop_ = true; }

private:
    static Simulator * instance_;
    std::mutex mutex_;
    Poco::Net::SocketReactor                                Reactor_;
    std::map<std::string,std::shared_ptr<uCentralClient>>   Clients_;
    Poco::Thread                                            SocketReactorThread_;
    volatile bool                                           Stop_;
};


#endif //UCENTRAL_CLNT_SIMULATOR_H
