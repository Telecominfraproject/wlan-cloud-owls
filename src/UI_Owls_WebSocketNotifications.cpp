//
// Created by stephane bourque on 2022-10-30.
//

#include "UI_Owls_WebSocketNotifications.h"
#include "framework/UI_WebSocketClientServer.h"

namespace OpenWifi {

    void WebSocketNotificationSimulationUpdate( WebSocketNotificationSimulationUpdate_t &N) {
        N.type = "owls_simulation_update";
        UI_WebSocketClientServer()->SendNotification(N);
    }

    void WebSocketNotificationSimulationUpdate( const std::string & User, WebSocketNotificationSimulationUpdate_t &N) {
        N.type = "owls_simulation_update";
        UI_WebSocketClientServer()->SendUserNotification(User,N);
    }

} // OpenWifi