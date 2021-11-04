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
        }
    }

    void SimulationCoordinator::StartSimulators() {
        std::cout << __func__ << " : " << __LINE__ << std::endl;
        Logger_.notice("Starting simulation threads...");
        std::cout << __func__ << " : " << __LINE__ << std::endl;
        SimStats()->StartSim();
        std::cout << __func__ << " : " << __LINE__ << std::endl;
        for(const auto &i:SimThreads_)
            i->Thread.start(i->Sim);
        std::cout << __func__ << " : " << __LINE__ << std::endl;
    }

    void SimulationCoordinator::PauseSimulators() {
        Logger_.notice("Starting simulation threads...");
        for(const auto &i:SimThreads_)
            i->Sim.Pause();
    }

    void SimulationCoordinator::ResumeSimulators() {
        Logger_.notice("Starting simulation threads...");
        for(const auto &i:SimThreads_)
            i->Sim.Resume();
    }

    void SimulationCoordinator::CancelSimulators() {
        Logger_.notice("Starting simulation threads...");
        for(const auto &i:SimThreads_)
            i->Sim.Cancel();
    }

    void SimulationCoordinator::StopSimulators() {
        Logger_.notice("Stopping simulation threads...");
        for(const auto &i:SimThreads_) {
            i->Sim.stop();
            i->Thread.join();
        }
        SimStats()->EndSim();
    }

    static const nlohmann::json DefaultCapabilities = R"(
    {"capabilities":{"compatible":"linksys_ea8300","model":"Linksys EA8300 (Dallas) ","network":{"lan":["eth0"],"wan":["eth1"]},"platform":"ap","switch":{"switch0":{"enable":true,"ports":[{"device":"eth0","need_tag":false,"num":0,"want_untag":true},{"num":1,"role":"lan"},{"num":2,"role":"lan"},{"num":3,"role":"lan"},{"num":4,"role":"lan"}],"reset":true,"roles":[{"device":"eth0","ports":"1 2 3 4 0","role":"lan"}]}},"wifi":{"platform/soc/a000000.wifi":{"band":["2G"],"channels":[1,2,3,4,5,6,7,8,9,10,11],"frequencies":[2412,2417,2422,2427,2432,2437,2442,2447,2452,2457,2462],"ht_capa":6639,"htmode":["HT20","HT40","VHT20","VHT40","VHT80"],"rx_ant":3,"tx_ant":3,"vht_capa":865687986},"platform/soc/a800000.wifi":{"band":["5G"],"channels":[36,40,44,48,52,56,60,64],"frequencies":[5180,5200,5220,5240,5260,5280,5300,5320],"ht_capa":6639,"htmode":["HT20","HT40","VHT20","VHT40","VHT80"],"rx_ant":3,"tx_ant":3,"vht_capa":865687986},"soc/40000000.pci/pci0000:00/0000:00:00.0/0000:01:00.0":{"band":["5G"],"channels":[100,104,108,112,116,120,124,128,132,136,140,144,149,153,157,161,165],"frequencies":[5500,5520,5540,5560,5580,5600,5620,5640,5660,5680,5700,5720,5745,5765,5785,5805,5825],"ht_capa":6639,"htmode":["HT20","HT40","VHT20","VHT40","VHT80"],"rx_ant":3,"tx_ant":3,"vht_capa":865696178}}},"firstUpdate":1623935450,"lastUpdate":1635876628,"serialNumber":"24f5a207a130"}
    )"_json;

    bool SimulationCoordinator::StartSim(const std::string &SimId, std::string & Id, std::string &Error) {
        if(SimRunning_) {
            Error = "Another simulation is already running.";
            return false;
        }

        if(!StorageService()->SimulationDB().GetRecord("id",SimId,CurrentSim_)) {
            Error = "Simulation ID specified does not exist.";
            return false;
        }

        nlohmann::json Temp = DefaultCapabilities;

        Temp["capabilities"]["compatible"] = CurrentSim_.deviceType;
        DefaultCapabilities_ = to_string(Temp);

        auto ClientCount = CurrentSim_.devices;
        auto NumClientsPerThread = CurrentSim_.devices;

        // create the actual simulation...
        if(CurrentSim_.threads==0) {
            CurrentSim_.threads = Poco::Environment::processorCount() * 4;
        }
        if(CurrentSim_.devices>250) {
            if(CurrentSim_.devices % CurrentSim_.threads == 0)
            {
                NumClientsPerThread = CurrentSim_.devices / CurrentSim_.threads;
            }
            else
            {
                NumClientsPerThread = CurrentSim_.devices / (CurrentSim_.threads+1);
            }
        }

        Poco::Logger    & ClientLogger = Poco::Logger::get("WS-CLIENT");
        ClientLogger.setLevel(Poco::Message::PRIO_WARNING);
        for(auto i=0;ClientCount;i++)
        {
            auto Clients = std::min(ClientCount,NumClientsPerThread);
            auto NewSimThread = std::make_unique<SimThread>(i,CurrentSim_.macPrefix,Clients, Logger_);
            NewSimThread->Sim.Initialize(ClientLogger);
            SimThreads_.push_back(std::move(NewSimThread));
            ClientCount -= Clients;
        }

        StartSimulators();
        SimRunning_ = true ;
        SimStats()->SetId(MicroService::instance().CreateUUID(), SimId);
        return true;
    }

    bool SimulationCoordinator::StopSim(const std::string &Id, std::string &Error) {
        if(!SimRunning_) {
            Error = "No simulation is running.";
            return false;
        }

        StopSimulators();

        SimRunning_ = false;
        SimStats()->SetState("stopped");
        return true;
    }

    bool SimulationCoordinator::PauseSim(const std::string &Id, std::string &Error) {
        if(!SimRunning_) {
            Error = "No simulation is running.";
            return false;
        }
        PauseSimulators();
        SimStats()->SetState("paused");
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

    bool SimulationCoordinator::ResumeSim(const std::string &Id, std::string &Error) {
        if(!SimRunning_) {
            Error = "No simulation is running.";
            return false;
        }

        if(SimStats()->GetState()!="paused") {
            Error = "Simulation must be paused first.";
            return false;
        }

        ResumeSimulators();
        SimStats()->SetState("running");
        return true;
    }

    static const nlohmann::json DefaultState = R"(
                {
                  "interfaces": [
                    {
                      "clients": [
                        {
                          "ipv6_addresses": [
                            "fe80:0:0:0:923c:b3ff:febb:1ef4"
                          ],
                          "mac": "90:3c:b3:bb:1e:f4",
                          "ports": [
                            "eth1"
                          ]
                        },
                        {
                          "ipv4_addresses": [
                            "10.2.0.1"
                          ],
                          "ipv6_addresses": [
                            "fe80:0:0:0:7053:96ff:fe8c:94fe",
                            "2604:3d08:9680:bd00:0:0:0:1"
                          ],
                          "mac": "e2:63:da:86:64:8e",
                          "ports": [
                            "eth1"
                          ]
                        }
                      ],
                      "counters": {
                        "collisions": 0,
                        "multicast": 139,
                        "rx_bytes": 47808,
                        "rx_dropped": 0,
                        "rx_errors": 0,
                        "rx_packets": 238,
                        "tx_bytes": 3800,
                        "tx_dropped": 0,
                        "tx_errors": 0,
                        "tx_packets": 15
                      },
                      "location": "/interfaces/0",
                      "name": "up0v0",
                      "ssids": [
                        {
                          "associations": [
                            {
                              "bssid": "0e:f3:35:85:ce:eb",
                              "connected": 4575,
                              "inactive": 79,
                              "rssi": -51,
                              "rx_bytes": 0,
                              "rx_packets": 0,
                              "rx_rate": {
                                "bitrate": 229400,
                                "chwidth": 20
                              },
                              "station": "0e:f3:35:85:ce:eb",
                              "tx_bytes": 0,
                              "tx_duration": 4995,
                              "tx_failed": 0,
                              "tx_offset": 0,
                              "tx_packets": 0,
                              "tx_rate": {
                                "bitrate": 6000,
                                "chwidth": 20
                              },
                              "tx_retries": 0
                            }
                          ],
                          "bssid": "90:3c:b3:bb:1e:f6",
                          "counters": {
                            "collisions": 0,
                            "multicast": 0,
                            "rx_bytes": 0,
                            "rx_dropped": 0,
                            "rx_errors": 0,
                            "rx_packets": 0,
                            "tx_bytes": 6922,
                            "tx_dropped": 0,
                            "tx_errors": 0,
                            "tx_packets": 78
                          },
                          "iface": "wlan0",
                          "mode": "ap",
                          "phy": "platform/soc/c000000.wifi",
                          "radio": {
                            "$ref": "#/radios/0"
                          },
                          "ssid": "OpenWifi-test5"
                        }
                      ],
                      "uptime": 127159
                    },
                    {
                      "dns_servers": [
                        "10.2.0.1"
                      ],
                      "ipv4": {
                        "addresses": [
                          "10.2.8.192/16"
                        ],
                        "leasetime": 86400
                      },
                      "location": "/interfaces/0",
                      "name": "up0v0_4",
                      "uptime": 127154
                    },
                    {
                      "dns_servers": [
                        "2604:3d08:9680:bd00::1"
                      ],
                      "ipv6": {
                        "addresses": [
                          {
                            "address": "2604:3d08:9680:bd00::431/128",
                            "preferred": 80692,
                            "valid": 80692
                          },
                          {
                            "address": "2604:3d08:9680:bd00:923c:b3ff:febb:1ef4/64",
                            "preferred": 86018,
                            "valid": 86018
                          }
                        ]
                      },
                      "location": "/interfaces/0",
                      "name": "up0v0_6",
                      "uptime": 127154
                    },
                    {
                      "counters": {
                        "collisions": 0,
                        "multicast": 0,
                        "rx_bytes": 0,
                        "rx_dropped": 0,
                        "rx_errors": 0,
                        "rx_packets": 0,
                        "tx_bytes": 1058,
                        "tx_dropped": 0,
                        "tx_errors": 0,
                        "tx_packets": 5
                      },
                      "ipv4": {
                        "addresses": [
                          "192.168.1.1/24"
                        ]
                      },
                      "location": "/interfaces/1",
                      "name": "down1v0",
                      "ssids": [
                        {
                          "bssid": "90:3c:b3:bb:1e:fe",
                          "counters": {
                            "collisions": 0,
                            "multicast": 0,
                            "rx_bytes": 0,
                            "rx_dropped": 0,
                            "rx_errors": 0,
                            "rx_packets": 0,
                            "tx_bytes": 1148,
                            "tx_dropped": 0,
                            "tx_errors": 0,
                            "tx_packets": 5
                          },
                          "iface": "wlan1",
                          "mode": "ap",
                          "phy": "platform/soc/c000000.wifi+1",
                          "radio": {
                            "$ref": "#/radios/1"
                          },
                          "ssid": "OpenWifi-test2"
                        }
                      ],
                      "uptime": 127163
                    }
                  ],
                  "link-state": {
                    "lan": {
                      "eth1": {
                        "carrier": 0
                      },
                      "eth2": {
                        "carrier": 0
                      }
                    },
                    "wan": {
                      "eth0": {
                        "carrier": 1,
                        "duplex": "full",
                        "speed": 1000
                      }
                    }
                  },
                  "radios": [
                    {
                      "active_ms": 127043598,
                      "busy_ms": 24099474,
                      "channel": 100,
                      "channel_width": "80",
                      "noise": -104,
                      "phy": "platform/soc/c000000.wifi",
                      "receive_ms": 19544,
                      "temperature": 42,
                      "transmit_ms": 223073,
                      "tx_power": 10
                    },
                    {
                      "active_ms": 127112622,
                      "busy_ms": 10388172,
                      "channel": 11,
                      "channel_width": "20",
                      "noise": -98,
                      "phy": "platform/soc/c000000.wifi+1",
                      "receive_ms": 2015,
                      "temperature": 39,
                      "transmit_ms": 145093,
                      "tx_power": 10
                    }
                  ],
                  "unit": {
                    "load": [
                      0,
                      0,
                      0
                    ],
                    "localtime": 1636032352,
                    "memory": {
                      "buffered": 9949184,
                      "cached": 27176960,
                      "free": 751276032,
                      "total": 973139968
                    },
                    "uptime": 127196
                  }
                }
    )"_json;

    const std::string SimulationCoordinator::GetSimDefaultState(uint64_t StartTime) {
        nlohmann::json Temp = DefaultState;

        uint64_t Now = std::time(nullptr);
        Temp["unit"]["localtime"] = Now;
        Temp["unit"]["uptime"] = Now - StartTime;

        return to_string(Temp);
    }

    static const nlohmann::json DefaultConfiguration = R"(
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
    )";

    std::string SimulationCoordinator::GetSimConfiguration( uint64_t uuid ) {
        nlohmann::json Temp = DefaultConfiguration;

        Temp["uuid"] = uuid;
        return to_string(Temp);
    }

}