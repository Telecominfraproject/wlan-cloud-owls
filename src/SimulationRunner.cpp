//
// Created by stephane bourque on 2021-03-13.
//
#include <random>
#include <thread>
#include <chrono>

#include "Poco/Logger.h"

#include "SimStats.h"
#include "SimulationRunner.h"
#include "OWLSevent.h"

#include "fmt/format.h"

#include "UI_Owls_WebSocketNotifications.h"

#include <Poco/Net/NetException.h>
#include <Poco/Net/SSLException.h>
#include <Poco/NObserver.h>

namespace OpenWifi {
	void SimulationRunner::Start() {
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> distrib(5, 25);

        Running_ = true;
		std::lock_guard Lock(Mutex_);

        SocketReactorThread_.start(Reactor_);
		for (uint64_t i = 0; i < Details_.devices; i++) {
			char Buffer[32];
			snprintf(Buffer, sizeof(Buffer), "%s%05x0", Details_.macPrefix.c_str(), (unsigned int)i);
			auto Client = std::make_shared<OWLSclient>(Buffer, Logger_, this);
            Client->SerialNumber_ = Buffer;
            Client->Valid_ = true;
            Scheduler_.in(std::chrono::seconds(distrib(gen)), OWLSclientEvents::EstablishConnection, Client, this);
			Clients_[Buffer] = Client;
		}
        Scheduler_.in(std::chrono::seconds(10), ProgressUpdate, this);

	}

    void SimulationRunner::ProgressUpdate(SimulationRunner *sim) {
        if(sim->Running_) {
//            std::cout << "Progress update..." << std::endl;
            OWLSNotifications::SimulationUpdate_t Notification;
            SimStats()->GetCurrent(sim->Id_, Notification.content);
            OWLSNotifications::SimulationUpdate(Notification);
            sim->Scheduler_.in(std::chrono::seconds(10), ProgressUpdate, sim);
        }
    }

	void SimulationRunner::Stop() {
		if (Running_) {
            Running_ = false;
            for(auto &client:Clients_) {
                OWLSclientEvents::Disconnect(client.second, this, "Simulation shutting down", false);
                client.second->Valid_ = false;
            }
			Reactor_.stop();
			SocketReactorThread_.join();
            Clients_.clear();
		}
	}

    void SimulationRunner::OnSocketError(const Poco::AutoPtr<Poco::Net::ErrorNotification> &pNf) {
        std::lock_guard G(Mutex_);

        auto socket = pNf->socket().impl()->sockfd();
        std::map<std::int64_t, std::shared_ptr<OWLSclient>>::iterator client_hint;
        std::shared_ptr<OWLSclient> client;

        client_hint = Clients_fd_.find(socket);
        if (client_hint == end(Clients_fd_)) {
            poco_warning(Logger_, fmt::format("{}: Invalid socket", socket));
            return;
        }
        client = client_hint->second;
        client->Disconnect();
        Reactor_.removeEventHandler(
                *client->WS_, Poco::NObserver<SimulationRunner, Poco::Net::ReadableNotification>(
                        *this, &SimulationRunner::OnSocketReadable));
        Reactor_.removeEventHandler(
                *client->WS_, Poco::NObserver<SimulationRunner, Poco::Net::ErrorNotification>(
                        *this, &SimulationRunner::OnSocketError));
        Reactor_.removeEventHandler(
                *client->WS_, Poco::NObserver<SimulationRunner, Poco::Net::ShutdownNotification>(
                        *this, &SimulationRunner::OnSocketShutdown));
        client->fd_ = -1;
        if(Running_)
            OWLSclientEvents::Reconnect(client,this);
    }

    void SimulationRunner::OnSocketShutdown(const Poco::AutoPtr<Poco::Net::ShutdownNotification> &pNf) {
        std::lock_guard G(Mutex_);

        auto socket = pNf->socket().impl()->sockfd();
        std::map<std::int64_t, std::shared_ptr<OWLSclient>>::iterator client_hint;
        std::shared_ptr<OWLSclient> client;

        client_hint = Clients_fd_.find(socket);
        if (client_hint == end(Clients_fd_)) {
            poco_warning(Logger_, fmt::format("{}: Invalid socket", socket));
            return;
        }
        client = client_hint->second;
        client->Disconnect();
        if(Running_)
            OWLSclientEvents::Reconnect(client,this);
    }

    void SimulationRunner::OnSocketReadable(const Poco::AutoPtr<Poco::Net::ReadableNotification> &pNf) {
        std::map<std::int64_t, std::shared_ptr<OWLSclient>>::iterator client_hint;
        std::shared_ptr<OWLSclient> client;

        DEBUG_LINE("got some data")
        int socket;
        {
            std::lock_guard G(Mutex_);

            DEBUG_LINE("locked and loaded")
            socket = pNf->socket().impl()->sockfd();
            client_hint = Clients_fd_.find(socket);
            if (client_hint == end(Clients_fd_)) {
                poco_warning(Logger_, fmt::format("{}: Invalid socket", socket));
                return;
            }
            client = client_hint->second;
        }

        DEBUG_LINE("found data")

        std::lock_guard Guard(client->Mutex_);

        try {
            Poco::Buffer<char> IncomingFrame(0);
            int Flags;

            auto MessageSize = client->WS_->receiveFrame(IncomingFrame, Flags);
            auto Op = Flags & Poco::Net::WebSocket::FRAME_OP_BITMASK;

            if (MessageSize == 0 && Flags == 0 && Op == 0) {
                Clients_fd_.erase(socket);
                OWLSclientEvents::Disconnect(client, this, "Error while waiting for data in WebSocket", true);
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
                            ProcessCommand(client, Frame);
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

        Clients_fd_.erase(socket);
        OWLSclientEvents::Disconnect(client, this, "Error while waiting for data in WebSocket", true);
    }

    void SimulationRunner::ProcessCommand(std::shared_ptr<OWLSclient> Client, Poco::JSON::Object::Ptr Frame) {

        std::string Method = Frame->get("method");
        std::uint64_t Id = Frame->get("id");
        auto Params = Frame->getObject("params");

        if (Method == "configure") {
            CensusReport_.ev_configure++;
            Client->DoConfigure(Client, Id, Params);
        } else if (Method == "reboot") {
            Client->DoReboot(Client, Id, Params);
            CensusReport_.ev_reboot++;
        } else if (Method == "upgrade") {
            Client->DoUpgrade(Client, Id, Params);
            CensusReport_.ev_firmwareupgrade++;
        } else if (Method == "factory") {
            Client->DoFactory(Client, Id, Params);
            CensusReport_.ev_factory++;
        } else if (Method == "leds") {
            CensusReport_.ev_leds++;
            Client->DoLEDs(Client, Id, Params);
        } else {
            Logger_.warning(fmt::format("COMMAND({}): unknown method '{}'", Client->SerialNumber_, Method));
        }

    }

} // namespace OpenWifi