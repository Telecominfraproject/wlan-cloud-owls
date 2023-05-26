//
// Created by stephane bourque on 2022-10-30.
//

#pragma once

#include <framework/UI_WebSocketClientNotifications.h>
#include <RESTObjects/RESTAPI_OWLSobjects.h>

namespace OpenWifi::OWLSNotifications {

	typedef WebSocketNotification<OpenWifi::OWLSObjects::SimulationStatus> SimulationUpdate_t;

	void Register();

	void SimulationUpdate(SimulationUpdate_t &N);
	void SimulationUpdate(const std::string &User, SimulationUpdate_t &N);

} // namespace OpenWifi::OWLSNotifications
