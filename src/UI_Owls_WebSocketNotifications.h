//
// Created by stephane bourque on 2022-10-30.
//

#pragma once

#include "framework/UI_WebSocketClientNotifications.h"
#include "RESTObjects/RESTAPI_OWLSobjects.h"

namespace OpenWifi {

    typedef WebSocketNotification<OpenWifi::OWLSObjects::SimulationStatus>  WebSocketNotificationSimulationUpdate_t;

    void WebSocketNotificationSimulationUpdate( WebSocketNotificationSimulationUpdate_t &N);
    void WebSocketNotificationSimulationUpdate( const std::string & User, WebSocketNotificationSimulationUpdate_t &N);

} // OpenWifi

