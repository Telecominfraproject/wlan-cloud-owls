//
// Created by stephane bourque on 2021-11-02.
//

#ifndef OWLS_STORAGE_RESULTS_H
#define OWLS_STORAGE_RESULTS_H

#include "framework/orm.h"
#include "RESTObjects/RESTAPI_OWLSobjects.h"

namespace OpenWifi {
    typedef Poco::Tuple<
        std::string,
        std::string,
        std::string,
        uint64_t,
        uint64_t,
        uint64_t,
        uint64_t,
        uint64_t,
        uint64_t,
        uint64_t,
        uint64_t,
        uint64_t
    > SimulationResultsDBRecordType;

    class SimulationResultsDB : public ORM::DB<SimulationResultsDBRecordType,OWLSObjects::SimulationStatus> {
    public:
        SimulationResultsDB( OpenWifi::DBType T, Poco::Data::SessionPool & P, Poco::Logger &L);
    private:
    };

}

#endif //OWLS_STORAGE_RESULTS_H
