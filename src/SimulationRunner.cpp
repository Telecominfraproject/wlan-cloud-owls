//
// Created by stephane bourque on 2021-03-13.
//
#include <random>
#include <thread>
#include <chrono>

#include <fmt/format.h>

#include <Poco/Logger.h>
#include <Poco/Net/NetException.h>
#include <Poco/Net/SSLException.h>
#include <Poco/NObserver.h>

#include "SimStats.h"
#include "SimulationRunner.h"
#include "UI_Owls_WebSocketNotifications.h"


namespace OpenWifi {
	void SimulationRunner::Start() {
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> distrib(5, 25);

        Running_ = true;
		std::lock_guard Lock(Mutex_);

        NumberOfReactors_ = Poco::Environment::processorCount() * 2;
        for(std::uint64_t  i=0;i<NumberOfReactors_;i++) {
            auto NewReactor = std::make_unique<Poco::Net::SocketReactor>();
            auto NewReactorThread = std::make_unique<Poco::Thread>();
            NewReactorThread->start(*NewReactor);
            SocketReactorPool_.push_back(std::move(NewReactor));
            SocketReactorThreadPool_.push_back(std::move(NewReactorThread));
        }

        std::uint64_t ReactorIndex=0;
		for (uint64_t i = 0; i < Details_.devices; i++) {
			char Buffer[32];
			snprintf(Buffer, sizeof(Buffer), "%s%05x0", Details_.macPrefix.c_str(), (unsigned int)i);
			auto Client = std::make_shared<OWLSclient>(Buffer, Logger_, this, *SocketReactorPool_[ReactorIndex++ % NumberOfReactors_]);
            Client->SerialNumber_ = Buffer;
            Client->Valid_ = true;
            Scheduler_.in(std::chrono::seconds(distrib(gen)), OWLSClientEvents::EstablishConnection, Client, this);
			Clients_[Buffer] = Client;
		}

        UpdateTimerCallback_ = std::make_unique<Poco::TimerCallback<SimulationRunner>>(
                *this, &SimulationRunner::onUpdateTimer);
        UpdateTimer_.setStartInterval(10000);
        UpdateTimer_.setPeriodicInterval(10 * 1000);
        UpdateTimer_.start(*UpdateTimerCallback_, MicroServiceTimerPool());
	}

    void SimulationRunner::onUpdateTimer([[maybe_unused]] Poco::Timer &timer) {
        if(Running_) {

            OWLSNotifications::SimulationUpdate_t Notification;
            SimStats()->GetCurrent(Id_, Notification.content, UInfo_);
            OWLSNotifications::SimulationUpdate(Notification);
            ++StatsUpdates_;

            if((StatsUpdates_ % 3) == 0) {
                std::lock_guard Lock(Mutex_);

                for(auto &client:Clients_) {
                    std::lock_guard   Guard(client.second->Mutex_);
                    if(client.second->Connected_) {
                        client.second->Update();
                    }
                }
            }
        }
    }

    void SimulationRunner::ProgressUpdate(SimulationRunner *sim) {
        if(sim->Running_) {
            OWLSNotifications::SimulationUpdate_t Notification;
            SimStats()->GetCurrent(sim->Id_, Notification.content, sim->UInfo_);
            OWLSNotifications::SimulationUpdate(Notification);
            // sim->Scheduler_.in(std::chrono::seconds(10), ProgressUpdate, sim);
        }
    }

	void SimulationRunner::Stop() {
		if (Running_) {
            Running_ = false;
            UpdateTimer_.stop();
            std::for_each(SocketReactorPool_.begin(),SocketReactorPool_.end(),[](auto &reactor) { reactor->stop(); });
            std::for_each(SocketReactorThreadPool_.begin(),SocketReactorThreadPool_.end(),[](auto &t){ t->join(); });
            SocketReactorThreadPool_.clear();
            SocketReactorPool_.clear();
            Clients_.clear();
		}
	}

    void SimulationRunner::OnSocketError(const Poco::AutoPtr<Poco::Net::ErrorNotification> &pNf) {
        std::lock_guard G(Mutex_);

        std::cout << "SimulationRunner::OnSocketError" << std::endl;

        auto socket = pNf->socket().impl()->sockfd();
        std::map<std::int64_t, std::shared_ptr<OWLSclient>>::iterator client_hint;
        std::shared_ptr<OWLSclient> client;

        {
            std::lock_guard GG(SocketFdMutex_);
            client_hint = Clients_fd_.find(socket);
            if (client_hint == end(Clients_fd_)) {
                pNf->socket().impl()->close();
                poco_warning(Logger_, fmt::format("{}: Invalid socket", socket));
                return;
            }
            client = client_hint->second;
        }

        {
            std::lock_guard Guard(client->Mutex_);
            client->Disconnect(__func__, Guard);
        }
        if (Running_) {
            OWLSClientEvents::Reconnect(client, this);
        }
    }

    void SimulationRunner::OnSocketShutdown(const Poco::AutoPtr<Poco::Net::ShutdownNotification> &pNf) {
        auto socket = pNf->socket().impl()->sockfd();
        std::shared_ptr<OWLSclient> client;
        {
            std::lock_guard G(SocketFdMutex_);
            auto client_hint = Clients_fd_.find(socket);
            if (client_hint == end(Clients_fd_)) {
                pNf->socket().impl()->close();
                poco_warning(Logger_, fmt::format("{}: Invalid socket", socket));
                return;
            }
            client = client_hint->second;
        }
        {
            std::lock_guard Guard(client->Mutex_);
            client->Disconnect(__func__ , Guard);
        }
        if(Running_)
            OWLSClientEvents::Reconnect(client,this);
    }

    void SimulationRunner::OnSocketReadable(const Poco::AutoPtr<Poco::Net::ReadableNotification> &pNf) {
        std::shared_ptr<OWLSclient> client;

        int socket;
        {
            std::lock_guard G(SocketFdMutex_);
            socket = pNf->socket().impl()->sockfd();
            auto client_hint = Clients_fd_.find(socket);
            if (client_hint == end(Clients_fd_)) {
                poco_warning(Logger_, fmt::format("{}: Invalid socket", socket));
                return;
            }
            client = client_hint->second;
        }

        std::lock_guard Guard(client->Mutex_);

        try {
            Poco::Buffer<char> IncomingFrame(0);
            int Flags;

            auto MessageSize = client->WS_->receiveFrame(IncomingFrame, Flags);
            auto Op = Flags & Poco::Net::WebSocket::FRAME_OP_BITMASK;

            if (MessageSize == 0 && Flags == 0 && Op == 0) {
                OWLSClientEvents::Disconnect(__func__, Guard, client, this, "Error while waiting for data in WebSocket", true);
                return;
            }
            IncomingFrame.append(0);

            switch (Op) {
                case Poco::Net::WebSocket::FRAME_OP_PING: {
                    client->WS_->sendFrame("", 0,
                                   Poco::Net::WebSocket::FRAME_OP_PONG |
                                   Poco::Net::WebSocket::FRAME_FLAG_FIN);
                } break;

                case Poco::Net::WebSocket::FRAME_OP_PONG: {
                } break;

                case Poco::Net::WebSocket::FRAME_OP_TEXT: {
                    if (MessageSize > 0) {
                        SimStats()->AddInMsg(Id_, MessageSize);
                        Poco::JSON::Parser  parser;
                        auto Frame = parser.parse(IncomingFrame.begin()).extract<Poco::JSON::Object::Ptr>();

                        if (Frame->has("jsonrpc") && Frame->has("id") &&
                            Frame->has("method") && Frame->has("params")) {
                            ProcessCommand(Guard,client, Frame);
                        } else {
                            Logger_.warning(
                                    fmt::format("MESSAGE({}): invalid incoming message.", client->SerialNumber_));
                        }
                    }
                } break;
                default: {
                } break;
            }
            return;
        } catch (const Poco::Net::SSLException &E) {
            Logger_.warning(
                    fmt::format("Exception({}): SSL exception: {}", client->SerialNumber_, E.displayText()));
        } catch (const Poco::Exception &E) {
            Logger_.warning(fmt::format("Exception({}): Generic exception: {}", client->SerialNumber_,
                                        E.displayText()));
        }
        OWLSClientEvents::Disconnect(__func__, Guard,client, this, "Error while waiting for data in WebSocket", true);
    }

    void SimulationRunner::ProcessCommand(std::lock_guard<std::mutex> &ClientGuard, const std::shared_ptr<OWLSclient> & Client, Poco::JSON::Object::Ptr Frame) {

        std::string Method = Frame->get("method");
        std::uint64_t Id = Frame->get("id");
        auto Params = Frame->getObject("params");

        if (Method == "configure") {
            CensusReport_.ev_configure++;
            std::thread     t(OWLSclient::DoConfigure,Client, Id, Params);
            t.detach();
        } else if (Method == "reboot") {
            CensusReport_.ev_reboot++;
            std::thread     t(OWLSclient::DoReboot, Client, Id, Params);
            t.detach();
        } else if (Method == "upgrade") {
            CensusReport_.ev_firmwareupgrade++;
            std::thread     t(OWLSclient::DoUpgrade, Client, Id, Params);
            t.detach();
        } else if (Method == "factory") {
            CensusReport_.ev_factory++;
            std::thread     t(OWLSclient::DoFactory, Client, Id, Params);
            t.detach();
        } else if (Method == "leds") {
            CensusReport_.ev_leds++;
            std::thread     t(OWLSclient::DoLEDs, Client, Id, Params);
            t.detach();
        } else {
            Logger_.warning(fmt::format("COMMAND({}): unknown method '{}'", Client->SerialNumber_, Method));
            Client->UnSupportedCommand(ClientGuard,Client, Id, Method);
        }

    }

} // namespace OpenWifi