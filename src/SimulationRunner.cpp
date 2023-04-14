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
#include "OWLSscheduler.h"

#include <Poco/Net/NetException.h>
#include <Poco/Net/SSLException.h>

namespace OpenWifi {
	void SimulationRunner::Start() {
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> distrib(3, 15);

        std::cout << __LINE__ << std::endl;
		std::lock_guard Lock(Mutex_);

        std::cout << __LINE__ << std::endl;
		for (uint64_t i = 0; i < Details_.devices; i++) {
			char Buffer[32];
            std::cout << __LINE__ << std::endl;
			snprintf(Buffer, sizeof(Buffer), "%s%05x0", Details_.macPrefix.c_str(), (unsigned int)i);
            std::cout << __LINE__ << std::endl;

			auto Client = std::make_shared<OWLSclient>(Reactor_, Buffer, Logger_, this);
            std::cout << __LINE__ << std::endl;
            Client->SerialNumber_ = Buffer;
            std::cout << __LINE__ << std::endl;
            // OWLSscheduler()->Ref().in(std::chrono::seconds(distrib(gen)), OWLSclientEvents::EstablishConnection, Client, this);
			Clients_[Buffer] = Client;
            std::cout << __LINE__ << std::endl;
		}
        std::cout << __LINE__ << std::endl;
        OWLSscheduler()->Ref().in(std::chrono::seconds(10), ProgressUpdate, this);
        std::cout << __LINE__ << std::endl;
	}

    void SimulationRunner::ProgressUpdate(SimulationRunner *sim) {
        if(sim->Running_) {
            OWLSNotifications::SimulationUpdate_t Notification;
            SimStats()->GetCurrent(sim->Id_, Notification.content);
            OWLSNotifications::SimulationUpdate(Notification);
            OWLSscheduler()->Ref().in(std::chrono::seconds(10), ProgressUpdate, sim);
        }
    }

	void SimulationRunner::Stop() {
		if (Running_) {
			Running_ = false;
			Reactor_.stop();
			SocketReactorThread_.join();
            for(auto &client:Clients_) {
                OWLSclientEvents::Disconnect(client.second, this, "Simulation shutting down", false);
                client.second->Valid_ = false;
            }
            Clients_.clear();
		}
	}

    void SimulationRunner::OnSocketReadable(const Poco::AutoPtr<Poco::Net::ReadableNotification> &pNf) {
        std::lock_guard G(Mutex_);

        auto socket = pNf->socket().impl()->sockfd();
        auto client_hint = Clients_fd_.find(socket);
        if(client_hint==end(Clients_fd_)) {
            poco_warning(Logger_,fmt::format("{}: Invalid socket", socket));
            return;
        }

        auto client = client_hint->second;

        try {
            char Message[16000];
            int Flags;

            auto MessageSize = client->WS_->receiveFrame(Message, sizeof(Message), Flags);
            auto Op = Flags & Poco::Net::WebSocket::FRAME_OP_BITMASK;

            if (MessageSize == 0 && Flags == 0 && Op == 0) {
                Clients_fd_.erase(socket);
                OWLSclientEvents::Disconnect(client, this, "Error while waiting for data in WebSocket", true);
                return;
            }

            Message[MessageSize] = 0;
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
                        SimStats()->AddRX(Id_,MessageSize);
                        SimStats()->AddInMsg(Id_);
                        auto Vars = nlohmann::json::parse(Message);

                        if (Vars.contains("jsonrpc") && Vars.contains("id") &&
                            Vars.contains("method") && Vars.contains("params")) {
                            ProcessCommand(client, Vars);
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

    void SimulationRunner::ProcessCommand(std::shared_ptr<OWLSclient> Client, nlohmann::json &Vars) {

        std::string Method = Vars["method"];

        auto Id = Vars["id"];
        auto Params = Vars["params"];

        if (Method == "configure") {
            CensusReport_.ev_configure++;
            Client->DoConfigure(Id, Params);
        } else if (Method == "reboot") {
            Client->DoReboot(Id, Params);
            CensusReport_.ev_reboot++;
            OWLSclientEvents::Disconnect(Client,this,"Performing reboot", true);
        } else if (Method == "upgrade") {
            Client->DoUpgrade(Id, Params);
            CensusReport_.ev_firmwareupgrade++;
            OWLSclientEvents::Disconnect(Client,this,"Performing firmware upgrade", true);
        } else if (Method == "factory") {
            Client->DoFactory(Id, Params);
            CensusReport_.ev_factory++;
            OWLSclientEvents::Disconnect(Client,this,"Performing factory reset", true);
        } else if (Method == "leds") {
            CensusReport_.ev_leds++;
            Client->DoLEDs(Id, Params);
        } else if (Method == "perform") {
            CensusReport_.ev_perform++;
            Client->DoPerform(Id, Params);
        } else if (Method == "trace") {
            CensusReport_.ev_trace++;
            Client->DoTrace(Id, Params);
        } else {
            Logger_.warning(fmt::format("COMMAND({}): unknown method '{}'", Client->SerialNumber_, Method));
        }

    }

} // namespace OpenWifi