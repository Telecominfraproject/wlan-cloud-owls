//
// Created by stephane bourque on 2022-10-30.
//

#include "UI_Owls_WebSocketNotifications.h"
#include "framework/UI_WebSocketClientServer.h"

namespace OpenWifi::OWLSNotifications {

    void SimulationUpdate( SimulationUpdate_t &N) {
        N.type_id = 1000;
        UI_WebSocketClientServer()->SendNotification(N);
    }

    void SimulationUpdate( const std::string & User, SimulationUpdate_t &N) {
        N.type_id = 1000;
        UI_WebSocketClientServer()->SendUserNotification(User,N);
    }

    void Register() {
        static const UI_WebSocketClientServer::NotificationTypeIdVec Notifications = {
                { 1000, "owls_simulation_update" }
        };

        UI_WebSocketClientServer()->RegisterNotifications(Notifications);
    }
} // OpenWifi