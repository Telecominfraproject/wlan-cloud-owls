//
// Created by stephane bourque on 2021-03-13.
//

#include "Simulator.h"

#include "uCentralClientApp.h"
#include <thread>
#include "Poco/Logger.h"

Simulator *Simulator::instance_ = nullptr;

void Connect(std::shared_ptr<uCentralClient> C)
{
    C->Connect();
}

void SendState(std::shared_ptr<uCentralClient> C)
{
    C->SendState();
}

void SendHealthCheck(std::shared_ptr<uCentralClient> C)
{
    C->SendHealthCheck();
}

void SendConnection( std::shared_ptr<uCentralClient> C)
{
    C->SendConnection();
}

void SendClosing( std::shared_ptr<uCentralClient> C)
{
    C->SendClosing();
}

void Simulator::run() {

    for(auto i=0;i<Service()->GetNumClients();i++)
    {
        std::string Serial = Service()->GetSerialNumberBase() + std::to_string(i);
        auto Client = std::shared_ptr<uCentralClient>(new uCentralClient(Reactor_,
                                                                         Serial,
                                                                         Service()->GetURI(),
                                                                         Service()->GetKeyFileName(),
                                                                         Service()->GetCertFileName(),
                                                                         uCentralClientApp::instance().logger()));
        Clients_[Serial] = Client;
    }

    SocketReactorThread_.start(Reactor_);

    while(!Stop_)
    {
        //  wake up every quarter second
        Poco::Thread::sleep(250);

        {
            std::lock_guard<std::mutex> guard(mutex_);

            auto Now = time(nullptr);

            for( const auto &[SerialNumber,Client] : Clients_ )
            {
                switch( Client->GetState() )
                {
                    case uCentralClient::initialized:
                        {
                            std::thread T(Connect,Client);
                            T.detach();
                        }
                        break;
                    case uCentralClient::connecting:
                        {

                        }
                        break;
                    case uCentralClient::connected:
                        {
                            std::thread T(SendConnection,Client);
                            T.detach();
                        }
                        break;
                    case uCentralClient::sending_hello:
                        {

                        }
                        break;
                    case uCentralClient::running:
                        {
                            if(Client->GetNextCheck()<Now)
                            {
                                std::thread T(SendHealthCheck,Client);
                                T.detach();
                            }
                            else if(Client->GetNextState()<Now)
                            {
                                std::thread T(SendState,Client);
                                T.detach();
                            }
                        }
                        break;
                    case uCentralClient::closing:
                        {
                            std::thread T(SendClosing,Client);
                            T.detach();
                        }
                        break;
                }
            }
        }
        Poco::Thread::sleep(1000);
    }

    for(auto &[Key,Client]:Clients_)
        Client->Terminate();

    Reactor_.stop();
}
