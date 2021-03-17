//
// Created by stephane bourque on 2021-03-13.
//

#include "Simulator.h"

#include "uCentralClientApp.h"

Simulator *Simulator::instance_ = nullptr;

void Simulator::run() {

    initialize();

    for(auto i=0;i<NumClients_;i++)
    {
        std::string Serial = SerialNumberBase_ + std::to_string(i);
        auto Client = std::shared_ptr<uCentralClient>(new uCentralClient(Reactor_,Serial,URI_,KeyFileName_,CertFileName_,uCentralClient::legacy));
        Clients_[Serial] = Client;
    }

    SocketReactorThread_.start(Reactor_);

    for(auto &[SerialNumber,Client]:Clients_)
        CommandList_[SerialNumber] = std::make_pair(0,Command::connect);

    while(!stop_)
    {
        {
            std::lock_guard<std::mutex> guard(mutex_);

            auto Now = time(nullptr);

            std::set<std::string>   ToBeRemoved;

            typedef void(*FunctionPointer)();

            std::vector<FunctionPointer>  Functions;

            for( auto &[Serial, Action] : CommandList_ )
            {
                auto &[Time,Cmd] = Action;

                if(Time==0 || Time<Now)
                {
                    auto Client = Clients_[Serial];
                    ToBeRemoved.insert(Serial);
                    switch(Cmd) {
                        case Command::send_heartbeat:
                            {
                                Client->SendHeartBeat();
                                Functions.push_back(Client->SendHeartBeat);
                            }
                            break;
                        case Command::reconnect:
                            {
                                Client->Connect();
                            }
                            break;
                        case Command::send_state:
                            {
                                Client->SendState();
                            }
                            break;
                        case Command::connect:
                            {
                                Client->Connect();
                            }
                            break;
                    }
                }

                for( const auto & Serial : ToBeRemoved )
                    CommandList_.erase(Serial);

                std::cout << "Serial: " << Serial << " time: " << Time << " Command: " << Cmd << std::endl;
            }
        }

        Poco::Thread::sleep(1000);
    }

    for(auto &[Key,Client]:Clients_)
        Client->Terminate();

    Reactor_.stop();
}

void Simulator::Reconnect(const std::string &Serial) {
    std::lock_guard<std::mutex> guard(mutex_);
    CommandList_[Serial] = std::make_pair(time(nullptr)+10,Command::reconnect);
}

void Simulator::SendState(const std::string &Serial) {
    std::lock_guard<std::mutex> guard(mutex_);
    CommandList_[Serial] = std::make_pair(time(nullptr)+30,Command::send_state);
}

void Simulator::HeartBeat(const std::string &Serial) {
    std::lock_guard<std::mutex> guard(mutex_);
    CommandList_[Serial] = std::make_pair(time(nullptr)+2,Command::send_heartbeat);
}

void Simulator::initialize() {
    CertFileName_ = Poco::Path::expand(uCentralClientApp::instance().config().getString("ucentral.simulation.certfile"));
    KeyFileName_ = Poco::Path::expand(uCentralClientApp::instance().config().getString("ucentral.simulation.keyfile"));
    URI_ = uCentralClientApp::instance().config().getString("ucentral.simulation.uri");
    NumClients_ = uCentralClientApp::instance().config().getInt64("ucentral.simulation.maxclients");
    SerialNumberBase_ = uCentralClientApp::instance().config().getString("ucentral.simulation.serialbase");
}
