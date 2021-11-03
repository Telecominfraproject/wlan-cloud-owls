//
// Created by stephane bourque on 2021-11-02.
//

#include "storage_results.h"

namespace OpenWifi {

    static  ORM::FieldVec    SimulationResultsDB_Fields{
        ORM::Field{"id",64, true},
        ORM::Field{"simulationId",ORM::FieldType::FT_TEXT},
        ORM::Field{"startTime",ORM::FieldType::FT_BIGINT},
        ORM::Field{"endTime",ORM::FieldType::FT_BIGINT},
        ORM::Field{"numClientsSuccesses",ORM::FieldType::FT_BIGINT},
        ORM::Field{"numClientsErrors",ORM::FieldType::FT_BIGINT},
        ORM::Field{"timeToFullDevices",ORM::FieldType::FT_BIGINT},
        ORM::Field{"tx",ORM::FieldType::FT_BIGINT},
        ORM::Field{"rx",ORM::FieldType::FT_BIGINT}
    };

    static  ORM::IndexVec    SimulationResultsDB_Indexes{
    };

    SimulationResultsDB::SimulationResultsDB( OpenWifi::DBType T, Poco::Data::SessionPool & P, Poco::Logger &L) :
        DB(T, "simresults", SimulationResultsDB_Fields, SimulationResultsDB_Indexes, P, L, "sir") {
    }
}

template<> void ORM::DB<OpenWifi::SimulationResultsDBRecordType, OpenWifi::OWLSObjects::SimulationResults>::Convert(OpenWifi::SimulationResultsDBRecordType &In, OpenWifi::OWLSObjects::SimulationResults &Out) {
    Out.id = In.get<0>();
    Out.simulationId = In.get<1>();
    Out.startTime = In.get<2>();
    Out.endTime = In.get<3>();
    Out.numClientsSuccesses = In.get<4>();
    Out.numClientsErrors = In.get<5>();
    Out.timeToFullDevices = In.get<6>();
    Out.tx = In.get<7>();
    Out.rx = In.get<8>();
}

template<> void ORM::DB<OpenWifi::SimulationResultsDBRecordType, OpenWifi::OWLSObjects::SimulationResults>::Convert(OpenWifi::OWLSObjects::SimulationResults &In, OpenWifi::SimulationResultsDBRecordType &Out) {
    Out.set<0>(In.id);
    Out.set<1>(In.simulationId);
    Out.set<2>(In.startTime);
    Out.set<3>(In.endTime);
    Out.set<4>(In.numClientsSuccesses);
    Out.set<5>(In.numClientsErrors);
    Out.set<6>(In.timeToFullDevices);
    Out.set<7>(In.tx);
    Out.set<8>(In.rx);
}
