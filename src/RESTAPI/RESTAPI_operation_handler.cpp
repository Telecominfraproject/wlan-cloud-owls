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
            ( Op != "start" && Op!= "stop" && Op != "cancel")) {
            return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
        }

        std::cout << __func__ << " : " << __LINE__ << std::endl;
        std::string Id;
        if(HasParameter("id",Id) && Op=="start") {
            return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
        }
        std::cout << __func__ << " : " << __LINE__ << std::endl;

        std::string SimId;
        if(HasParameter("simulationId",SimId) && Op!="start") {
            return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
        }
        std::cout << __func__ << " : " << __LINE__ << std::endl;

        std::string Error;
        if(Op=="start") {
            SimulationCoordinator()->StartSim(SimId,Id,Error, UserInfo_.userinfo.email);
        } else if(Op=="stop") {
            SimulationCoordinator()->StopSim(Id,Error);
        } else if(Op=="cancel") {
            SimulationCoordinator()->CancelSim(Id,Error);
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