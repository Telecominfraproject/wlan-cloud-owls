//
// Created by stephane bourque on 2021-03-13.
//
#include <thread>
#include <random>

#include "Poco/Logger.h"

#include "Simulator.h"
#include "uCentralEvent.h"
#include "SimStats.h"

#include "fmt/format.h"

#include "UI_Owls_WebSocketNotifications.h"

namespace OpenWifi {
    void Simulator::Initialize(/*Poco::Logger &ClientLogger*/) {
        std::random_device  rd;
        std::mt19937        gen(rd());
        std::uniform_int_distribution<> distrib(1, 15);

        std::lock_guard Lock(Mutex_);

        for(uint64_t i=0;i<NumClients_;i++)
        {
            char Buffer[32];
            snprintf(Buffer,sizeof(Buffer),"%s%02x%03x0",SerialStart_.c_str(),(unsigned int)Index_,(unsigned int)i);
            auto Client = std::make_shared<uCentralClient>( Reactor_,
                                                            Buffer,
                                                            Logger_);
            Client->AddEvent(ev_reconnect, distrib(gen) );
            Clients_[Buffer] = std::move(Client);
        }
    }

    void Simulator::stop() {
        if(Running_) {
            Running_ = false;
            Reactor_.stop();
            SocketReactorThread_.join();
        }
    }

    void Simulator::run() {
        Logger_.notice(fmt::format("Starting reactor {}...",Index_));
        Running_ = true;
        SocketReactorThread_.start(Reactor_);

        while(Running_)
        {
            //  wake up every quarter second
            Poco::Thread::sleep(1000);

            if(State_=="paused")
                continue;

            if(State_=="cancel")
                break;

            my_guard Lock(Mutex_);

            try {
                CensusReport_.Reset();
                for (const auto &i:Clients_)
                    i.second->DoCensus(CensusReport_);

                for (const auto &i:Clients_) {
                    auto Client = i.second;
                    auto Event = Client->NextEvent(false);

                    switch (Event) {
                        case ev_none: {
                            // nothing to do
                        }
                        break;

                        case ev_reconnect: {
                            Logger_.information(fmt::format("reconnect({}): ", Client->Serial()));
                            std::thread T([Client]() {
                                Client->NextEvent(true);
                                Client->EstablishConnection();
                            });
                            T.detach();
                        }
                        break;

                        case ev_connect: {
                            Logger_.information(fmt::format("connect({}): ", Client->Serial()));
                            std::thread T([Client]() {
                                Client->NextEvent(true);
                                ConnectEvent E(Client);
                                E.Send();
                            });
                            T.detach();
                        }
                        break;

                        case ev_healthcheck: {
                            Logger_.information(fmt::format("healthcheck({}): ", Client->Serial()));
                            std::thread T([Client]() {
                                Client->NextEvent(true);
                                HealthCheckEvent E(Client);
                                E.Send();
                            });
                            T.detach();
                        }
                        break;

                        case ev_state: {
                            Logger_.information(fmt::format("state({}): ", Client->Serial()));
                            std::thread T([Client]() {
                                Client->NextEvent(true);
                                StateEvent E(Client);
                                E.Send();
                            });
                            T.detach();
                        }
                        break;

                        case ev_log: {
                            Logger_.information(fmt::format("log({}): ", Client->Serial()));
                            std::thread T([Client]() {
                                Client->NextEvent(true);
                                LogEvent E(Client, std::string("log"), 2);
                                E.Send();
                            });
                            T.detach();
                        }
                        break;

                        case ev_crashlog: {
                            Logger_.information(fmt::format("crash-log({}): ", Client->Serial()));
                            std::thread T([Client]() {
                                Client->NextEvent(true);
                                CrashLogEvent E(Client);
                                E.Send();
                            });
                            T.detach();
                        }
                        break;

                        case ev_configpendingchange: {
                            Logger_.information(fmt::format("pendingchange({}): ", Client->Serial()));
                            std::thread T([Client]() {
                                Client->NextEvent(true);
                                ConfigChangePendingEvent E(Client);
                                E.Send();
                            });
                            T.detach();
                        }
                        break;

                        case ev_keepalive: {
                            Logger_.information(fmt::format("keepalive({}): ", Client->Serial()));
                            std::thread T([Client]() {
                                Client->NextEvent(true);
                                KeepAliveEvent E(Client);
                                E.Send();
                            });
                            T.detach();
                        }
                        break;

                        case ev_reboot: {
                            Logger_.information(fmt::format("reboot({}): ", Client->Serial()));
                            std::thread T([Client]() {
                                Client->NextEvent(true);
                                RebootEvent E(Client);
                                E.Send();
                            });
                            T.detach();
                        }
                        break;

                        case ev_disconnect: {
                            Logger_.information(fmt::format("disconnect({}): ", Client->Serial()));
                            std::thread T([Client]() {
                                Client->NextEvent(true);
                                DisconnectEvent E(Client);
                                E.Send();
                            });
                            T.detach();
                        }
                        break;

                        case ev_wsping: {
                            Logger_.information(fmt::format("ws-ping({}): ", Client->Serial()));
                            std::thread T([Client]() {
                                Client->NextEvent(true);
                                WSPingEvent E(Client);
                                E.Send();
                            });
                            T.detach();
                        }
                        break;
                    }
                }

                OWLSNotifications::SimulationUpdate_t Notification;
                SimStats()->GetCurrent(Notification.content);
                OWLSNotifications::SimulationUpdate(Notification);

            } catch ( const Poco::Exception & E) {
                Logger_.warning(fmt::format("SIMULATOR({}): Crashed. Poco exception:{}",Index_,E.displayText()));
            } catch ( const std::exception & E ) {
                std::string S = E.what();
                Logger_.warning(fmt::format("SIMULATOR({}): Crashed. std::exception:{}",Index_,S));
            }
        }

        for(auto &[Key,Client]:Clients_) {
            Client->Disconnect("Simulation termination", false);
        }

        Clients_.clear();
        Logger_.notice(fmt::format("Stopped reactor {}...",Index_));
    }
}