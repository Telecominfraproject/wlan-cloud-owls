//
// Created by stephane bourque on 2021-04-03.
//

#include "Poco/JSON/Object.h"
#include "Poco/JSON/Parser.h"
#include "Poco/JSON/Stringifier.h"
#include "Poco/zlib.h"
#include "base64util.h"

#include "uCentralEvent.h"
#include "uCentralClientApp.h"
#include <cstdlib>

static void SetHeader(Poco::JSON::Object & O, const char * Method)
{
    O.set("jsonrpc","2.0");
    O.set("method",Method);
}

bool uCentralEvent::SendObject(Poco::JSON::Object &Obj) {
    std::stringstream  OS;
    Poco::JSON::Stringifier::stringify(Obj,OS);
    return Client_->Send(OS.str());
}

bool ConnectEvent::Send() {
    Poco::JSON::Object  O;

    SetHeader(O,"connect");

    Poco::JSON::Object  Params;
    Params.set("serial",Client_->Serial());
    Params.set("uuid",Client_->UUID());
    Params.set("firmware",Client_->Firmware());

    Poco::JSON::Parser Parser;
    auto CapabilitiesObj = Parser.parse(Client_->DefaultCapabilities()).extract<Poco::JSON::Object::Ptr>();
    Params.set("capabilities",CapabilitiesObj);

    O.set("params",Params);
    Client_->AddEvent(ev_keepalive,App()->GetKeepAliveInterval());
    Client_->AddEvent(ev_state,App()->GetStateInterval());
    Client_->AddEvent(ev_healthcheck,App()->GetHealthCheckInterval());
    Client_->AddEvent(ev_log,120 + (rand() % 200));

    return SendObject(O);
}

bool StateEvent::Send() {
    Poco::JSON::Object  O;

    SetHeader(O,"state");
    Poco::JSON::Object  Params;

    Params.set("serial",Client_->Serial());
    Params.set("uuid",Client_->UUID());

    Poco::JSON::Parser  Parser;

    std::string State{ Client_->DefaultState() };

    if(State.size()>3000) {
        // compress
        unsigned long BufSize = State.size()+2000;
        std::vector<Bytef>   Buffer(BufSize);

        compress( &Buffer[0], &BufSize, (Bytef*)State.c_str(), State.size());
        auto Compressed = base64::encode(&Buffer[0],BufSize);
        O.set("compress_64",Compressed);

    } else {

        auto StateObj = Parser.parse(State).extract<Poco::JSON::Object::Ptr>();
        Params.set("state", StateObj);
        O.set("params", Params);

    }

    Client_->AddEvent(ev_state, App()->GetStateInterval());

    return SendObject(O);
}

bool HealthCheckEvent::Send() {
    Poco::JSON::Object  O;

    SetHeader(O,"healthcheck");
    Poco::JSON::Object  Params;

    Params.set("serial",Client_->Serial());
    Params.set("uuid",Client_->UUID());
    Params.set("sanity",100);

    Poco::JSON::Parser  Parser;

    auto StateObj = Parser.parse(std::string("{}")).extract<Poco::JSON::Object::Ptr>();
    Params.set("data",StateObj);
    O.set("params",Params);

    Client_->AddEvent(ev_healthcheck, App()->GetHealthCheckInterval());

    return SendObject(O);
};

bool LogEvent::Send() {
    Poco::JSON::Object  O;

    SetHeader(O,"log");

    Poco::JSON::Object  Params;
    Params.set("serial", Client_->Serial());
    Params.set("severity",Severity_);
    Params.set("log",LogLine_);

    O.set("params",Params);
    Client_->AddEvent(ev_log,120 + (rand() % 200));

    return SendObject(O);
};

bool CrashLogEvent::Send() {
    Poco::JSON::Object  O;

    SetHeader(O,"crashlog");
    return false;
};

bool ConfigChangePendingEvent::Send() {
    Poco::JSON::Object  O;

    SetHeader(O,"cfgpending");
    Poco::JSON::Object  Params;

    Params.set("serial",Client_->Serial());
    Params.set("uuid",Client_->UUID());
    Params.set("active",Client_->Active());

    O.set("params",Params);

    Client_->AddEvent(ev_configpendingchange,App()->GetConfigChangePendingInterval());

    return SendObject(O);
}

bool KeepAliveEvent::Send() {
    Poco::JSON::Object  O;

    SetHeader(O,"ping");
    Poco::JSON::Object  Params;

    Params.set("serial",Client_->Serial());
    Params.set("uuid",Client_->UUID());

    O.set("params",Params);

    Client_->AddEvent(ev_keepalive, App()->GetKeepAliveInterval());

    return SendObject(O);
};

bool RebootEvent::Send() {

    return true;
}

bool DisconnectEvent::Send() {

    return true;
}

bool WSPingEvent::Send() {
    Client_->AddEvent(ev_wsping,60*5);
    return Client_->SendWSPing();
}