//
// Created by stephane bourque on 2021-03-13.
//
#include <thread>
#include <random>

#include "Poco/Logger.h"
#include "Poco/Message.h"

#include "Simulator.h"
#include "uCentralEvent.h"
#include "SimStats.h"

namespace OpenWifi {
    void Simulator::Initialize(Poco::Logger &ClientLogger) {
        std::random_device  rd;
        std::mt19937        gen(rd());
        std::uniform_int_distribution<> distrib(1, 15);
        std::cout << __func__ << " : " << __LINE__ << std::endl;

        std::lock_guard Lock(Mutex_);

        for(auto i=0;i<NumClients_;i++)
        {
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            char Buffer[32];
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            snprintf(Buffer,sizeof(Buffer),"%s%02x%04x",SerialStart_.c_str(),(unsigned int)Index_,i);
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            auto Client = std::make_shared<uCentralClient>( Reactor_,
                                                            Buffer,
                                                            ClientLogger);
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            Client->AddEvent(ev_reconnect, distrib(gen) );
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            Clients_[Buffer] = std::move(Client);
            std::cout << __func__ << " : " << __LINE__ << std::endl;
        }
    }



    void Simulator::run() {

        SocketReactorThread_.start(Reactor_);

        Logger_.setLevel(Poco::Message::PRIO_NOTICE);
        Logger_.notice(Poco::format("Starting reactor %Lu...",Index_));

        while(!Stop_)
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
                            Logger_.information(Poco::format("reconnect(%s): ", Client->Serial()));
                            std::thread T([Client]() {
                                Client->NextEvent(true);
                                Client->EstablishConnection();
                            });
                            T.detach();
                        }
                        break;

                        case ev_connect: {
                            Logger_.information(Poco::format("connect(%s): ", Client->Serial()));
                            std::thread T([Client]() {
                                Client->NextEvent(true);
                                ConnectEvent E(Client);
                                E.Send();
                            });
                            T.detach();
                        }
                        break;

                        case ev_healthcheck: {
                            Logger_.information(Poco::format("healthcheck(%s): ", Client->Serial()));
                            std::thread T([Client]() {
                                Client->NextEvent(true);
                                HealthCheckEvent E(Client);
                                E.Send();
                            });
                            T.detach();
                        }
                        break;

                        case ev_state: {
                            Logger_.information(Poco::format("state(%s): ", Client->Serial()));
                            std::thread T([Client]() {
                                Client->NextEvent(true);
                                StateEvent E(Client);
                                E.Send();
                            });
                            T.detach();
                        }
                        break;

                        case ev_log: {
                            Logger_.information(Poco::format("log(%s): ", Client->Serial()));
                            std::thread T([Client]() {
                                Client->NextEvent(true);
                                LogEvent E(Client, std::string("log"), 2);
                                E.Send();
                            });
                            T.detach();
                        }
                        break;

                        case ev_crashlog: {
                            Logger_.information(Poco::format("crash-log(%s): ", Client->Serial()));
                            std::thread T([Client]() {
                                Client->NextEvent(true);
                                CrashLogEvent E(Client);
                                E.Send();
                            });
                            T.detach();
                        }
                        break;

                        case ev_configpendingchange: {
                            Logger_.information(Poco::format("pendingchange(%s): ", Client->Serial()));
                            std::thread T([Client]() {
                                Client->NextEvent(true);
                                ConfigChangePendingEvent E(Client);
                                E.Send();
                            });
                            T.detach();
                        }
                        break;

                        case ev_keepalive: {
                            Logger_.information(Poco::format("keepalive(%s): ", Client->Serial()));
                            std::thread T([Client]() {
                                Client->NextEvent(true);
                                KeepAliveEvent E(Client);
                                E.Send();
                            });
                            T.detach();
                        }
                        break;

                        case ev_reboot: {
                            Logger_.information(Poco::format("reboot(%s): ", Client->Serial()));
                            std::thread T([Client]() {
                                Client->NextEvent(true);
                                RebootEvent E(Client);
                                E.Send();
                            });
                            T.detach();
                        }
                        break;

                        case ev_disconnect: {
                            Logger_.information(Poco::format("disconnect(%s): ", Client->Serial()));
                            std::thread T([Client]() {
                                Client->NextEvent(true);
                                DisconnectEvent E(Client);
                                E.Send();
                            });
                            T.detach();
                        }
                        break;

                        case ev_wsping: {
                            Logger_.information(Poco::format("wp-ping(%s): ", Client->Serial()));
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
            } catch ( const Poco::Exception & E) {
                Logger_.warning(Poco::format("SIMULATOR(%Lu): Crashed. Poco exception:%s",Index_,E.displayText()));
            } catch ( const std::exception & E ) {
                std::string S = E.what();
                Logger_.warning(Poco::format("SIMULATOR(%Lu): Crashed. std::exception:%s",Index_,S));
            }
        }

        for(auto &[Key,Client]:Clients_)
            Client->Terminate();

        Reactor_.stop();
        SocketReactorThread_.join();
        Logger_.notice(Poco::format("Stopped reactor %Lu...",Index_));
    }
}