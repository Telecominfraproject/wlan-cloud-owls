//
// Created by stephane bourque on 2022-10-30.
//

#pragma once

#include "RESTObjects/RESTAPI_OWLSobjects.h"
#include "framework/UI_WebSocketClientNotifications.h"

namespace OpenWifi::OWLSNotifications {

	typedef WebSocketNotification<OpenWifi::OWLSObjects::SimulationStatus> SimulationUpdate_t;

	void Register();

	void SimulationUpdate(SimulationUpdate_t &N);
	void SimulationUpdate(const std::string &User, SimulationUpdate_t &N);

} // namespace OpenWifi::OWLSNotifications
