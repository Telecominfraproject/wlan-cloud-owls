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

namespace OpenWifi::OWLSclientEvents {

    void EstablishConnection(std::shared_ptr<OWLSclient> Client, SimulationRunner *Runner) {
        Poco::URI uri(Runner->Details().gateway);

        Poco::Net::Context::Params P;

        std::cout << "Trying to connected: " << Client->SerialNumber_ << std::endl;

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
            std::cout << "Wrong Certificate: " << SimulationCoordinator()->GetCertFileName()
                      << " for " << SimulationCoordinator()->GetKeyFileName() << std::endl;
        }

        if (SimulationCoordinator()->GetLevel() == Poco::Net::Context::VERIFY_STRICT) {
        }

        Poco::Net::HTTPSClientSession Session(uri.getHost(), uri.getPort(), Context);
        Poco::Net::HTTPRequest Request(Poco::Net::HTTPRequest::HTTP_GET, "/?encoding=text",
                                       Poco::Net::HTTPMessage::HTTP_1_1);
        Request.set("origin", "http://www.websocket.org");
        Poco::Net::HTTPResponse Response;

        Client->Logger_.information(fmt::format("connecting({}): host={} port={}", Client->SerialNumber_,
                                        uri.getHost(), uri.getPort()));

        try {
            Client->WS_ = std::make_unique<Poco::Net::WebSocket>(Session, Request, Response);
            (*Client->WS_).setKeepAlive(true);
            (*Client->WS_).setReceiveTimeout(Poco::Timespan());
            (*Client->WS_).setSendTimeout(Poco::Timespan(20, 0));
            (*Client->WS_).setNoDelay(true);
            Runner->Reactor().addEventHandler(
                    *Client->WS_, Poco::NObserver<SimulationRunner, Poco::Net::ReadableNotification>(
                            *Runner, &SimulationRunner::OnSocketReadable));
            Client->Connected_ = true;
            Runner->AddClientFd(Client->WS_->impl()->sockfd(), Client);
            Runner->Scheduler().in(std::chrono::seconds(1), Connect, Client, Runner);
            SimStats()->Connect(Runner->Id());
            std::cout << "Connected: " << Client->SerialNumber_ << std::endl;
        } catch (const Poco::Exception &E) {
            Client->Logger_.warning(
                    fmt::format("connecting({}): exception. {}", Client->SerialNumber_, E.displayText()));
            Runner->Scheduler().in(std::chrono::seconds(60), Reconnect, Client, Runner);
        } catch (const std::exception &E) {
            Client->Logger_.warning(
                    fmt::format("connecting({}): std::exception. {}", Client->SerialNumber_, E.what()));
            Runner->Scheduler().in(std::chrono::seconds(60), Reconnect, Client, Runner);
        } catch (...) {
            Client->Logger_.warning(fmt::format("connecting({}): unknown exception. {}", Client->SerialNumber_));
            Runner->Scheduler().in(std::chrono::seconds(60), Reconnect, Client, Runner);
        }
    }
}