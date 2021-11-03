//
// Created by stephane bourque on 2021-11-02.
//

#include "storage_simulations.h"

namespace OpenWifi {
    static  ORM::FieldVec    SimulationDB_Fields{
        ORM::Field{"id",64, true},
        ORM::Field{"name",ORM::FieldType::FT_TEXT},
        ORM::Field{"gateway",ORM::FieldType::FT_TEXT},
        ORM::Field{"certificate",ORM::FieldType::FT_TEXT},
        ORM::Field{"key",ORM::FieldType::FT_TEXT},
        ORM::Field{"devices",ORM::FieldType::FT_BIGINT},
        ORM::Field{"healthCheckInterval",ORM::FieldType::FT_BIGINT},
        ORM::Field{"stateInterval",ORM::FieldType::FT_BIGINT},
        ORM::Field{"minAssociations",ORM::FieldType::FT_BIGINT},
        ORM::Field{"maxAssociations",ORM::FieldType::FT_BIGINT},
        ORM::Field{"minClients",ORM::FieldType::FT_BIGINT},
        ORM::Field{"maxClients",ORM::FieldType::FT_BIGINT},
        ORM::Field{"simulationLength",ORM::FieldType::FT_BIGINT}
    };

    static  ORM::IndexVec    SimulationDB_Indexes{
    };

    SimulationDB::SimulationDB( OpenWifi::DBType T, Poco::Data::SessionPool & P, Poco::Logger &L) :
        DB(T, "simulations", SimulationDB_Fields, SimulationDB_Indexes, P, L, "sim") {
    }
}

template<> void ORM::DB<OpenWifi::SimulationDBRecordType, OpenWifi::OWLSObjects::SimulationDetails>::Convert(OpenWifi::SimulationDBRecordType &In, OpenWifi::OWLSObjects::SimulationDetails &Out) {
    Out.id = In.get<0>();
    Out.name = In.get<1>();
    Out.gateway = In.get<2>();
    Out.certificate = In.get<3>();
    Out.key = In.get<4>();
    Out.devices = In.get<5>();
    Out.healthCheckInterval = In.get<6>();
    Out.stateInterval = In.get<7>();
    Out.minAssociations = In.get<8>();
    Out.maxAssociations = In.get<9>();
    Out.minClients = In.get<10>();
    Out.maxClients = In.get<11>();
    Out.simulationLength = In.get<12>();
}

template<> void ORM::DB<OpenWifi::SimulationDBRecordType, OpenWifi::OWLSObjects::SimulationDetails>::Convert(OpenWifi::OWLSObjects::SimulationDetails &In, OpenWifi::SimulationDBRecordType &Out) {
    Out.set<0>(In.id);
    Out.set<1>(In.name);
    Out.set<2>(In.gateway);
    Out.set<3>(In.certificate);
    Out.set<4>(In.key);
    Out.set<5>(In.devices);
    Out.set<6>(In.healthCheckInterval);
    Out.set<7>(In.stateInterval);
    Out.set<8>(In.minAssociations);
    Out.set<9>(In.maxAssociations);
    Out.set<10>(In.minClients);
    Out.set<11>(In.maxClients);
    Out.set<12>(In.simulationLength);
}



