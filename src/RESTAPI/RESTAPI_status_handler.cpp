//
// Created by stephane bourque on 2021-11-02.
//

#include "RESTAPI_status_handler.h"
#include "RESTObjects/RESTAPI_OWLSobjects.h"
#include "SimStats.h"

namespace OpenWifi {
    void RESTAPI_status_handler::DoGet() {
        std::cout << __func__ << " : " << __LINE__ << std::endl;
        OWLSObjects::SimulationStatus   S;
        std::cout << __func__ << " : " << __LINE__ << std::endl;
        SimStats()->GetCurrent(S);
        std::cout << __func__ << " : " << __LINE__ << std::endl;
        Poco::JSON::Object  Answer;
        std::cout << __func__ << " : " << __LINE__ << std::endl;
        S.to_json(Answer);
        std::cout << __func__ << " : " << __LINE__ << std::endl;
        ReturnObject(Answer);
        std::cout << __func__ << " : " << __LINE__ << std::endl;
    }
}