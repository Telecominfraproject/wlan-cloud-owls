//
// Created by stephane bourque on 2021-11-02.
//

#include "RESTAPI_results_handler.h"
#include "RESTObjects/RESTAPI_OWLSobjects.h"
#include "StorageService.h"

namespace OpenWifi {

    void RESTAPI_results_handler::DoGet() {
        std::vector<OWLSObjects::SimulationResults>     Results;
        StorageService()->SimulationResultsDB().GetRecords(1,10000,Results);
        return ReturnObject("list",Results);
    }

    void RESTAPI_results_handler::DoDelete() {
        std::string id;

        if(!HasParameter("id",id) || id.empty()) {
            return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
        }
        if(!StorageService()->SimulationResultsDB().DeleteRecord("id",id))
            return NotFound();
        return OK();
    }
}