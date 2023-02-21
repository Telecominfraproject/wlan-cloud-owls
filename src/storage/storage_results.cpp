//
// Created by stephane bourque on 2021-11-02.
//

#include "storage_results.h"

namespace OpenWifi {

	static ORM::FieldVec SimulationResultsDB_Fields{
		ORM::Field{"id", 64, true},
		ORM::Field{"simulationId", ORM::FieldType::FT_TEXT},
		ORM::Field{"state", ORM::FieldType::FT_TEXT},
		ORM::Field{"tx", ORM::FieldType::FT_BIGINT},
		ORM::Field{"rx", ORM::FieldType::FT_BIGINT},
		ORM::Field{"msgsTx", ORM::FieldType::FT_BIGINT},
		ORM::Field{"msgsRx", ORM::FieldType::FT_BIGINT},
		ORM::Field{"liveDevices", ORM::FieldType::FT_BIGINT},
		ORM::Field{"timeToFullDevices", ORM::FieldType::FT_BIGINT},
		ORM::Field{"startTime", ORM::FieldType::FT_BIGINT},
		ORM::Field{"endTime", ORM::FieldType::FT_BIGINT},
		ORM::Field{"errorDevices", ORM::FieldType::FT_BIGINT},
		ORM::Field{"owner", ORM::FieldType::FT_TEXT}};

	static ORM::IndexVec SimulationResultsDB_Indexes{};

	SimulationResultsDB::SimulationResultsDB(OpenWifi::DBType T, Poco::Data::SessionPool &P,
											 Poco::Logger &L)
		: DB(T, "simresults", SimulationResultsDB_Fields, SimulationResultsDB_Indexes, P, L,
			 "sir") {}
} // namespace OpenWifi

template <>
void ORM::DB<OpenWifi::SimulationResultsDBRecordType, OpenWifi::OWLSObjects::SimulationStatus>::
	Convert(const OpenWifi::SimulationResultsDBRecordType &In,
			OpenWifi::OWLSObjects::SimulationStatus &Out) {
	Out.id = In.get<0>();
	Out.simulationId = In.get<1>();
	Out.state = In.get<2>();
	Out.tx = In.get<3>();
	Out.rx = In.get<4>();
	Out.msgsTx = In.get<5>();
	Out.msgsRx = In.get<6>();
	Out.liveDevices = In.get<7>();
	Out.timeToFullDevices = In.get<8>();
	Out.startTime = In.get<9>();
	Out.endTime = In.get<10>();
	Out.errorDevices = In.get<11>();
	Out.owner = In.get<12>();
}

template <>
void ORM::DB<OpenWifi::SimulationResultsDBRecordType, OpenWifi::OWLSObjects::SimulationStatus>::
	Convert(const OpenWifi::OWLSObjects::SimulationStatus &In,
			OpenWifi::SimulationResultsDBRecordType &Out) {
	Out.set<0>(In.id);
	Out.set<1>(In.simulationId);
	Out.set<2>(In.state);
	Out.set<3>(In.tx);
	Out.set<4>(In.rx);
	Out.set<5>(In.msgsTx);
	Out.set<6>(In.msgsRx);
	Out.set<7>(In.liveDevices);
	Out.set<8>(In.timeToFullDevices);
	Out.set<9>(In.startTime);
	Out.set<10>(In.endTime);
	Out.set<11>(In.errorDevices);
	Out.set<12>(In.owner);
}
