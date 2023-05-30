//
// Created by stephane bourque on 2023-04-12.
//

#include <fmt/format.h>
#include <Poco/NObserver.h>

#include "OWLSclient.h"
#include "SimulationRunner.h"
#include "SimulationCoordinator.h"
#include "SimStats.h"
#include "OWLSclientEvents.h"

namespace OpenWifi::OWLSClientEvents {

    void EstablishConnection( const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner) {
        if(!Runner->Running()) {
            return;
        }

        Poco::URI uri(Runner->Details().gateway);

        Poco::Net::Context::Params P;

        Runner->Report().ev_establish_connection++;

        P.verificationMode = Poco::Net::Context::VERIFY_STRICT;
        P.verificationDepth = 9;
        P.caLocation = SimulationCoordinator()->GetCasLocation();
        P.loadDefaultCAs = false;
        P.certificateFile = SimulationCoordinator()->GetCertFileName();
        P.privateKeyFile = SimulationCoordinator()->GetKeyFileName();
        P.cipherList = "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH";
        P.dhUse2048Bits = true;

        auto Context = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, P);
        Poco::Crypto::X509Certificate Cert(SimulationCoordinator()->GetCertFileName());
        Poco::Crypto::X509Certificate Root(SimulationCoordinator()->GetRootCAFileName());

        Context->useCertificate(Cert);
        Context->addChainCertificate(Root);

        Context->addCertificateAuthority(Root);

        if (SimulationCoordinator()->GetLevel() == Poco::Net::Context::VERIFY_STRICT) {
        }

        Poco::Crypto::RSAKey Key("", SimulationCoordinator()->GetKeyFileName(), "");
        Context->usePrivateKey(Key);

        SSL_CTX *SSLCtx = Context->sslContext();
        if (!SSL_CTX_check_private_key(SSLCtx)) {
            poco_error(Client->Logger_,fmt::format("Wrong Certificate: {} for {}",SimulationCoordinator()->GetCertFileName() ,
                                                   SimulationCoordinator()->GetKeyFileName()));
        }

        if (SimulationCoordinator()->GetLevel() == Poco::Net::Context::VERIFY_STRICT) {
        }

        Poco::Net::HTTPSClientSession Session(uri.getHost(), uri.getPort(), Context);
        Poco::Net::HTTPRequest Request(Poco::Net::HTTPRequest::HTTP_GET, "/?encoding=text",
                                       Poco::Net::HTTPMessage::HTTP_1_1);
        Request.set("origin", "http://www.websocket.org");
        Poco::Net::HTTPResponse Response;

        std::lock_guard ClientGuard(Client->Mutex_);

        Client->Logger_.information(fmt::format("Connecting({}): host={} port={}", Client->SerialNumber_,
                                        uri.getHost(), uri.getPort()));

        try {
            Client->WS_ = std::make_unique<Poco::Net::WebSocket>(Session, Request, Response);
            (*Client->WS_).setReceiveTimeout(Poco::Timespan(1200,0));
            (*Client->WS_).setSendTimeout(Poco::Timespan(1200,0));
            (*Client->WS_).setKeepAlive(true);
            (*Client->WS_).setNoDelay(true);
            (*Client->WS_).setBlocking(false);
            (*Client->WS_).setMaxPayloadSize(128000);
            Client->fd_ = Client->WS_->impl()->sockfd();
            Runner->AddClientFd(Client->fd_, Client);
            Client->Connected_ = true;
            Client->Reactor_.addEventHandler(
                    *Client->WS_, Poco::NObserver<SimulationRunner, Poco::Net::ReadableNotification>(
                            *Runner, &SimulationRunner::OnSocketReadable));
            Client->Reactor_.addEventHandler(
                    *Client->WS_, Poco::NObserver<SimulationRunner, Poco::Net::ErrorNotification>(
                            *Runner, &SimulationRunner::OnSocketError));
            Client->Reactor_.addEventHandler(
                    *Client->WS_, Poco::NObserver<SimulationRunner, Poco::Net::ShutdownNotification>(
                            *Runner, &SimulationRunner::OnSocketShutdown));
            Connect(ClientGuard, Client, Runner);
            Client->Logger_.information(fmt::format("connecting({}): connected.", Client->SerialNumber_));
        } catch (const Poco::Exception &E) {
            Client->Logger_.warning(
                    fmt::format("Connecting({}): exception. {}", Client->SerialNumber_, E.displayText()));
            Runner->Scheduler().in(std::chrono::seconds(Client->Backoff()), Reconnect, Client, Runner);
        } catch (const std::exception &E) {
            Client->Logger_.warning(
                    fmt::format("Connecting({}): std::exception. {}", Client->SerialNumber_, E.what()));
            Runner->Scheduler().in(std::chrono::seconds(Client->Backoff()), Reconnect, Client, Runner);
        } catch (...) {
            Client->Logger_.warning(fmt::format("Connecting({}): unknown exception. {}", Client->SerialNumber_));
            Runner->Scheduler().in(std::chrono::seconds(Client->Backoff()), Reconnect, Client, Runner);
        }
    }

    void Connect(std::lock_guard<std::mutex> & ClientGuard, const std::shared_ptr<OWLSclient> &Client, SimulationRunner *Runner) {

        try {
            Runner->Report().ev_connect++;

            Poco::JSON::Object::Ptr  ConnectMessage{new Poco::JSON::Object}, Params{new Poco::JSON::Object},
            TmpCapabilities{new Poco::JSON::Object}, Capabilities{new Poco::JSON::Object}, MacAddr{new Poco::JSON::Object};
            auto LabelMac = Utils::SerialNumberToInt(Client->SerialNumber_);
            Params->set("serial", Client->SerialNumber_);
            Params->set("uuid", Client->UUID_);
            Params->set("firmware", Client->Firmware_);
            MacAddr->set("wan", Client->SerialNumber_);
            MacAddr->set("lan", Utils::SerialToMAC(Utils::IntToSerialNumber(LabelMac + 1)));
            TmpCapabilities = SimulationCoordinator()->GetSimCapabilitiesPtr();
            TmpCapabilities->set("label_macaddr", Client->SerialNumber_);
            TmpCapabilities->set("macaddr", MacAddr);
            Params->set("capabilities", TmpCapabilities);

            OWLSutils::MakeHeader(ConnectMessage,"connect",Params);

            if (Client->SendObject(__func__, ConnectMessage)) {
                Client->Reset();
                Runner->Scheduler().in(std::chrono::seconds(Client->StatisticsInterval_),
                                       OWLSClientEvents::State, Client, Runner);
                Runner->Scheduler().in(std::chrono::seconds(Client->HealthInterval_),
                                       OWLSClientEvents::HealthCheck, Client, Runner);
                Runner->Scheduler().in(std::chrono::seconds(MicroServiceRandom(120, 200)),
                                       OWLSClientEvents::Log, Client, Runner, 1, "Device started");
                Runner->Scheduler().in(std::chrono::seconds(60 * 4),
                                       OWLSClientEvents::WSPing, Client, Runner);
                Runner->Scheduler().in(std::chrono::seconds(30),
                                       OWLSClientEvents::Update, Client, Runner);
                Client->Logger_.information(fmt::format("connect({}): completed.", Client->SerialNumber_));
                Client->Backoff_=0;
                SimStats()->Connect(Runner->Id());
                return;
            }
        } catch (const Poco::Exception &E) {
            Client->Logger().log(E);
        }
        OWLSClientEvents::Disconnect(__func__, ClientGuard, Client, Runner, "Error occurred during connection", true);
    }


}
