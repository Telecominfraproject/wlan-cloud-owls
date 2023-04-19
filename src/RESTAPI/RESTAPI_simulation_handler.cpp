//
// Created by stephane bourque on 2021-11-02.
//

#include "RESTAPI_simulation_handler.h"
#include "RESTObjects/RESTAPI_OWLSobjects.h"
#include "StorageService.h"

#include "Poco/Net/HTTPCookie.h"
#include "Poco/Net/NameValueCollection.h"
#include "framework/MicroServiceFuncs.h"
#include <framework/default_device_types.h>

namespace OpenWifi {


	static bool GooDeviceType(const std::string &D) {
        for(const auto &[name,_]:DefaultDeviceTypeList) {
            if(name==D) return true;
        }
        return false;
	}

	void RESTAPI_simulation_handler::DoPost() {

		OWLSObjects::SimulationDetails D;
		const auto &Raw = ParsedBody_;

		if (!D.from_json(Raw) || D.name.empty() || D.gateway.empty() || D.macPrefix.size() != 6 ||
			D.deviceType.empty() || !GooDeviceType(D.deviceType) || (D.maxClients < D.minClients) ||
			(D.maxAssociations < D.minAssociations)) {
			return BadRequest(RESTAPI::Errors::InvalidJSONDocument);
		}

		D.id = MicroServiceCreateUUID();
		if (StorageService()->SimulationDB().CreateRecord(D)) {
			OWLSObjects::SimulationDetails N;
			StorageService()->SimulationDB().GetRecord("id", D.id, N);
			Poco::JSON::Object Answer;
			N.to_json(Answer);
			return ReturnObject(Answer);
		}
		BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
	}

	void RESTAPI_simulation_handler::DoGet() {
		std::vector<OWLSObjects::SimulationDetails> Sims;
		StorageService()->SimulationDB().GetRecords(1, 1000, Sims);
		ReturnObject("list", Sims);
	}

	void RESTAPI_simulation_handler::DoDelete() {
		std::string id;

		if (!HasParameter("id", id) || id.empty()) {
			return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
		}

		if (!StorageService()->SimulationDB().DeleteRecord("id", id))
			return NotFound();
		return OK();
	}

	void RESTAPI_simulation_handler::DoPut() {
		OWLSObjects::SimulationDetails D;
		const auto &Raw = ParsedBody_;

		if (!D.from_json(Raw) || D.id.empty() || D.name.empty() || D.gateway.empty() ||
			D.macPrefix.size() != 6 || D.deviceType.empty() || !GooDeviceType(D.deviceType) ||
			(D.maxClients < D.minClients) || (D.maxAssociations < D.minAssociations)) {
			return BadRequest(RESTAPI::Errors::InvalidJSONDocument);
		}

		if (StorageService()->SimulationDB().UpdateRecord("id", D.id, D)) {
			OWLSObjects::SimulationDetails N;
			StorageService()->SimulationDB().GetRecord("id", D.id, N);
			Poco::JSON::Object Answer;
			N.to_json(Answer);
			return ReturnObject(Answer);
		}
		NotFound();
	}
} // namespace OpenWifi