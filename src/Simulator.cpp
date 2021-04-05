//
// Created by stephane bourque on 2021-03-13.
//
#include <thread>
#include <random>

#include "Poco/Logger.h"

#include "Simulator.h"
#include "uCentralClientApp.h"
#include "uCentralEvent.h"

Simulator *Simulator::instance_ = nullptr;

void Simulator::Initialize() {
    std::random_device  rd;
    std::mt19937        gen(rd());
    std::uniform_int_distribution<> distrib(1, 15);

    Poco::Logger    & Logger_ = App()->logger();

    for(auto i=0;i<NumClients_;i++)
    {
        char Buffer[32];
        snprintf(Buffer,sizeof(Buffer),"%s%02x%04x",SerialStart_.c_str(),(unsigned int)Index_,i);
        auto Client = std::make_shared<uCentralClient>( Reactor_,
                                                        Buffer,
                                                        App()->GetURI(),
                                                        App()->GetKeyFileName(),
                                                        App()->GetCertFileName(),
                                                        uCentralClientApp::instance().logger());
        Client->AddEvent(ev_reconnect, distrib(gen) );
        Clients_[Buffer] = std::move(Client);
    }
}

void Simulator::run() {

    Poco::Logger    & Logger_ = App()->logger();

    SocketReactorThread_.start(Reactor_);

    while(!Stop_)
    {
        //  wake up every quarter second
        Poco::Thread::sleep(1000);
        auto Now = time(nullptr);

        // Logger_.information("Looking for new events...");

        for(const auto & i:Clients_)
        {
            auto Client = i.second;
            auto Event = Client->NextEvent();

            switch(Event)
            {
                case ev_none: {
                        // nothing to do
                    }
                    break;

                case ev_reconnect: {
                        Logger_.information(Poco::format("reconnect(%s): ",Client->Serial()));
                        std::thread T([Client]() {
                            Client->EstablishConnection();
                        });
                        T.detach();
                    }
                    break;

                case ev_connect: {
                    Logger_.information(Poco::format("connect(%s): ",Client->Serial()));
                    std::thread T([Client]() {
                            ConnectEvent    E(Client);
                            E.Send(); });
                        T.detach();
                    }
                    break;

                case ev_healthcheck: {
                    Logger_.information(Poco::format("healthcheck(%s): ",Client->Serial()));
                    std::thread T([Client]() {
                            HealthCheckEvent    E(Client);
                            E.Send(); });
                        T.detach();
                    }
                    break;

                case ev_state: {
                    Logger_.information(Poco::format("state(%s): ",Client->Serial()));
                    std::thread T([Client]() {
                            StateEvent    E(Client);
                            E.Send(); });
                        T.detach();
                    }
                    break;

                case ev_log: {
                    Logger_.information(Poco::format("log(%s): ",Client->Serial()));
                    std::thread T([Client]() {
                            LogEvent    E(Client,std::string("log"),2);
                            E.Send(); });
                        T.detach();
                    }
                    break;

                case ev_crashlog:  {
                    Logger_.information(Poco::format("crash-log(%s): ",Client->Serial()));
                    std::thread T([Client]() {
                            CrashLogEvent E(Client);
                            E.Send(); });
                        T.detach();
                    }
                    break;

                case ev_configpendingchange: {
                    Logger_.information(Poco::format("pendingchange(%s): ",Client->Serial()));
                    std::thread T([Client]() {
                        ConfigChangePendingEvent E(Client);
                        E.Send(); });
                        T.detach();
                    }
                    break;

                case ev_keepalive: {
                    Logger_.information(Poco::format("keepalive(%s): ",Client->Serial()));
                    std::thread T([Client]() {
                            KeepAliveEvent    E(Client);
                            E.Send(); });
                        T.detach();
                    }
                    break;

                case ev_reboot: {
                    Logger_.information(Poco::format("reboot(%s): ",Client->Serial()));
                    std::thread T([Client]() {
                            RebootEvent    E(Client);
                            E.Send(); });
                        T.detach();
                    }
                    break;

                case ev_disconnect: {
                    Logger_.information(Poco::format("disconnect(%s): ",Client->Serial()));
                    std::thread T([Client]() {
                            DisconnectEvent    E(Client);
                            E.Send(); });
                        T.detach();
                    }
                    break;

                case ev_wsping: {
                    Logger_.information(Poco::format("wp-ping(%s): ",Client->Serial()));
                    std::thread T([Client]() {
                            WSPingEvent    E(Client);
                            E.Send(); });
                        T.detach();
                    }
                    break;
            }
        }
    }

    for(auto &[Key,Client]:Clients_)
        Client->Terminate();

    Reactor_.stop();
    SocketReactorThread_.join();
    Logger_.information(Poco::format("Stopped reactor %Lu...",Index_));
}
