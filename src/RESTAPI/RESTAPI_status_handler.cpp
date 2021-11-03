//
// Created by stephane bourque on 2021-11-02.
//

#include "RESTAPI_status_handler.h"
#include "RESTObjects/RESTAPI_OWLSobjects.h"

namespace OpenWifi {
    void RESTAPI_status_handler::DoGet() {
        OWLSObjects::SimulationStatus   S;
        S.state = "none";
        Poco::JSON::Object  Answer;
        S.to_json(Answer);
        ReturnObject(Answer);
    }
}