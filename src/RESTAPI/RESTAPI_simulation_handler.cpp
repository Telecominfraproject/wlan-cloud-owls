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