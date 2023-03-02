//
// Created by stephane bourque on 2021-11-02.
//

#include "RESTAPI_status_handler.h"
#include "RESTObjects/RESTAPI_OWLSobjects.h"
#include "SimStats.h"

namespace OpenWifi {
	void RESTAPI_status_handler::DoGet() {
		OWLSObjects::SimulationStatus S;
		SimStats()->GetCurrent(S);
		Poco::JSON::Object Answer;
		S.to_json(Answer);
		ReturnObject(Answer);
	}
} // namespace OpenWifi