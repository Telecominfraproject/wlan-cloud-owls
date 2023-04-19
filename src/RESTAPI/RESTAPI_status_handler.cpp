//
// Created by stephane bourque on 2021-11-02.
//

#include "RESTAPI_status_handler.h"
#include "RESTObjects/RESTAPI_OWLSobjects.h"
#include "SimStats.h"

namespace OpenWifi {
	void RESTAPI_status_handler::DoGet() {

        auto id = GetBinding("id","");

        if(id.empty()) {
            return BadRequest(RESTAPI::Errors::MissingOrInvalidParameters);
        }

        std::vector<OWLSObjects::SimulationStatus>  Statuses;
        if(id=="*") {
            SimStats()->GetAllSimulations(Statuses, UserInfo_.userinfo);
        } else {
            OWLSObjects::SimulationStatus S;
            SimStats()->GetCurrent(id, S, UserInfo_.userinfo);
            Statuses.emplace_back(S);
        }
		Poco::JSON::Array   Arr;
        for(const auto &status:Statuses) {
            Poco::JSON::Object  Obj;
            status.to_json(Obj);
            Arr.add(Obj);
        }
        std::ostringstream os;
        Arr.stringify(os);
        ReturnRawJSON(os.str());
	}
} // namespace OpenWifi