//
// Created by stephane bourque on 2023-04-12.
//
#include "OWLSclient.h"
#include "SimulationRunner.h"
#include "SimulationCoordinator.h"
#include <fmt/format.h>
#include "SimStats.h"
#include <Poco/NObserver.h>

#include "OWLSclientEvents.h"
#include "OWLSevent.h"
#include "OWLS_utils.h"

namespace OpenWifi::OWLSclientEvents {

    void Disconnect(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner, const std::string &Reason, bool Reconnect) {
        std::lock_guard G(Client->Mutex_);

        if(Client->Valid_) {
            Runner->Report().ev_disconnect++;
            if (Client->Connected_) {
                Runner->RemoveClientFd(Client->fd_);
                Client->fd_ = -1;
                Runner->Reactor().removeEventHandler(
                        *Client->WS_, Poco::NObserver<SimulationRunner, Poco::Net::ReadableNotification>(
                                *Client->Runner_, &SimulationRunner::OnSocketReadable));
                Runner->Reactor().removeEventHandler(
                        *Client->WS_, Poco::NObserver<SimulationRunner, Poco::Net::ErrorNotification>(
                                *Client->Runner_, &SimulationRunner::OnSocketError));
                Runner->Reactor().removeEventHandler(
                        *Client->WS_, Poco::NObserver<SimulationRunner, Poco::Net::ShutdownNotification>(
                                *Client->Runner_, &SimulationRunner::OnSocketShutdown));
                (*Client->WS_).close();
            }
            Client->Connected_ = false;
            poco_debug(Client->Logger(),fmt::format("{}: disconnecting. Reason: {}", Client->SerialNumber_, Reason));
            if(Reconnect) {
                Runner->Scheduler().in(std::chrono::seconds(OWLSutils::local_random(3, 15)),
                                          OWLSclientEvents::EstablishConnection, Client, Runner);
            } else {
//                DEBUG_LINE("not reconnecting");
            }
        }
    }

}