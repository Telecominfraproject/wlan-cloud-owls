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

    void RESTAPI_simulation_handler::DoGet() {
        auto id = GetBinding("id","");

        if(id.empty()) {
            return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
        }

        if(id == "*") {
            std::vector<OWLSObjects::SimulationDetails> Sims;
            StorageService()->SimulationDB().GetRecords(QB_.Offset, QB_.Limit, Sims);
            return ReturnObject("list", Sims);
        }

        OWLSObjects::SimulationDetails  Sim;
        if(StorageService()->SimulationDB().GetRecord("id",id, Sim)) {
            Poco::JSON::Object  Answer;
            Sim.to_json(Answer);
            return ReturnObject(Answer);
        }
        return NotFound();
    }

    static bool ValidateSimulation(const OWLSObjects::SimulationDetails & ExistingSimulation ) {
        if( ExistingSimulation.name.empty()  ||
            ExistingSimulation.gateway.empty()  ||
            ExistingSimulation.deviceType.empty()   ||
            !GooDeviceType(ExistingSimulation.deviceType)   ||
            ExistingSimulation.maxClients < ExistingSimulation.minClients ||
            ExistingSimulation.maxAssociations < ExistingSimulation.minAssociations ||
            ExistingSimulation.devices <1 || ExistingSimulation.devices>50000    ||
            ExistingSimulation.healthCheckInterval < 30 || ExistingSimulation.healthCheckInterval >600 ||
            ExistingSimulation.stateInterval < 30 || ExistingSimulation.healthCheckInterval>600 ||
            ExistingSimulation.minAssociations > 4 ||
            ExistingSimulation.maxAssociations > 64 ||
            ExistingSimulation.minClients > 4 ||
            ExistingSimulation.maxAssociations > 16 ||
            ExistingSimulation.keepAlive <120 || ExistingSimulation.keepAlive>3000 ||
            ExistingSimulation.reconnectInterval <10 || ExistingSimulation.reconnectInterval>300 ||
            ExistingSimulation.concurrentDevices < 1 || ExistingSimulation.concurrentDevices >1000 ||
            ExistingSimulation.threads < 4 || ExistingSimulation.threads > 1024 ||
            ExistingSimulation.macPrefix.size()!=6 ) {
            return false;
        }
        return true;
    }

	void RESTAPI_simulation_handler::DoPost() {

		OWLSObjects::SimulationDetails NewSimulation;
		const auto &Raw = ParsedBody_;

		if (!NewSimulation.from_json(Raw)) {
            return BadRequest(RESTAPI::Errors::InvalidJSONDocument);
        }

        if(!ValidateSimulation(NewSimulation)) {
            return BadRequest(RESTAPI::Errors::InvalidJSONDocument);
		}

        NewSimulation.id = MicroServiceCreateUUID();
		if (StorageService()->SimulationDB().CreateRecord(NewSimulation)) {
			OWLSObjects::SimulationDetails N;
			StorageService()->SimulationDB().GetRecord("id", NewSimulation.id, N);
			Poco::JSON::Object Answer;
			N.to_json(Answer);
			return ReturnObject(Answer);
		}
		BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
	}

	void RESTAPI_simulation_handler::DoDelete() {
        auto id = GetBinding("id","");
		if (id.empty()) {
			return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
		}

		if (!StorageService()->SimulationDB().DeleteRecord("id", id))
			return NotFound();
		return OK();
	}

	void RESTAPI_simulation_handler::DoPut() {

        OWLSObjects::SimulationDetails NewSimulation;

        auto id = GetBinding("id","");
        if (id.empty()) {
            return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
        }

		if (!NewSimulation.from_json(ParsedBody_)) {
            return BadRequest(RESTAPI::Errors::InvalidJSONDocument);
        }

        OWLSObjects::SimulationDetails  ExistingSimulation;
        if(!StorageService()->SimulationDB().GetRecord("id", id, ExistingSimulation)) {
            return NotFound();
        }

        AssignIfPresent(ParsedBody_, "name", ExistingSimulation.name);
        AssignIfPresent(ParsedBody_, "gateway", ExistingSimulation.gateway);
        AssignIfPresent(ParsedBody_, "macPrefix", ExistingSimulation.macPrefix);
        AssignIfPresent(ParsedBody_, "deviceType", ExistingSimulation.deviceType);
        AssignIfPresent(ParsedBody_, "devices", ExistingSimulation.devices);
        AssignIfPresent(ParsedBody_, "healthCheckInterval", ExistingSimulation.healthCheckInterval);
        AssignIfPresent(ParsedBody_, "stateInterval", ExistingSimulation.stateInterval);
        AssignIfPresent(ParsedBody_, "minAssociations", ExistingSimulation.minAssociations);
        AssignIfPresent(ParsedBody_, "maxAssociations", ExistingSimulation.maxAssociations);
        AssignIfPresent(ParsedBody_, "minClients", ExistingSimulation.minClients);
        AssignIfPresent(ParsedBody_, "maxClients", ExistingSimulation.maxClients);
        AssignIfPresent(ParsedBody_, "simulationLength", ExistingSimulation.simulationLength);
        AssignIfPresent(ParsedBody_, "threads", ExistingSimulation.threads);
        AssignIfPresent(ParsedBody_, "keepAlive", ExistingSimulation.keepAlive);
        AssignIfPresent(ParsedBody_, "reconnectInterval", ExistingSimulation.reconnectInterval);
        AssignIfPresent(ParsedBody_, "concurrentDevices", ExistingSimulation.concurrentDevices);

        if(!ValidateSimulation(NewSimulation)) {
            return BadRequest(RESTAPI::Errors::InvalidJSONDocument);
        }

		if (StorageService()->SimulationDB().UpdateRecord("id", id, ExistingSimulation)) {
			OWLSObjects::SimulationDetails N;
			StorageService()->SimulationDB().GetRecord("id", ExistingSimulation.id, N);
			Poco::JSON::Object Answer;
			N.to_json(Answer);
			return ReturnObject(Answer);
		}
		NotFound();
	}
} // namespace OpenWifi