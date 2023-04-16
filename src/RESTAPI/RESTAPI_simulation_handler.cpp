//
// Created by stephane bourque on 2021-11-02.
//

#include "RESTAPI_simulation_handler.h"
#include "RESTObjects/RESTAPI_OWLSobjects.h"
#include "StorageService.h"

#include "Poco/Net/HTTPCookie.h"
#include "Poco/Net/NameValueCollection.h"
#include "framework/MicroServiceFuncs.h"

namespace OpenWifi {

	static const std::vector<std::string> DefaultDeviceTypes{"actiontec_web7200",
															 "cig_wf188n",
															 "cig_wf194c",
															 "cig_wf194c4",
															 "cig_wf196",
															 "cig_wf610d",
															 "cig_wf808",
															 "cybertan_eww622-a1",
															 "edgecore_eap101",
															 "edgecore_eap102",
															 "edgecore_eap104",
															 "edgecore_ecs4100-12ph",
															 "edgecore_ecw5211",
															 "edgecore_ecw5410",
															 "edgecore_oap100",
															 "edgecore_spw2ac1200",
															 "edgecore_spw2ac1200-lan-poe",
															 "edgecore_ssw2ac2600",
															 "hfcl_ion4",
															 "hfcl_ion4x",
															 "hfcl_ion4x_2",
															 "hfcl_ion4xe",
															 "hfcl_ion4xi",
															 "indio_um-305ac",
															 "indio_um-305ax",
															 "indio_um-310ax-v1",
															 "indio_um-325ac",
															 "indio_um-510ac-v3",
															 "indio_um-510axm-v1",
															 "indio_um-510axp-v1",
															 "indio_um-550ac",
															 "linksys_e8450-ubi",
															 "linksys_ea6350-v4",
															 "linksys_ea8300",
															 "liteon_wpx8324",
															 "meshpp_s618_cp01",
															 "meshpp_s618_cp03",
															 "tp-link_ec420-g1",
															 "tplink_ex227",
															 "tplink_ex228",
															 "tplink_ex447",
															 "udaya_a5-id2",
															 "wallys_dr40x9",
															 "wallys_dr6018",
															 "wallys_dr6018_v4",
															 "x64_vm",
															 "yuncore_ax840",
															 "yuncore_fap640",
															 "yuncore_fap650"};

	static bool GooDeviceType(const std::string &D) {
		for (const auto &i : DefaultDeviceTypes) {
			if (i == D)
				return true;
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