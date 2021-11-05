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
        for(const auto &i:SimThreads_)
            i->Thread.start(i->Sim);
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

    bool SimulationCoordinator::StartSim(const std::string &SimId, std::string & Id, std::string &Error) {
        if(SimRunning_) {
            Error = "Another simulation is already running.";
            return false;
        }

        if(!StorageService()->SimulationDB().GetRecord("id",SimId,CurrentSim_)) {
            Error = "Simulation ID specified does not exist.";
            return false;
        }

        DefaultCapabilities_ = DefaultCapabilities;
        DefaultCapabilities_["compatible"] = CurrentSim_.deviceType;

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
        SimStats()->StartSim(MicroService::instance().CreateUUID(), SimId, CurrentSim_.devices);
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

    static const nlohmann::json DefaultState = R"(
        {
          "interfaces": [
            {
              "clients": [
                {
                  "ipv6_addresses": [
                    "fe80:0:0:0:14a2:4348:a5bd:6842"
                  ],
                  "mac": "0e:f9:d5:98:67:94",
                  "ports": [
                    "wlan0"
                  ]
                },
                {
                  "ipv6_addresses": [
                    "fe80:0:0:0:304c:beff:fe83:c81d"
                  ],
                  "mac": "32:4c:be:83:c8:1d",
                  "ports": [
                    "eth1"
                  ]
                },
                {
                  "ipv6_addresses": [
                    "fe80:0:0:0:3167:bcb9:8fca:85ae"
                  ],
                  "mac": "38:2c:4a:b3:87:88",
                  "ports": [
                    "eth1"
                  ]
                },
                {
                  "ipv6_addresses": [
                    "fe80:0:0:0:640c:44ff:fe61:dc9b"
                  ],
                  "mac": "66:0c:44:61:dc:9b",
                  "ports": [
                    "eth1"
                  ]
                },
                {
                  "ipv6_addresses": [
                    "fe80:0:0:0:5886:66fb:eea1:fd46"
                  ],
                  "mac": "70:85:c2:45:93:c9",
                  "ports": [
                    "eth1"
                  ]
                },
                {
                  "ipv6_addresses": [
                    "fe80:0:0:0:fd10:eb6:9d8b:8c8d"
                  ],
                  "mac": "70:85:c2:7c:9b:8b",
                  "ports": [
                    "eth1"
                  ]
                },
                {
                  "ipv6_addresses": [
                    "fe80:0:0:0:74d3:a238:802f:4cdb"
                  ],
                  "mac": "70:85:c2:f2:be:9a",
                  "ports": [
                    "eth1"
                  ]
                },
                {
                  "ipv6_addresses": [
                    "fe80:0:0:0:923c:b3ff:fe6a:e55c"
                  ],
                  "mac": "90:3c:b3:6a:e5:5c",
                  "ports": [
                    "eth1"
                  ]
                },
                {
                  "ipv6_addresses": [
                    "fe80:0:0:0:a491:65ff:fed1:a74c"
                  ],
                  "mac": "a6:91:65:d1:a7:4c",
                  "ports": [
                    "wlan0"
                  ]
                },
                {
                  "ipv6_addresses": [
                    "fe80:0:0:0:5efd:f030:76ab:85ce"
                  ],
                  "mac": "b8:27:eb:96:14:2f",
                  "ports": [
                    "eth1"
                  ]
                },
                {
                  "ipv6_addresses": [
                    "fe80:0:0:0:f1c8:8c95:355b:22d1",
                    "fd00:ab:cd:0:0:0:0:29"
                  ],
                  "mac": "b8:27:eb:d9:24:74",
                  "ports": [
                    "wlan0"
                  ]
                },
                {
                  "ipv4_addresses": [
                    "192.168.88.1"
                  ],
                  "ipv6_addresses": [
                    "fe80:0:0:0:c417:e6ff:fe7e:fa99"
                  ],
                  "mac": "c6:17:e6:7e:fa:99",
                  "ports": [
                    "eth1"
                  ]
                },
                {
                  "ipv6_addresses": [
                    "fe80:0:0:0:f474:cfff:fe60:5634"
                  ],
                  "mac": "f6:74:cf:60:56:34",
                  "ports": [
                    "eth1"
                  ]
                }
              ],
              "counters": {
                "collisions": 0,
                "multicast": 169,
                "rx_bytes": 36411,
                "rx_dropped": 0,
                "rx_errors": 0,
                "rx_packets": 235,
                "tx_bytes": 3535,
                "tx_dropped": 0,
                "tx_errors": 0,
                "tx_packets": 11
              },
              "dns_servers": [
                "192.168.88.1"
              ],
              "ipv4": {
                "addresses": [
                  "192.168.88.90/24"
                ],
                "leasetime": 43200
              },
              "location": "/interfaces/0",
              "name": "up0v0",
              "ssids": [
                {
                  "associations": [
                    {
                      "bssid": "a6:91:65:d1:a7:4c",
                      "connected": 26241,
                      "inactive": 245,
                      "rssi": -65,
                      "rx_bytes": 0,
                      "rx_packets": 0,
                      "rx_rate": {
                        "bitrate": 108000,
                        "chwidth": 40,
                        "mcs": 3,
                        "nss": 2,
                        "sgi": true,
                        "vht": true
                      },
                      "station": "a6:91:65:d1:a7:4c",
                      "tx_bytes": 0,
                      "tx_duration": 823,
                      "tx_failed": 0,
                      "tx_offset": 0,
                      "tx_packets": 0,
                      "tx_rate": {
                        "bitrate": 300000,
                        "chwidth": 40,
                        "mcs": 7,
                        "nss": 2,
                        "sgi": true,
                        "vht": true
                      },
                      "tx_retries": 0
                    },
                    {
                      "bssid": "0e:f9:d5:98:67:94",
                      "connected": 50734,
                      "inactive": 0,
                      "rssi": -66,
                      "rx_bytes": 78218,
                      "rx_packets": 508,
                      "rx_rate": {
                        "bitrate": 180000,
                        "chwidth": 40,
                        "mcs": 8,
                        "nss": 1,
                        "sgi": true,
                        "vht": true
                      },
                      "station": "0e:f9:d5:98:67:94",
                      "tx_bytes": 311735,
                      "tx_duration": 58721,
                      "tx_failed": 0,
                      "tx_offset": 0,
                      "tx_packets": 563,
                      "tx_rate": {
                        "bitrate": 360000,
                        "chwidth": 40,
                        "mcs": 8,
                        "nss": 2,
                        "sgi": true,
                        "vht": true
                      },
                      "tx_retries": 0
                    },
                    {
                      "bssid": "b8:27:eb:d9:24:74",
                      "connected": 54112,
                      "inactive": 26,
                      "rssi": -62,
                      "rx_bytes": 1642,
                      "rx_packets": 5,
                      "rx_rate": {
                        "bitrate": 150000,
                        "chwidth": 40,
                        "mcs": 7,
                        "nss": 1,
                        "sgi": true,
                        "vht": true
                      },
                      "station": "b8:27:eb:d9:24:74",
                      "tx_bytes": 221,
                      "tx_duration": 28629,
                      "tx_failed": 0,
                      "tx_offset": 0,
                      "tx_packets": 2,
                      "tx_rate": {
                        "bitrate": 200000,
                        "chwidth": 40,
                        "mcs": 9,
                        "nss": 1,
                        "sgi": true,
                        "vht": true
                      },
                      "tx_retries": 0
                    }
                  ],
                  "bssid": "90:3c:b3:6c:42:36",
                  "counters": {
                    "collisions": 0,
                    "multicast": 0,
                    "rx_bytes": 72770,
                    "rx_dropped": 0,
                    "rx_errors": 0,
                    "rx_packets": 555,
                    "tx_bytes": 365626,
                    "tx_dropped": 0,
                    "tx_errors": 0,
                    "tx_packets": 651
                  },
                  "iface": "wlan0",
                  "mode": "ap",
                  "phy": "platform/soc/c000000.wifi",
                  "radio": {
                    "$ref": "#/radios/0"
                  },
                  "ssid": "theDao"
                },
                {
                  "associations": [
                    {
                      "bssid": "dc:4f:22:cc:9f:51",
                      "connected": 48590,
                      "inactive": 1,
                      "rssi": -78,
                      "rx_bytes": 4944,
                      "rx_packets": 23,
                      "rx_rate": {
                        "bitrate": 48000,
                        "chwidth": 20
                      },
                      "station": "dc:4f:22:cc:9f:51",
                      "tx_bytes": 120,
                      "tx_duration": 2097,
                      "tx_failed": 0,
                      "tx_offset": 0,
                      "tx_packets": 2,
                      "tx_rate": {
                        "bitrate": 13000,
                        "chwidth": 20,
                        "ht": true,
                        "mcs": 1,
                        "sgi": true
                      },
                      "tx_retries": 0
                    }
                  ],
                  "bssid": "90:3c:b3:6c:42:3e",
                  "counters": {
                    "collisions": 0,
                    "multicast": 0,
                    "rx_bytes": 4530,
                    "rx_dropped": 0,
                    "rx_errors": 0,
                    "rx_packets": 23,
                    "tx_bytes": 2834,
                    "tx_dropped": 0,
                    "tx_errors": 0,
                    "tx_packets": 30
                  },
                  "iface": "wlan1",
                  "mode": "ap",
                  "phy": "platform/soc/c000000.wifi+1",
                  "radio": {
                    "$ref": "#/radios/1"
                  },
                  "ssid": "theDao24G"
                }
              ],
              "uptime": 54176
            },
            {
              "counters": {
                "collisions": 0,
                "multicast": 0,
                "rx_bytes": 0,
                "rx_dropped": 0,
                "rx_errors": 0,
                "rx_packets": 0,
                "tx_bytes": 0,
                "tx_dropped": 0,
                "tx_errors": 0,
                "tx_packets": 0
              },
              "ipv4": {
                "addresses": [
                  "192.168.1.1/24"
                ]
              },
              "location": "/interfaces/1",
              "name": "down1v0",
              "uptime": 54159
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
              "active_ms": 54138788,
              "busy_ms": 4602642,
              "channel": 157,
              "channel_width": "40",
              "noise": -104,
              "phy": "platform/soc/c000000.wifi",
              "receive_ms": 24292,
              "temperature": 47,
              "transmit_ms": 223348,
              "tx_power": 25
            },
            {
              "active_ms": 54139791,
              "busy_ms": 9552294,
              "channel": 6,
              "channel_width": "20",
              "noise": -97,
              "phy": "platform/soc/c000000.wifi+1",
              "receive_ms": 41999,
              "temperature": 43,
              "transmit_ms": 266360,
              "tx_power": 23
            }
          ],
          "unit": {
            "load": [
              0.024903,
              0.010254,
              0
            ],
            "localtime": 1636087409,
            "memory": {
              "buffered": 10137600,
              "cached": 29208576,
              "free": 753881088,
              "total": 973139968
            },
            "uptime": 54223
          }
        }
    )"_json;

    nlohmann::json SimulationCoordinator::GetSimDefaultState(uint64_t StartTime) {
        nlohmann::json Temp = DefaultState;

        uint64_t Now = std::time(nullptr);
        Temp["unit"]["localtime"] = Now;
        Temp["unit"]["uptime"] = Now - StartTime;

        return Temp;
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