//
// Created by stephane bourque on 2021-11-02.
//

#include "RESTAPI_operation_handler.h"
#include "Simulation.h"
#include "SimStats.h"

namespace OpenWifi {
    void RESTAPI_operation_handler::DoPost() {

        std::string Op;
        if(!HasParameter("operation", Op) ||
            ( Op != "start" && Op!= "stop" && Op != "cancel" && Op != "pause" && Op != "resume")) {
            return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
        }

        std::string Id;
        if(HasParameter("id",Id) && Op=="start") {
            return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
        }

        std::string SimId;
        if(HasParameter("simulationId",SimId) && Op!="start") {
            return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
        }

        std::string Error;
        std::cout << __func__ << " : " << __LINE__ << std::endl;
        if(Op=="start") {
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            SimulationCoordinator()->StartSim(SimId,Id,Error);
            std::cout << __func__ << " : " << __LINE__ << std::endl;
        } else if(Op=="stop") {
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            SimulationCoordinator()->StopSim(Id,Error);
            std::cout << __func__ << " : " << __LINE__ << std::endl;
        } else if(Op=="pause") {
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            SimulationCoordinator()->PauseSim(Id,Error);
            std::cout << __func__ << " : " << __LINE__ << std::endl;
        } else if(Op=="cancel") {
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            SimulationCoordinator()->CancelSim(Id,Error);
            std::cout << __func__ << " : " << __LINE__ << std::endl;
        } else if(Op=="resume") {
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            SimulationCoordinator()->ResumeSim(Id,Error);
            std::cout << __func__ << " : " << __LINE__ << std::endl;
        }

        std::cout << __func__ << " : " << __LINE__ << std::endl;
        if(Error.empty()) {
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            OWLSObjects::SimulationStatus   S;
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            SimStats()->GetCurrent(S);
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            Poco::JSON::Object  Answer;
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            S.to_json(Answer);
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            return ReturnObject(Answer);
        }
        std::cout << __func__ << " : " << __LINE__ << std::endl;
        return BadRequest(Error);
     }
}