//
// Created by stephane bourque on 2021-11-02.
//

#include "RESTAPI_operation_handler.h"
#include "SimStats.h"
#include "SimulationCoordinator.h"

namespace OpenWifi {
	void RESTAPI_operation_handler::DoPost() {

        auto Id = GetBinding("id","");

        if(Id.empty()) {
            return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
        }

		std::string Op;
		if (!HasParameter("operation", Op) || (Op != "start" && Op != "stop" && Op != "cancel")) {
			return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
		}

		std::string SimId;
		if (!HasParameter("runningId", SimId) && Op!="start") {
			return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
		}

		std::string Error;
		if (Op == "start") {
			SimulationCoordinator()->StartSim(SimId, Id, Error, UserInfo_.userinfo.email);
		} else if (Op == "stop") {
			SimulationCoordinator()->StopSim(Id, Error);
		} else if (Op == "cancel") {
			SimulationCoordinator()->CancelSim(Id, Error);
		}

		if (Error.empty()) {
			OWLSObjects::SimulationStatus S;
			SimStats()->GetCurrent(SimId,S);
			Poco::JSON::Object Answer;
			S.to_json(Answer);
			return ReturnObject(Answer);
		}
		return BadRequest(RESTAPI::Errors::CouldNotPerformCommand);
	}
} // namespace OpenWifi