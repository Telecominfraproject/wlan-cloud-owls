//
// Created by stephane bourque on 2021-11-02.
//

#include "RESTAPI_results_handler.h"
#include "RESTObjects/RESTAPI_OWLSobjects.h"
#include "StorageService.h"

namespace OpenWifi {

	void RESTAPI_results_handler::DoGet() {

        auto sim_id = GetBinding("id","");
        if(sim_id.empty()) {
            return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
        }

		std::vector<OWLSObjects::SimulationStatus> Results;
        auto where = fmt::format(" simulationId='{}' ", sim_id);
		StorageService()->SimulationResultsDB().GetRecords(QB_.Offset, QB_.Limit, Results, where, " ORDER BY startTime DESC ");

        return ReturnObject("list", Results);
	}

	void RESTAPI_results_handler::DoDelete() {
		auto id = GetBinding("id","");
		if (id.empty()) {
			return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
		}

		if (!StorageService()->SimulationResultsDB().DeleteRecord("id", id))
			return NotFound();

		return OK();
	}
} // namespace OpenWifi