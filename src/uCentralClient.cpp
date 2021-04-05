//
// Created by stephane bourque on 2021-03-12.
//
#include <sys/time.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <cstdlib>

#include "uCentralClient.h"
#include "uCentralClientApp.h"

#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/URI.h"
#include "Poco/Net/Context.h"
#include "Poco/NObserver.h"
#include "Poco/Net/SSLException.h"
#include "Poco/JSON/Stringifier.h"
#include "Poco/JSON/Parser.h"

using namespace std::chrono_literals;

std::string uCentralClient::DefaultCapabilities() {
    return std::string(
            "{\"model\":{\"id\":\"linksys,ea8300\",\"name\":\"Linksys EA8300 (Dallas)\"},\"network\":{\"lan\":{\"ifname\":"
            "\"eth0\",\"protocol\":\"static\"},\"wan\":{\"ifname\":\"eth1\",\"protocol\":\"dhcp\"}},\"switch\":{\"switch0\":"
            "{\"enable\":true,\"reset\":true,\"ports\":[{\"num\":0,\"device\":\"eth0\",\"need_tag\":false,\"want_untag\":true},"
            "{\"num\":1,\"role\":\"lan\"},{\"num\":2,\"role\":\"lan\"},{\"num\":3,\"role\":\"lan\"},{\"num\":4,\"role\":\"lan\"}],"
            "\"roles\":[{\"role\":\"lan\",\"ports\":\"1 2 3 4 0\",\"device\":\"eth0\"}]}},\"wifi\":"
            "{\"soc/40000000.pci/pci0000:00/0000:00:00.0/0000:01:00.0\":{\"band\":[\"5u\"],\"ht_capa\":6639,\"vht_capa\":"
            "865696178,\"htmode\":[\"HT20\",\"HT40\",\"VHT20\",\"VHT40\",\"VHT80\"],\"tx_ant\":3,\"rx_ant\":3,\"channels\":"
            "[100,104,108,112,116,120,124,128,132,136,140,144,149,153,157,161,165]},\"platform/soc/a000000.wifi\":{\"band\":[\"2\"],\"ht_capa\":"
            "6639,\"vht_capa\":865687986,\"htmode\":[\"HT20\",\"HT40\",\"VHT20\",\"VHT40\",\"VHT80\"],\"tx_ant\":3,\"rx_ant\":3,"
            "\"channels\":[1,2,3,4,5,6,7,8,9,10,11]},\"platform/soc/a800000.wifi\":{\"band\":[\"5l\"],\"ht_capa\":6639,\"vht_capa\":"
            "865687986,\"htmode\":[\"HT20\",\"HT40\",\"VHT20\",\"VHT40\",\"VHT80\"],\"tx_ant\":3,\"rx_ant\":3,"
            "\"channels\":[36,40,44,48,52,56,60,64]}}}");
}

void uCentralClient::DefaultConfiguration(std::string & Config, uint64_t & UUID )  {
    Config = std::string{
            "{\"uuid\":1613927736,\"steer\":{\"enabled\":1,\"network\":\"wan\",\"debug_level\":0},\"stats\":"
            "{\"interval\":60,\"neighbours\":1,\"traffic\":1,\"wifiiface\":1,\"wifistation\":1,\"pids\":1,"
            "\"serviceprobe\":1,\"lldp\":1,\"system\":1,\"poe\":1},\"phy\":[{\"band\":\"2\",\"cfg\":{\"disabled\":0"
            ",\"country\":\"DE\",\"channel\":6,\"txpower\":30,\"beacon_int\":100,\"htmode\":\"HE40\",\"hwmode"
            "\":\"11g\",\"chanbw\":20}},{\"band\":\"5\",\"cfg\":{\"mimo\":\"4x4\",\"disabled\":0,\"country\":\"DE\","
            "\"channel\":0,\"htmode\":\"HE80\"}},{\"band\":\"5u\",\"cfg\":{\"disabled\":0,\"country\":\"DE\","
            "\"channel\":100,\"htmode\":\"VHT80\"}},{\"band\":\"5l\",\"cfg\":{\"disabled\":0,\"country\":\"DE\",\"channel"
            "\":36,\"htmode\":\"VHT80\"}}],\"ssid\":[{\"band\":[\"2\"],\"cfg\":{\"ssid\":\"uCentral-Guest\",\"encryption"
            "\":\"psk2\",\"key\":\"OpenWifi\",\"mode\":\"ap\",\"isolate\":1,\"network\":\"guest\",\"ieee80211r\":1,"
            "\"ieee80211v\":1,\"ieee80211k\":1,\"ft_psk_generate_local\":1,\"ft_over_ds\":1,\"mobility_domain\":\"4f57\"}"
            "},{\"band\":[\"5l\",\"5\"],\"cfg\":{\"ssid\":\"uCentral-NAT.200\",\"encryption\":\"psk2\",\"key\":\""
            "OpenWifi\",\"mode\":\"ap\",\"network\":\"nat200\",\"ieee80211r\":1,\"ieee80211v\":1,\"ieee80211k\":1,"
            "\"ft_psk_generate_local\":1,\"ft_over_ds\":1,\"mobility_domain\":\"4f51\"}},{\"band\":[\"5l\",\"5\"],\"cfg\""
            ":{\"ssid\":\"uCentral-EAP\",\"encryption\":\"wpa2\",\"server\":\"148.251.188.218\",\"port\":1812,\""
            "auth_secret\":\"uSyncRad1u5\",\"mode\":\"ap\",\"network\":\"lan\",\"ieee80211r\":1,\"ieee80211v\":1,"
            "\"ieee80211k\":1,\"ft_psk_generate_local\":1,\"ft_over_ds\":1,\"mobility_domain\":\"4f51\"}},"
            "{\"band\":[\"5l\",\"5\"],\"cfg\":{\"ssid\":\"uCentral\",\"encryption\":\"psk2\",\"key\":\"OpenWifi\","
            "\"mode\":\"ap\",\"network\":\"wan\",\"ieee80211r\":1,\"ieee80211v\":1,\"ieee80211k\":1,"
            "\"ft_psk_generate_local\":1,\"ft_over_ds\":1,\"mobility_domain\":\"4f51\"}}],\"network\":"
            "[{\"mode\":\"wan\",\"cfg\":{\"proto\":\"dhcp\"}},{\"mode\":\"gre\",\"cfg\":{\"vid\":\"50\""
            ",\"peeraddr\":\"50.210.104.108\"}},{\"mode\":\"nat\",\"vlan\":200,\"cfg\":{\"proto\":\"static\""
            ",\"ipaddr\":\"192.168.16.1\",\"netmask\":\"255.255.255.0\",\"mtu\":1500,\"ip6assign\":60,\"dhcp\":"
            "{\"start\":10,\"limit\":100,\"leasetime\":\"6h\"},\"leases\":[{\"ip\":\"192.168.100.2\",\"mac\":"
            "\"00:11:22:33:44:55\",\"hostname\":\"test\"},{\"ip\":\"192.168.100.3\",\"mac\":\"00:11:22:33:44:56\","
            "\"hostname\":\"test2\"}]}},{\"mode\":\"guest\",\"cfg\":{\"proto\":\"static\",\"ipaddr\":\"192.168.12.11\","
            "\"dhcp\":{\"start\":10,\"limit\":100,\"leasetime\":\"6h\"}}}],\"ntp\":{\"enabled\":1,\"enable_server\":1,"
            "\"server\":[\"0.openwrt.pool.ntp.org\",\"1.openwrt.pool.ntp.org\"]},\"ssh\":{\"enable\":1,\"Port\":22},"
            "\"system\":{\"timezone\":\"CET-1CEST,M3.5.0,M10.5.0/3\"},\"log\":{\"_log_proto\":\"udp\",\"_log_ip\":"
            "\"192.168.11.23\",\"_log_port\":12345,\"_log_hostname\":\"foo\",\"_log_size\":128},\"rtty\":{\"host\":"
            "\"websocket.usync.org\",\"token\":\"7049cb6b7949ba06c6b356d76f0f6275\",\"interface\":\"wan\"}}"};
    UUID = 1613927736;
}

std::string uCentralClient::DefaultState() {
    return std::string(
        "{\"uuid\":1615694464,\"cfg_uuid\":1615694035,\"system\":{\"localtime\":1615698064,\"uptime\":1312691,"
           "\"load\":[14272,10944,11232],\"memory\":{\"total\":254595072,\"free\":107769856,\"shared\":135168,\"buffered\":6066176,"
           "\"available\":96727040,\"cached\":16949248},\"swap\":{\"total\":0,\"free\":0}},\"wifi-iface\":{\"wlan1\":"
           "{\"ssid\":\"uCentral-Guest\",\"mode\":\"ap\",\"frequency\":[2437,2437],\"ch_width\":\"20\",\"tx_power\":20,\"mac\":"
           "\"24:f5:a2:07:a1:32\"},\"wlan2-2\":{\"ssid\":\"uCentral\",\"mode\":\"ap\",\"frequency\":[5180,5210],\"ch_width\":\"80\","
           "\"tx_power\":23,\"mac\":\"24:f5:a2:07:a1:33\"},\"wlan2-1\":{\"ssid\":\"uCentral-EAP\",\"mode\":\"ap\",\"frequency\":"
           "[5180,5210],\"ch_width\":\"80\",\"tx_power\":23,\"mac\":\"24:f5:a2:07:a1:33\"},\"wlan2\":{\"ssid\":\"uCentral-NAT.200\","
           "\"mode\":\"ap\",\"frequency\":[5180,5210],\"ch_width\":\"80\",\"tx_power\":23,\"mac\":\"24:f5:a2:07:a1:33\"}},\"neighbours\":"
           "{\"00:14:ee:0a:e6:a9\":{\"interface\":\"wan\",\"last_seen\":0,\"ipv4\":[\"10.2.207.137\"],\"fdb\":[\"eth0\"]},\"00:23:a4:05:05:95\":"
           "{\"interface\":\"lan\",\"last_seen\":0,\"ipv4\":[\"192.168.1.176\"],\"ipv6\":[\"fe80:0:0:0:482:423e:7fdd:f4ad\",\"fe80:0:0:0:1c40:2ff8:b4e1:a4d7\","
           "\"fe80:0:0:0:1cff:d5ea:9462:9a79\",\"fe80:0:0:0:1ca9:72e6:237a:31c0\"],\"dhcpv4\":[\"192.168.1.176\",\"renegademac\"],\"fdb\":"
           "[\"eth0\"]},\"18:e8:29:25:f7:0a\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"22:f5:a2:07:a1:33\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":"
           "[\"wlan2-1\"]},\"24:f5:a2:07:a1:30\":{\"interface\":\"lan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"24:f5:a2:07:a1:31\":{\"interface\":\"wan\","
           "\"last_seen\":0,\"fdb\":[\"eth0\"]},\"24:f5:a2:07:a1:32\":{\"interface\":\"guest\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"24:f5:a2:07:a1:33\":"
           "{\"interface\":\"nat200\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"26:f5:a2:07:a1:33\":{\"interface\":\"lan\",\"last_seen\":0,\"fdb\":[\"wlan2-1\"]},"
           "\"26:f5:a2:07:a1:34\":{\"interface\":\"lan\",\"offline\":true},\"28:11:a5:f5:79:f3\":{\"interface\":\"wan\",\"last_seen\":0,\"ipv6\":"
           "[\"fe80:0:0:0:2a11:a5ff:fef5:79f3\"],\"fdb\":[\"eth0\"]},\"38:c9:86:33:e2:e6\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"40:62:31:07:e4:6b\":"
           "{\"interface\":\"wan\",\"offline\":true},\"40:62:31:07:e4:8f\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"4a:e1:e3:b7:7e:6b\":"
           "{\"interface\":\"gretun_50\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"6a:24:99:dc:3c:28\":{\"interface\":\"lan\",\"last_seen\":0,\"ipv6\":"
           "[\"fe80:0:0:0:d2:e972:f1c7:34c6\"],\"dhcpv4\":[\"192.168.1.101\",\"StephaneiPhone\"]},\"6c:33:a9:27:8e:c6\":{\"interface\":\"wan\","
           "\"offline\":true},\"74:83:c2:0d:f6:a5\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"74:83:c2:11:3a:a1\":{\"interface\""
           ":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"74:83:c2:39:e8:76\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"74:83:c2:3b:e8:76\":"
           "{\"interface\":\"wan\",\"last_seen\":0,\"ipv6\":[\"fe80:0:0:0:7683:c2ff:fe3b:e876\"]},\"74:83:c2:6d:91:ef\":{\"interface\""
           ":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"74:83:c2:73:9e:a6\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},"
           "\"74:83:c2:78:fb:13\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"74:83:c2:d0:a7:ee\":{\"interface\":\"wan\","
           "\"last_seen\":0,\"fdb\":[\"eth0\"]},\"74:83:c2:d0:a7:ef\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"74:83:c2:f9:a8:c6\":"
           "{\"interface\":\"wan\",\"last_seen\":0,\"ipv4\":[\"10.2.49.142\"],\"fdb\":[\"eth0\"]},\"76:83:c2:78:fb:13\":{\"interface\":\"wan\",\"last_seen\":0,"
           "\"fdb\":[\"eth0\"]},\"b4:fb:e4:46:41:ba\":{\"interface\":\"wan\",\"offline\":true},\"b4:fb:e4:b7:d9:4b\":{\"interface\":\"wan\",\"last_seen\":0,"
           "\"fdb\":[\"eth0\"]},\"dc:a6:32:0f:6a:0e\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"e0:63:da:57:61:84\":{\"interface\":"
           "\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"e0:63:da:86:64:8d\":{\"interface\":\"wan\",\"offline\":true},\"e0:63:da:86:64:8e\":{\"interface\":"
           "\"wan\",\"offline\":true},\"e0:63:da:86:64:8f\":{\"interface\":\"wan\",\"offline\":true},\"e0:63:da:86:64:90\":{\"interface\":\"wan\",\"offline\":true},"
           "\"e0:63:da:86:64:91\":{\"interface\":\"wan\",\"offline\":true},\"e0:63:da:86:64:92\":{\"interface\":\"wan\",\"offline\":true},\"e0:63:da:86:64:93\":"
           "{\"interface\":\"wan\",\"offline\":true},\"e0:63:da:86:64:94\":{\"interface\":\"wan\",\"offline\":true},\"e2:63:da:86:64:8e\":{\"interface\":\"wan\","
           "\"last_seen\":0,\"ipv4\":[\"10.2.0.1\"],\"ipv6\":[\"fe80:0:0:0:5c99:d6ff:fea0:9b50\",\"fe80:0:0:0:5c40:bdff:fe67:8961\",\"2604:3d08:9680:bd00:0:0:0:1\"],"
           "\"fdb\":[\"eth0\"]},\"fc:ec:da:46:db:06\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"fc:ec:da:7c:d8:8a\":{\"interface\":\"wan\","
           "\"last_seen\":0,\"fdb\":[\"eth0\"]},\"fc:ec:da:f3:12:62\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"fc:ec:da:f3:18:92\":{\"interface\""
           ":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]}},\"traffic\":{\"gretun_50\":{\"hwaddr\":\"4a:e1:e3:b7:7e:6b\",\"stats\":{\"rx_packets\":0,"
           "\"tx_packets\":8,\"rx_bytes\":0,\"tx_bytes\":896,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":0,\"tx_dropped\":0,\"multicast\":0,\"collisions\":0},"
           "\"bridge\":[\"gre4t-gre.50\"]},\"guest\":{\"hwaddr\":\"24:f5:a2:07:a1:32\",\"ipv4\":[\"192.168.12.11\"],\"ipv6\":[\"fe80::26f5:a2ff:fe07:a132%br-guest\"],"
           "\"stats\":{\"rx_packets\":0,\"tx_packets\":64692,\"rx_bytes\":0,\"tx_bytes\":13688092,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":0,\"tx_dropped\":0,"
           "\"multicast\":0,\"collisions\":0},\"bridge\":[\"wlan1\"]},\"lan\":{\"hwaddr\":\"24:f5:a2:07:a1:30\",\"ipv4\":[\"192.168.1.1\"],\"ipv6\":"
           "[\"fe80::26f5:a2ff:fe07:a130%br-lan\",\"fd50:1356:f3ff::1\"],\"stats\":{\"rx_packets\":2414716,\"tx_packets\":2285933,\"rx_bytes\":2032533370,"
           "\"tx_bytes\":2913005975,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":0,\"tx_dropped\":0,\"multicast\":94325,\"collisions\":0},\"bridge\":[\"eth0\","
           "\"wlan2-1\"]},\"nat200\":{\"hwaddr\":\"24:f5:a2:07:a1:33\",\"ipv4\":[\"192.168.16.1\"],\"ipv6\":[\"fe80::26f5:a2ff:fe07:a133%br-nat200\"],\"stats\":"
           "{\"rx_packets\":0,\"tx_packets\":64692,\"rx_bytes\":0,\"tx_bytes\":13688092,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":0,\"tx_dropped\":0,"
           "\"multicast\":0,\"collisions\":0},\"bridge\":[\"wlan2\"]},\"wan\":{\"hwaddr\":\"24:f5:a2:07:a1:31\",\"ipv4\":[\"10.2.155.233\"],\"ipv6\":"
           "[\"fe80::26f5:a2ff:fe07:a131%br-wan\",\"2604:3d08:9680:bd00::29d\",\"2604:3d08:9680:bd00:26f5:a2ff:fe07:a131\"],\"stats\":{\"rx_packets\":9357911,"
           "\"tx_packets\":4074275,\"rx_bytes\":3604333614,\"tx_bytes\":3644920217,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":333,\"tx_dropped\":0,"
           "\"multicast\":2636176,\"collisions\":0},\"bridge\":[\"wlan2-2\",\"eth1\"]},\"eth0\":{\"hwaddr\":\"24:f5:a2:07:a1:30\",\"stats\":{\"rx_packets\":2045320,"
           "\"tx_packets\":1672178,\"rx_bytes\":1848867981,\"tx_bytes\":1710677798,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":0,\"tx_dropped\":0,\"multicast\":0,"
           "\"collisions\":0}},\"eth1\":{\"hwaddr\":\"24:f5:a2:07:a1:31\",\"stats\":{\"rx_packets\":9928186,\"tx_packets\":4154021,\"rx_bytes\":3773908880,"
           "\"tx_bytes\":3650153837,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":43736,\"tx_dropped\":0,\"multicast\":0,\"collisions\":0}},\"gre4t-gre\":"
           "{\"hwaddr\":\"4a:e1:e3:b7:7e:6b\",\"stats\":{\"rx_packets\":0,\"tx_packets\":14,\"rx_bytes\":0,\"tx_bytes\":1316,\"rx_errors\":0,\"tx_errors\":0,"
           "\"rx_dropped\":0,\"tx_dropped\":1,\"multicast\":0,\"collisions\":0}},\"gre4t-gre.50\":{\"hwaddr\":\"4a:e1:e3:b7:7e:6b\",\"stats\":{\"rx_packets\":0,"
           "\"tx_packets\":8,\"rx_bytes\":0,\"tx_bytes\":896,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":0,\"tx_dropped\":0,\"multicast\":0,\"collisions\":0}},"
           "\"wlan1\":{\"hwaddr\":\"24:f5:a2:07:a1:32\",\"stats\":{\"rx_packets\":0,\"tx_packets\":64699,\"rx_bytes\":0,\"tx_bytes\":14853420,\"rx_errors\":0,"
           "\"tx_errors\":0,\"rx_dropped\":0,\"tx_dropped\":0,\"multicast\":0,\"collisions\":0}},\"wlan2\":{\"hwaddr\":\"24:f5:a2:07:a1:33\",\"stats\":{\"rx_packets\":0,"
           "\"tx_packets\":64699,\"rx_bytes\":0,\"tx_bytes\":14853420,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":0,\"tx_dropped\":0,\"multicast\":0,\"collisions\":0}},"
           "\"wlan2-1\":{\"hwaddr\":\"26:f5:a2:07:a1:33\",\"stats\":{\"rx_packets\":0,\"tx_packets\":216740,\"rx_bytes\":0,\"tx_bytes\":45023656,\"rx_errors\":0,\"tx_errors\":0,"
           "\"rx_dropped\":0,\"tx_dropped\":0,\"multicast\":0,\"collisions\":0}},\"wlan2-2\":{\"hwaddr\":\"22:f5:a2:07:a1:33\",\"stats\":{\"rx_packets\":0,\"tx_packets\":7712835,"
           "\"rx_bytes\":0,\"tx_bytes\":2573638616,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":0,\"tx_dropped\":0,\"multicast\":0,\"collisions\":0}}},\"lldp\":{}}");

}

void uCentralClient::Disconnect( bool Reconnect ) {
    my_guard guard(Mutex_);
    if(Connected_) {
        Reactor_.removeEventHandler(*WS_, Poco::NObserver<uCentralClient, Poco::Net::ReadableNotification>(*this,
                                                                                                           &uCentralClient::OnSocketReadable));
        Reactor_.removeEventHandler(*WS_, Poco::NObserver<uCentralClient, Poco::Net::ShutdownNotification>(*this,
                                                                                                           &uCentralClient::OnSocketShutdown));
        (*WS_).close();
    }

    Connected_ = false ;
    Commands_.erase(Commands_.begin(),Commands_.end());

    if(Reconnect)
        AddEvent(ev_reconnect,App()->GetReconnectInterval());
}

void uCentralClient::OnSocketReadable(const Poco::AutoPtr<Poco::Net::ReadableNotification>& pNf) {
    my_guard guard(Mutex_);

    try {
        char        Message[16000];
        int         Flags;

        auto MessageSize = WS_->receiveFrame(Message,sizeof(Message),Flags);
        auto Op = Flags & Poco::Net::WebSocket::FRAME_OP_BITMASK;

        if (MessageSize == 0 && Flags == 0 && Op == 0) {
            Disconnect(true);
            return;
        }

        Message[MessageSize]=0;
        switch (Op) {
            case Poco::Net::WebSocket::FRAME_OP_PING: {
                    WS_->sendFrame("", 0,Poco::Net::WebSocket::FRAME_OP_PONG | Poco::Net::WebSocket::FRAME_FLAG_FIN);
                }
                break;

            case Poco::Net::WebSocket::FRAME_OP_PONG: {
                }
                break;

            case Poco::Net::WebSocket::FRAME_OP_TEXT: {
                    if (MessageSize > 0) {
                        Poco::JSON::Parser  Parser;

                        auto ParsedMessage = Parser.parse(Message);
                        auto Result = ParsedMessage.extract<Poco::JSON::Object::Ptr>();
                        Poco::DynamicStruct Vars = *Result;

                        if( Vars.contains("jsonrpc") &&
                            Vars.contains("id") &&
                            Vars.contains("method"))
                        {
                            ProcessCommand(Vars);
                        } else {
                            std::cout << "Bad incoming message: " << Message << std::endl;
                        }
                    }
                }
                break;
            default: {
                }
                break;
        }
        return;
    }
    catch ( const Poco::Net::SSLException & E )
    {
        std::cout << "Caught SSL exception: " << E.displayText() << std::endl;
    }
    catch ( const Poco::Exception & E )
    {
        std::cout << "Caught exception: " << E.displayText() << std::endl;
    }
    Disconnect(true);
}

void uCentralClient::ProcessCommand(Poco::DynamicStruct Vars) {

    auto ParamsObj = Vars["params"];

    if(!ParamsObj.isStruct())
    {
        std::cout << "Command does not contain proper parameters." << std::endl;
        return;
    }

    Poco::DynamicStruct Params = ParamsObj.extract<Poco::DynamicStruct>();

    auto Method = Vars["method"].toString();
    auto Id = Vars["id"];

    if(Method == "configure") {
        DoConfigure(Id,Params);
    } else if(Method =="reboot") {
        DoReboot(Id,Params);
    } else if(Method == "upgrade") {
        DoUpgrade(Id,Params);
    } else if(Method == "factory") {
        DoFactory(Id,Params);
    } else if(Method == "blink") {
        DoBlink(Id,Params);
    } else if(Method == "perform") {
        DoPerform(Id,Params);
    } else if(Method == "trace") {
        DoTrace(Id,Params);
    } else {
        std::cout << "Unknown method: " << Method << std::endl;
    }
}

void  uCentralClient::DoConfigure(uint64_t Id, Poco::DynamicStruct Params) {
    my_guard guard(Mutex_);

    try {
        if (Params.contains("serial") &&
            Params.contains("uuid") &&
            Params.contains("config")) {
            uint64_t When = Params.contains("when") ? (uint64_t) Params["when"] : 0;
            auto Serial = Params["serial"].toString();
            uint64_t UUID = Params["uuid"];
            auto Configuration = Params["config"];

            //  We need to store the configuration  in the object...
            std::stringstream OS;
            Poco::JSON::Stringifier::stringify(Configuration, OS);

            CurrentConfig_ = OS.str();

            UUID_ = Active_ = UUID ;

            //  prepare response...
            Poco::JSON::Object Answer;

            Answer.set("jsonrpc", "2.0");
            Answer.set("id", Id);

            Poco::JSON::Object Result;

            Result.set("serial", Serial);
            Result.set("uuid", UUID);

            Poco::JSON::Object Status;
            Status.set("error", 0);
            Status.set("when", When);
            Status.set("text", "No errors were found");
            Result.set("status", Status);
            Answer.set("result", Result);

            Logger_.information(Poco::format("configure(%s): done.",SerialNumber_));
            SendObject(Answer);
        } else {
            Logger_.warning(Poco::format("configure(%s): Illegal command.",SerialNumber_));
        }
    } catch (const Poco::Exception &E)
    {
        Logger_.warning(Poco::format("configure(%s): Exception. %s",SerialNumber_,E.displayText()));
    }
}

void  uCentralClient::DoReboot(uint64_t Id, Poco::DynamicStruct Params) {
    my_guard guard(Mutex_);
    try {
        if (Params.contains("serial")) {
            uint64_t When = Params.contains("when") ? (uint64_t) Params["when"] : 0;
            auto Serial = Params["serial"].toString();

            //  prepare response...
            Poco::JSON::Object Answer;

            Answer.set("jsonrpc", "2.0");
            Answer.set("id", Id);

            Poco::JSON::Object Result;

            Result.set("serial", Serial);

            Poco::JSON::Object Status;
            Status.set("error", 0);
            Status.set("when", When);
            Status.set("text", "No errors were found");
            Result.set("status", Status);
            Answer.set("result", Result);
            SendObject(Answer);
            Logger_.information(Poco::format("reboot(%s): done.",SerialNumber_));
            Disconnect(true);
        } else {
            Logger_.warning(Poco::format("reboot(%s): Illegal command.",SerialNumber_));
        }
    } catch( const Poco::Exception &E )
    {
        Logger_.warning(Poco::format("reboot(%s): Exception. %s",SerialNumber_,E.displayText()));
    }
}

void  uCentralClient::DoUpgrade(uint64_t Id, Poco::DynamicStruct Params) {
    my_guard guard(Mutex_);
    try {
        if (Params.contains("serial") &&
            Params.contains("uri")) {

            uint64_t When = Params.contains("when") ? (uint64_t) Params["when"] : 0;
            auto Serial = Params["serial"].toString();
            auto URI = Params["uri"].toString();

            //  prepare response...
            Poco::JSON::Object Answer;

            Answer.set("jsonrpc", "2.0");
            Answer.set("id", Id);

            Version_++;
            SetFirmware();

            Poco::JSON::Object Result;

            Result.set("serial", Serial);

            Poco::JSON::Object Status;
            Status.set("error", 0);
            Status.set("when", When);
            Status.set("text", "No errors were found");
            Result.set("status", Status);
            Answer.set("result", Result);
            SendObject(Answer);
            Logger_.information(Poco::format("upgrade(%s): from URI=%s.",SerialNumber_,URI));
            Disconnect(true);
        } else {
            Logger_.warning(Poco::format("upgrade(%s): Illegal command.",SerialNumber_));
        }
    } catch( const Poco::Exception &E )
    {
        Logger_.warning(Poco::format("upgrade(%s): Exception. %s",SerialNumber_,E.displayText()));
    }
}

void  uCentralClient::DoFactory(uint64_t Id, Poco::DynamicStruct Params) {
    my_guard guard(Mutex_);
    try {
        if (Params.contains("serial") &&
            Params.contains("keep_redirector")) {

            uint64_t When = Params.contains("when") ? (uint64_t) Params["when"] : 0;
            auto Serial = Params["serial"].toString();
            auto KeepRedirector = Params["uri"];

            //  prepare response...
            Poco::JSON::Object Answer;

            Answer.set("jsonrpc", "2.0");
            Answer.set("id", Id);

            Version_ = 1;
            SetFirmware();
            KeepRedirector_ = KeepRedirector;

            Poco::JSON::Object Result;

            Result.set("serial", Serial);

            Poco::JSON::Object Status;
            Status.set("error", 0);
            Status.set("when", When);
            Status.set("text", "No errors were found");
            Result.set("status", Status);
            Answer.set("result", Result);
            SendObject(Answer);
            Logger_.information(Poco::format("factory(%s): done.",SerialNumber_));
            Disconnect(true);
        } else {
            Logger_.warning(Poco::format("factory(%s): Illegal command.",SerialNumber_));
        }
    } catch( const Poco::Exception &E )
    {
        Logger_.warning(Poco::format("factory(%s): Exception. %s",SerialNumber_,E.displayText()));
    }
}

void  uCentralClient::DoBlink(uint64_t Id, Poco::DynamicStruct Params) {
    my_guard guard(Mutex_);
    try {
        if (Params.contains("serial") &&
            Params.contains("duration")) {

            uint64_t    When = Params.contains("when") ? (uint64_t) Params["when"] : 0;
            auto        Serial = Params["serial"].toString();
            uint64_t    Duration = Params["durarion"];

            //  prepare response...
            Poco::JSON::Object Answer;

            Answer.set("jsonrpc", "2.0");
            Answer.set("id", Id);

            Poco::JSON::Object Result;

            Result.set("serial", Serial);

            Poco::JSON::Object Status;
            Status.set("error", 0);
            Status.set("when", When);
            Status.set("text", "No errors were found");
            Result.set("status", Status);
            Answer.set("result", Result);
            SendObject(Answer);
            Logger_.information(Poco::format("blink(%s): for %Lu ms.",SerialNumber_,Duration));
        } else {
            Logger_.warning(Poco::format("blink(%s): Illegal command.",SerialNumber_));
        }
    } catch( const Poco::Exception &E )
    {
        Logger_.warning(Poco::format("blink(%s): Exception. %s",SerialNumber_,E.displayText()));
    }
}

void  uCentralClient::DoPerform(uint64_t Id, Poco::DynamicStruct Params) {
    my_guard guard(Mutex_);
    try {
        if (Params.contains("serial") &&
            Params.contains("command") &&
            Params.contains("payload")) {

            uint64_t    When = Params.contains("when") ? (uint64_t) Params["when"] : 0;
            auto        Serial = Params["serial"].toString();
            auto        Command = Params["command"].toString();
            auto        Payload = Params["payload"];

            //  prepare response...
            Poco::JSON::Object Answer;

            Answer.set("jsonrpc", "2.0");
            Answer.set("id", Id);

            Poco::JSON::Object Result;

            Result.set("serial", Serial);

            Poco::JSON::Object Status;
            Status.set("error", 0);
            Status.set("when", When);
            Status.set("text", "No errors were found");
            Status.set("resultCode",0);
            Status.set("resultText","no return status");
            Result.set("status", Status);
            Answer.set("result", Result);
            SendObject(Answer);
            Logger_.information(Poco::format("perform(%s): command=%s.",SerialNumber_,Command));
        } else {
            Logger_.warning(Poco::format("perform(%s): Illegal command.",SerialNumber_));
        }
    } catch( const Poco::Exception &E )
    {
        Logger_.warning(Poco::format("perform(%s): Exception. %s",SerialNumber_,E.displayText()));
    }
}

void  uCentralClient::DoTrace(uint64_t Id, Poco::DynamicStruct Params) {
    my_guard guard(Mutex_);
    try {
        if (Params.contains("serial") &&
            Params.contains("duration") &&
            Params.contains("network") &&
            Params.contains("interface") &&
            Params.contains("packets") &&
            Params.contains("uri")) {

            uint64_t    When = Params.contains("when") ? (uint64_t) Params["when"] : 0;
            auto        Serial = Params["serial"].toString();
            auto        Network = Params["network"].toString();
            auto        Interface = Params["interface"].toString();
            uint64_t    Duration = Params["duration"];
            uint64_t    Packets = Params["packets"];
            auto        URI = Params["uri"].toString();

            //  prepare response...
            Poco::JSON::Object Answer;

            Answer.set("jsonrpc", "2.0");
            Answer.set("id", Id);

            Poco::JSON::Object Result;

            Result.set("serial", Serial);

            Poco::JSON::Object Status;
            Status.set("error", 0);
            Status.set("when", When);
            Status.set("text", "No errors were found");
            Status.set("resultCode",0);
            Status.set("resultText","no return status");
            Result.set("status", Status);
            Answer.set("result", Result);
            SendObject(Answer);
            Logger_.information(Poco::format("trace(%s): network=%s interface=%s packets=%Lu duration=%Lu URI=%s.",SerialNumber_,Network,
                                             Interface, Packets, Duration, URI));
        } else {
            Logger_.warning(Poco::format("trace(%s): Illegal command.",SerialNumber_));
        }
    } catch( const Poco::Exception &E )
    {
        Logger_.warning(Poco::format("trace(%s): Exception. %s",SerialNumber_,E.displayText()));
    }
}


void uCentralClient::OnSocketShutdown(const Poco::AutoPtr<Poco::Net::ShutdownNotification>& pNf) {
    std::cout << "Serial:" << SerialNumber_ << " disconnecting" << std::endl;
    // delete this;
};

void uCentralClient::Terminate() {
    my_guard guard(Mutex_);
    Disconnect(false);
}

void uCentralClient::EstablishConnection() {
    Poco::URI   uri(URI_);

    Poco::Net::HTTPSClientSession Session(  uri.getHost(),
                                            uri.getPort(),
                                            new Poco::Net::Context( Poco::Net::Context::CLIENT_USE,
                                            CertFileName_,
                                            Poco::Net::Context::VERIFY_NONE));
    Poco::Net::HTTPRequest Request(Poco::Net::HTTPRequest::HTTP_GET, "/?encoding=text",Poco::Net::HTTPMessage::HTTP_1_1);
    // Request.set("origin", "http://www.websocket.org");
    Poco::Net::HTTPResponse Response;

    Logger_.information(Poco::format("connecting(%s): host=%s port=%Lu",SerialNumber_,uri.getHost(),(uint64_t )uri.getPort()));

    try {
        my_guard guard(Mutex_);

        WS_ = std::make_unique<Poco::Net::WebSocket>(Session, Request, Response);

        Reactor_.addEventHandler(*WS_, Poco::NObserver<uCentralClient,
                                 Poco::Net::ReadableNotification>(*this, &uCentralClient::OnSocketReadable));
        Connected_ = true ;

        //  Add all the first commands...
        Commands_.erase( Commands_.begin(), Commands_.end());
        AddEvent(ev_connect,1);
    }
    catch ( const Poco::Exception & E )
    {
        Logger_.warning(Poco::format("connecting(%s): exception. %s",SerialNumber_,E.displayText()));
        AddEvent(ev_reconnect,App()->GetReconnectInterval());
    }
}

bool uCentralClient::Send(const std::string & Cmd) {
    my_guard guard(Mutex_);

    WS_->sendFrame(Cmd.c_str(),Cmd.size());

    return true;
}

bool uCentralClient::SendWSPing() {
    my_guard guard(Mutex_);

    WS_->sendFrame("", 0,Poco::Net::WebSocket::FRAME_OP_PING | Poco::Net::WebSocket::FRAME_FLAG_FIN);
    return true;
}

bool uCentralClient::SendObject(Poco::JSON::Object O) {
    my_guard guard(Mutex_);

    std::stringstream OS;
    Poco::JSON::Stringifier::stringify( O, OS );
    auto BytesSent = WS_->sendFrame(OS.str().c_str(),OS.str().size());
    return BytesSent == OS.str().size();
}

static const uint64_t million = 1000000;

void uCentralClient::AddEvent(uCentralEventType E, uint64_t InSeconds) {
    my_guard guard(Mutex_);

    timeval curTime{0};
    gettimeofday(&curTime, nullptr);
    uint64_t NextCommand = (InSeconds * million) + (curTime.tv_sec * million) + curTime.tv_usec;

    //  we need to make sure we do not possibly overwrite other commands at the same time
    while(Commands_.find(NextCommand)!=Commands_.end())
        NextCommand++;

    Commands_[NextCommand] = E;
}

uCentralEventType uCentralClient::NextEvent() {
    my_guard guard(Mutex_);

    if(Commands_.empty())
        return ev_none;

    auto EventTime = Commands_.begin()->first;
    timeval curTime{0};
    gettimeofday(&curTime, nullptr);
    uint64_t Now = (curTime.tv_sec * million) + curTime.tv_usec;

    if(EventTime<Now) {
        uCentralEventType E = Commands_.begin()->second;
        Commands_.erase(Commands_.begin());
        return E;
    }

    return ev_none;
}

