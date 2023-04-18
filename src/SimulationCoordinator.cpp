//
// Created by stephane bourque on 2021-11-03.
//

#include "SimulationCoordinator.h"
#include "SimStats.h"
#include "StorageService.h"

#include "Poco/Environment.h"
#include "fmt/format.h"
#include "framework/MicroServiceFuncs.h"
#include "framework/utils.h"

#include <Poco/JSON/Parser.h>

namespace OpenWifi {

	int SimulationCoordinator::Start() {
		CASLocation_ = MicroServiceConfigPath("ucentral.cas", "");
		KeyFileName_ = MicroServiceConfigPath("ucentral.key", "");
		CertFileName_ = MicroServiceConfigPath("ucentral.cert", "");
		RootCAFileName_ = MicroServiceConfigPath("ucentral.rootca", "");
		std::string L = MicroServiceConfigGetString("ucentral.security", "");
		if (L == "strict") {
			Level_ = Poco::Net::Context::VERIFY_STRICT;
		} else if (L == "none") {
			Level_ = Poco::Net::Context::VERIFY_NONE;
		} else if (L == "relaxed") {
			Level_ = Poco::Net::Context::VERIFY_RELAXED;
		} else if (L == "once")
			Level_ = Poco::Net::Context::VERIFY_ONCE;
		Worker_.start(*this);
		return 0;
	}

	void SimulationCoordinator::Stop() {
		if (Running_) {
			Running_ = false;
			Worker_.wakeUp();
			Worker_.join();

            std::lock_guard     G(Mutex_);
            for(auto &[_,simulation]:Simulations_) {
                simulation->Runner.Stop();
            }
            Simulations_.clear();
		}
	}

	void SimulationCoordinator::run() {
		Running_ = true;
		while (Running_) {
			Poco::Thread::trySleep(30000);
			if (!Running_)
				break;

			uint64_t Now = Utils::Now();
            std::lock_guard     G(Mutex_);

            for(auto it = Simulations_.begin(); it!=end(Simulations_); ) {
                const auto &id = it->first;
                const auto &simulation = it->second;
                if (simulation->Details.simulationLength != 0 &&
                    (Now - SimStats()->GetStartTime(id)) > simulation->Details.simulationLength) {
                    std::string Error;
                    simulation->Runner.Stop();
                    SimStats()->EndSim(id);
                    OWLSObjects::SimulationStatus S;
                    SimStats()->GetCurrent(id, S);
                    StorageService()->SimulationResultsDB().CreateRecord(S);
                    SimStats()->RemoveSim(id);
                    it = Simulations_.erase(it);
                } else {
                    ++it;
                }
            }
		}
	}

	void SimulationCoordinator::CancelSimulations() {
		Logger().notice("Cancel simulation threads...");
		for (auto &[_,simulation] : Simulations_) {
            simulation->Runner.Stop();
            SimStats()->EndSim(simulation->Runner.Id());
		}
        Simulations_.clear();
	}

	void SimulationCoordinator::StopSimulations() {
		Logger().notice("Stopping simulation threads...");
        for (auto &[_,simulation] : Simulations_) {
            simulation->Runner.Stop();
            SimStats()->EndSim(simulation->Runner.Id());
        }
        Simulations_.clear();
	}

    static const std::string DefaultCapabilitiesStr = R"(
        {"compatible":"edgecore_eap101","label_macaddr":"90:3c:b3:bb:1e:04","macaddr":{"lan":"90:3c:b3:bb:1e:05","wan":"90:3c:b3:bb:1e:04"},"model":"EdgeCore EAP101","network":{"lan":["eth1","eth2"],"wan":["eth0"]},"platform":"ap","switch":{"switch0":{"enable":false,"reset":false}},"wifi":{"platform/soc/c000000.wifi":{"band":["5G"],"channels":[36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,144,149,153,157,161,165],"dfs_channels":[52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,144],"frequencies":[5180,5200,5220,5240,5260,5280,5300,5320,5500,5520,5540,5560,5580,5600,5620,5640,5660,5680,5700,5720,5745,5765,5785,5805,5825],"he_mac_capa":[13,39432,4160],"he_phy_capa":[28700,34892,49439,1155,11265,0],"ht_capa":6639,"htmode":["HT20","HT40","VHT20","VHT40","VHT80","HE20","HE40","HE80","HE160","HE80+80"],"rx_ant":3,"tx_ant":3,"vht_capa":1939470770},"platform/soc/c000000.wifi+1":{"band":["2G"],"channels":[1,2,3,4,5,6,7,8,9,10,11],"frequencies":[2412,2417,2422,2427,2432,2437,2442,2447,2452,2457,2462],"he_mac_capa":[13,39432,4160],"he_phy_capa":[28674,34828,49439,1155,11265,0],"ht_capa":6639,"htmode":["HT20","HT40","VHT20","VHT40","VHT80","HE20","HE40"],"rx_ant":3,"tx_ant":3,"vht_capa":1939437970}}}
    )";

    Poco::JSON::Object::Ptr SimulationCoordinator::GetSimCapabilitiesPtr() {
        Poco::JSON::Object::Ptr Res;
        Poco::JSON::Parser  P;
        Res = P.parse(DefaultCapabilitiesStr).extract<Poco::JSON::Object::Ptr>();
        return Res;
    }

    bool SimulationCoordinator::IsSimulationRunning(const std::string &id) {
        std::lock_guard G(Mutex_);

        for(const auto &[instance_id,simulation]:Simulations_) {
            if(simulation->Details.id==id)
                return true;
        }
        return false;
    }

	bool SimulationCoordinator::StartSim(std::string &SimId, const std::string &Id,
										 std::string &Error, const std::string &Owner) {
        std::lock_guard G(Mutex_);
        OWLSObjects::SimulationDetails  NewSim;
		if (!StorageService()->SimulationDB().GetRecord("id", Id, NewSim)) {
			Error = "Simulation ID specified does not exist.";
			return false;
		}

		DefaultCapabilities_ = GetSimCapabilitiesPtr();
        DefaultCapabilities_->set("compatible", NewSim.deviceType);

        SimId = MicroServiceCreateUUID();
        auto NewSimulation = std::make_shared<SimulationRecord>(NewSim, Logger(), SimId);
        Simulations_[SimId] = NewSimulation;
        Simulations_[SimId]->Runner.Start();
		SimStats()->StartSim(SimId, Id, NewSim.devices, Owner);
		return true;
	}

	bool SimulationCoordinator::StopSim(const std::string &Id, std::string &Error) {
        std::lock_guard G(Mutex_);

        auto sim_hint = Simulations_.find(Id);
        if(sim_hint==end(Simulations_)) {
            Error = "Simulation ID is not valid";
            return false;
        }

        sim_hint->second->Runner.Stop();
		OWLSObjects::SimulationStatus S;
        SimStats()->EndSim(sim_hint->second->Runner.Id());
		SimStats()->GetCurrent(sim_hint->second->Runner.Id(), S);
		StorageService()->SimulationResultsDB().CreateRecord(S);
        SimStats()->RemoveSim(sim_hint->second->Runner.Id());
        Simulations_.erase(sim_hint);

		return true;
	}

	bool SimulationCoordinator::CancelSim(const std::string &Id,
										  std::string &Error) {
        std::lock_guard G(Mutex_);

        auto sim_hint = Simulations_.find(Id);
        if(sim_hint==end(Simulations_)) {
            Error = "Simulation ID is not valid";
            return false;
        }

        sim_hint->second->Runner.Stop();
		SimStats()->SetState(sim_hint->second->Runner.Id(),"none");
        SimStats()->RemoveSim(sim_hint->second->Runner.Id());
        Simulations_.erase(sim_hint);

		return true;
	}

    static const std::string DefaultConfigurationStr = R"~~~(
        {
            "interfaces": [
                {
                    "ethernet": [
                        {
                            "select-ports": [
                                "WAN*"
                            ]
                        }
                    ],
                    "ipv4": {
                        "addressing": "dynamic"
                    },
                    "ipv6": {
                        "addressing": "dynamic"
                    },
                    "name": "WAN",
                    "role": "upstream",
                    "services": [
                        "lldp"
                    ],
                    "ssids": [
                        {
                            "bss-mode": "ap",
                            "encryption": {
                                "ieee80211w": "optional",
                                "key": "OpenWifi",
                                "proto": "psk2"
                            },
                            "name": "OpenWifi-test5",
                            "wifi-bands": [
                                "5G"
                            ]
                        }
                    ]
                },
                {
                    "ethernet": [
                        {
                            "select-ports": [
                                "LAN*"
                            ]
                        }
                    ],
                    "ipv4": {
                        "addressing": "static",
                        "dhcp": {
                            "lease-count": 100,
                            "lease-first": 10,
                            "lease-time": "6h"
                        },
                        "subnet": "192.168.1.1/24"
                    },
                    "name": "LAN",
                    "role": "downstream",
                    "services": [
                        "ssh",
                        "lldp"
                    ],
                    "ssids": [
                        {
                            "bss-mode": "ap",
                            "encryption": {
                                "ieee80211w": "optional",
                                "key": "OpenWifi",
                                "proto": "psk2"
                            },
                            "name": "OpenWifi-test2",
                            "wifi-bands": [
                                "2G", "5G"
                            ]
                        }
                    ]
                }
            ],
            "metrics": {
                "dhcp-snooping": {
                    "filters": [
                        "ack",
                        "discover",
                        "offer",
                        "request",
                        "solicit",
                        "reply",
                        "renew"
                    ]
                },
                "health": {
                    "interval": 60
                },
                "statistics": {
                    "interval": 60,
                    "types": [
                        "ssids",
                        "lldp",
                        "clients"
                    ]
                },
                "wifi-frames": {
                    "filters": [
                        "probe",
                        "auth",
                        "assoc",
                        "disassoc",
                        "deauth",
                        "local-deauth",
                        "inactive-deauth",
                        "key-mismatch",
                        "beacon-report",
                        "radar-detected"
                    ]
                }
            },
            "radios": [
                {
                    "band": "2G",
                    "bandwidth": 20,
                    "beacon-interval": 100,
                    "channel": "auto",
                    "channel-mode": "VHT",
                    "channel-width": 20,
                    "country": "CA",
                    "dtim-period": 2,
                    "hostapd-iface-raw": [],
                    "legacy-rates": false,
                    "maximum-clients": 64,
                    "rates": {
                        "beacon": 6000,
                        "multicast": 24000
                    },
                    "tx-power": 23
                },
                {
                    "band": "5G",
                    "bandwidth": 20,
                    "beacon-interval": 100,
                    "channel": "auto",
                    "channel-mode": "HE",
                    "channel-width": 80,
                    "country": "CA",
                    "dtim-period": 2,
                    "he": {
                        "bss-color": 64,
                        "ema": false,
                        "multiple-bssid": false
                    },
                    "hostapd-iface-raw": [],
                    "legacy-rates": false,
                    "maximum-clients": 50,
                    "rates": {
                        "beacon": 6000,
                        "multicast": 24000
                    },
                    "tx-power": 23
                }
            ],
            "services": {
                "lldp": {
                    "describe": "",
                    "location": ""
                },
                "ssh": {
                    "authorized-keys": [],
                    "password-authentication": false,
                    "port": 22
                }
            },
            "unit": {
                "leds-active": true,
                "location": "bowen island",
                "name": "Bowen Development Unit",
                "random-password": false,
                "timezone": "UTC-8:00"
            },
            "uuid": 1635660963
        }
    )~~~";

    Poco::JSON::Object::Ptr SimulationCoordinator::GetSimConfigurationPtr(uint64_t uuid) {
        Poco::JSON::Object::Ptr res;

        Poco::JSON::Parser  P;
        res = P.parse(DefaultConfigurationStr).extract<Poco::JSON::Object::Ptr>();
        res->set("uuid", uuid);
        return res;
    }


} // namespace OpenWifi