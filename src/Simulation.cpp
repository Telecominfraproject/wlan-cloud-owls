//
// Created by stephane bourque on 2021-11-03.
//

#include "Simulation.h"
#include "StorageService.h"
#include "SimStats.h"

namespace OpenWifi {

    int SimulationCoordinator::Start() {
        CASLocation_ = MicroService::instance().ConfigPath("ucentral.cas");
        KeyFileName_ = MicroService::instance().ConfigPath("ucentral.key");
        CertFileName_ = MicroService::instance().ConfigPath("ucentral.cert");
        RootCAFileName_ = MicroService::instance().ConfigPath("ucentral.rootca");
        std::string L = MicroService::instance().ConfigGetString("ucentral.security");
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
        if(Running_) {
            Running_ = false;
            Worker_.wakeUp();
            Worker_.join();
        }
    }

    void SimulationCoordinator::run() {
        Running_ = true ;

        while(Running_) {
            Poco::Thread::trySleep(2000);
            if(!Running_)
                break;

            if(SimStats()->GetState()!="running") {
                continue;
            }

            uint64_t    Now = std::time(nullptr);
            if( (Now - SimStats()->GetStartTime()) > CurrentSim_.simulationLength ) {
                std::string Error;
                StopSim( SimStats()->Id(), Error );
            }
        }
    }

    void SimulationCoordinator::StartSimulators() {
        Logger_.notice("Starting simulation threads...");
        for(const auto &i:SimThreads_) {
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            i->Thread.start(i->Sim);
            std::cout << __func__ << " : " << __LINE__ << std::endl;
        }
    }

    void SimulationCoordinator::CancelSimulators() {
        Logger_.notice("Cancel simulation threads...");
        SimStats()->EndSim();
        for(const auto &i:SimThreads_) {
            i->Sim.stop();
            i->Thread.join();
        }
        SimThreads_.clear();
    }

    void SimulationCoordinator::StopSimulators() {
        Logger_.notice("Stopping simulation threads...");
        SimStats()->EndSim();
        for(const auto &i:SimThreads_) {
            i->Sim.stop();
            i->Thread.join();
        }
        SimThreads_.clear();
    }

    static const nlohmann::json DefaultCapabilities = R"(
    {"compatible":"linksys_ea8300","model":"Linksys EA8300 (Dallas) ","network":{"lan":["eth0"],"wan":["eth1"]},"platform":"ap","switch":{"switch0":{"enable":true,"ports":[{"device":"eth0","need_tag":false,"num":0,"want_untag":true},{"num":1,"role":"lan"},{"num":2,"role":"lan"},{"num":3,"role":"lan"},{"num":4,"role":"lan"}],"reset":true,"roles":[{"device":"eth0","ports":"1 2 3 4 0","role":"lan"}]}},"wifi":{"platform/soc/a000000.wifi":{"band":["2G"],"channels":[1,2,3,4,5,6,7,8,9,10,11],"frequencies":[2412,2417,2422,2427,2432,2437,2442,2447,2452,2457,2462],"ht_capa":6639,"htmode":["HT20","HT40","VHT20","VHT40","VHT80"],"rx_ant":3,"tx_ant":3,"vht_capa":865687986},"platform/soc/a800000.wifi":{"band":["5G"],"channels":[36,40,44,48,52,56,60,64],"frequencies":[5180,5200,5220,5240,5260,5280,5300,5320],"ht_capa":6639,"htmode":["HT20","HT40","VHT20","VHT40","VHT80"],"rx_ant":3,"tx_ant":3,"vht_capa":865687986},"soc/40000000.pci/pci0000:00/0000:00:00.0/0000:01:00.0":{"band":["5G"],"channels":[100,104,108,112,116,120,124,128,132,136,140,144,149,153,157,161,165],"frequencies":[5500,5520,5540,5560,5580,5600,5620,5640,5660,5680,5700,5720,5745,5765,5785,5805,5825],"ht_capa":6639,"htmode":["HT20","HT40","VHT20","VHT40","VHT80"],"rx_ant":3,"tx_ant":3,"vht_capa":865696178}}}
    )"_json;

    bool SimulationCoordinator::StartSim(const std::string &SimId, std::string & Id, std::string &Error, const std::string & Owner) {
        if(SimRunning_) {
            Error = "Another simulation is already running.";
            return false;
        }

        if(!StorageService()->SimulationDB().GetRecord("id",SimId,CurrentSim_)) {
            Error = "Simulation ID specified does not exist.";
            return false;
        }
        std::cout << __func__ << " : " << __LINE__ << std::endl;

        DefaultCapabilities_ = DefaultCapabilities;
        std::cout << __func__ << " : " << __LINE__ << std::endl;
        DefaultCapabilities_["compatible"] = CurrentSim_.deviceType;
        std::cout << __func__ << " : " << __LINE__ << std::endl;

        auto ClientCount = CurrentSim_.devices;
        std::cout << __func__ << " : " << __LINE__ << std::endl;
        auto NumClientsPerThread = CurrentSim_.devices;
        std::cout << __func__ << " : " << __LINE__ << std::endl;

        // create the actual simulation...
        if(CurrentSim_.threads==0) {
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            CurrentSim_.threads = Poco::Environment::processorCount() * 4;
            std::cout << __func__ << " : " << __LINE__ << std::endl;
        }
        if(CurrentSim_.devices>250) {
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            if(CurrentSim_.devices % CurrentSim_.threads == 0)
            {
                NumClientsPerThread = CurrentSim_.devices / CurrentSim_.threads;
            }
            else
            {
                NumClientsPerThread = CurrentSim_.devices / (CurrentSim_.threads+1);
            }
        }

        // Poco::Logger    & ClientLogger = Poco::Logger::get("WS-CLIENT");
        // ClientLogger.setLevel(Poco::Message::PRIO_WARNING);
        for(auto i=0;ClientCount;i++)
        {
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            auto Clients = std::min(ClientCount,NumClientsPerThread);
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            auto NewSimThread = std::make_unique<SimThread>(i,CurrentSim_.macPrefix,Clients, Logger_);
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            NewSimThread->Sim.Initialize();
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            SimThreads_.push_back(std::move(NewSimThread));
            std::cout << __func__ << " : " << __LINE__ << std::endl;
            ClientCount -= Clients;
            std::cout << __func__ << " : " << __LINE__ << std::endl;
        }

        std::cout << __func__ << " : " << __LINE__ << std::endl;
        StartSimulators();
        std::cout << __func__ << " : " << __LINE__ << std::endl;
        SimRunning_ = true ;
        std::cout << __func__ << " : " << __LINE__ << std::endl;
        SimStats()->StartSim(MicroService::instance().CreateUUID(), SimId, CurrentSim_.devices, Owner);
        std::cout << __func__ << " : " << __LINE__ << std::endl;
        return true;
    }

    bool SimulationCoordinator::StopSim(const std::string &Id, std::string &Error) {
        if(!SimRunning_) {
            Error = "No simulation is running.";
            return false;
        }

        StopSimulators();
        SimRunning_ = false;

        OWLSObjects::SimulationStatus   S;
        SimStats()->GetCurrent(S);
        StorageService()->SimulationResultsDB().CreateRecord(S);

        return true;
    }

    bool SimulationCoordinator::CancelSim(const std::string &Id, std::string &Error) {
        if(!SimRunning_) {
            Error = "No simulation is running.";
            return false;
        }

        CancelSimulators();
        StopSimulators();
        SimRunning_ = false;
        SimStats()->SetState("none");

        return true;
    }


    static const nlohmann::json DefaultConfiguration = R"~~~(
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
                                "2G"
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
                    "tx-power": 10
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
                    "tx-power": 10
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
    )~~~"_json;

    nlohmann::json SimulationCoordinator::GetSimConfiguration( uint64_t uuid ) {
        nlohmann::json Temp = DefaultConfiguration;
        Temp["uuid"] = uuid;
        return Temp;
    }

}