//
// Created by stephane bourque on 2021-11-02.
//

#include "RESTAPI_simulation_handler.h"
#include "RESTObjects/RESTAPI_OWLSobjects.h"
#include "StorageService.h"

#include "Poco/Net/HTTPCookie.h"
#include "Poco/Net/NameValueCollection.h"

namespace OpenWifi {

    static const std::vector<std::string> DefaultDeviceTypes{
      "cig_wf160d",
      "cig_wf188",
      "cig_wf188n",
      "cig_wf194c",
      "cig_wf194c4",
      "edgecore_eap101",
      "edgecore_eap102",
      "edgecore_ecs4100-12ph",
      "edgecore_ecw5211",
      "edgecore_ecw5410",
      "edgecore_oap100",
      "edgecore_spw2ac1200",
      "edgecore_spw2ac1200-lan-poe",
      "edgecore_ssw2ac2600",
      "hfcl_ion4.yml",
      "indio_um-305ac",
      "linksys_e8450-ubi",
      "linksys_ea6350",
      "linksys_ea6350-v4",
      "linksys_ea8300",
      "mikrotik_nand",
      "tp-link_ec420-g1",
      "tplink_cpe210_v3",
      "tplink_cpe510_v3",
      "tplink_eap225_outdoor_v1",
      "tplink_ec420",
      "tplink_ex227",
      "tplink_ex228",
      "tplink_ex447",
      "wallys_dr40x9"};

    static bool GooDeviceType(const std::string &D) {
        for(const auto &i:DefaultDeviceTypes) {
            if(i==D)
                return true;
        }
        return false;
    }

    void RESTAPI_simulation_handler::DoPost() {

        OWLSObjects::SimulationDetails  D;
        auto Raw = ParseStream();
        if(!D.from_json(Raw) ||
            D.name.empty() ||
            D.gateway.empty() ||
            D.macPrefix.size()!=6 ||
            D.key.empty() ||
            D.deviceType.empty() ||
            !GooDeviceType(D.deviceType) ||
            D.certificate.empty() ||
            (D.maxClients<D.minClients) ||
            (D.maxAssociations<D.minAssociations)) {
            return BadRequest(RESTAPI::Errors::InvalidJSONDocument);
        }

        D.id = MicroService::instance().CreateUUID();
        if(StorageService()->SimulationDB().CreateRecord(D)) {
            OWLSObjects::SimulationDetails  N;
            StorageService()->SimulationDB().GetRecord("id", D.id, N);
            Poco::JSON::Object Answer;
            N.to_json(Answer);
            return ReturnObject(Answer);
        }
        BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
    }

    void RESTAPI_simulation_handler::DoGet() {
        std::vector<OWLSObjects::SimulationDetails> Sims;
        StorageService()->SimulationDB().GetRecords(1,1000,Sims);
        ReturnObject("list", Sims);
    }

    void RESTAPI_simulation_handler::DoDelete() {
        std::string id;

        if(!HasParameter("id",id) || id.empty()) {
            return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
        }

        if(!StorageService()->SimulationDB().DeleteRecord("id",id))
            return NotFound();
        return OK();
    }

    void RESTAPI_simulation_handler::DoPut() {
        OWLSObjects::SimulationDetails  D;
        auto Raw = ParseStream();

        if(!D.from_json(Raw) ||
            D.id.empty() ||
            D.name.empty() ||
            D.gateway.empty() ||
            D.macPrefix.size()!=6 ||
            D.key.empty() ||
            D.certificate.empty() ||
            D.deviceType.empty() ||
            !GooDeviceType(D.deviceType) ||
            (D.maxClients<D.minClients) ||
            (D.maxAssociations<D.minAssociations)) {
            return BadRequest(RESTAPI::Errors::InvalidJSONDocument);
        }

        if(StorageService()->SimulationDB().UpdateRecord("id", D.id, D)) {
            OWLSObjects::SimulationDetails  N;
            StorageService()->SimulationDB().GetRecord("id", D.id, N);
            Poco::JSON::Object Answer;
            N.to_json(Answer);
            return ReturnObject(Answer);
        }
        NotFound();
    }
}