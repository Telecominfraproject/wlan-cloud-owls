//
// Created by stephane bourque on 2021-03-12.
//
#include <iostream>

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

#include "Simulator.h"

std::string uCentralClient::DefaultCapabilities() {
    return std::string("{\"model\":{\"id\":\"linksys,ea8300\",\"name\":\"Linksys EA8300 (Dallas)\"},\"network\":{\"lan\":{\"ifname\":\"eth0\",\"protocol\":\"static\"},\"wan\":{\"ifname\":\"eth1\",\"protocol\":\"dhcp\"}},\"switch\":{\"switch0\":{\"enable\":true,\"reset\":true,\"ports\":[{\"num\":0,\"device\":\"eth0\",\"need_tag\":false,\"want_untag\":true},{\"num\":1,\"role\":\"lan\"},{\"num\":2,\"role\":\"lan\"},{\"num\":3,\"role\":\"lan\"},{\"num\":4,\"role\":\"lan\"}],\"roles\":[{\"role\":\"lan\",\"ports\":\"1 2 3 4 0\",\"device\":\"eth0\"}]}},\"wifi\":{\"soc/40000000.pci/pci0000:00/0000:00:00.0/0000:01:00.0\":{\"band\":[\"5u\"],\"ht_capa\":6639,\"vht_capa\":865696178,\"htmode\":[\"HT20\",\"HT40\",\"VHT20\",\"VHT40\",\"VHT80\"],\"tx_ant\":3,\"rx_ant\":3,\"channels\":[100,104,108,112,116,120,124,128,132,136,140,144,149,153,157,161,165]},\"platform/soc/a000000.wifi\":{\"band\":[\"2\"],\"ht_capa\":6639,\"vht_capa\":865687986,\"htmode\":[\"HT20\",\"HT40\",\"VHT20\",\"VHT40\",\"VHT80\"],\"tx_ant\":3,\"rx_ant\":3,\"channels\":[1,2,3,4,5,6,7,8,9,10,11]},\"platform/soc/a800000.wifi\":{\"band\":[\"5l\"],\"ht_capa\":6639,\"vht_capa\":865687986,\"htmode\":[\"HT20\",\"HT40\",\"VHT20\",\"VHT40\",\"VHT80\"],\"tx_ant\":3,\"rx_ant\":3,\"channels\":[36,40,44,48,52,56,60,64]}}}");
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
    return std::string("{\"uuid\":1615694464,\"cfg_uuid\":1615694035,\"system\":{\"localtime\":1615698064,\"uptime\":1312691,\"load\":[14272,10944,11232],\"memory\":{\"total\":254595072,\"free\":107769856,\"shared\":135168,\"buffered\":6066176,\"available\":96727040,\"cached\":16949248},\"swap\":{\"total\":0,\"free\":0}},\"wifi-iface\":{\"wlan1\":{\"ssid\":\"uCentral-Guest\",\"mode\":\"ap\",\"frequency\":[2437,2437],\"ch_width\":\"20\",\"tx_power\":20,\"mac\":\"24:f5:a2:07:a1:32\"},\"wlan2-2\":{\"ssid\":\"uCentral\",\"mode\":\"ap\",\"frequency\":[5180,5210],\"ch_width\":\"80\",\"tx_power\":23,\"mac\":\"24:f5:a2:07:a1:33\"},\"wlan2-1\":{\"ssid\":\"uCentral-EAP\",\"mode\":\"ap\",\"frequency\":[5180,5210],\"ch_width\":\"80\",\"tx_power\":23,\"mac\":\"24:f5:a2:07:a1:33\"},\"wlan2\":{\"ssid\":\"uCentral-NAT.200\",\"mode\":\"ap\",\"frequency\":[5180,5210],\"ch_width\":\"80\",\"tx_power\":23,\"mac\":\"24:f5:a2:07:a1:33\"}},\"neighbours\":{\"00:14:ee:0a:e6:a9\":{\"interface\":\"wan\",\"last_seen\":0,\"ipv4\":[\"10.2.207.137\"],\"fdb\":[\"eth0\"]},\"00:23:a4:05:05:95\":{\"interface\":\"lan\",\"last_seen\":0,\"ipv4\":[\"192.168.1.176\"],\"ipv6\":[\"fe80:0:0:0:482:423e:7fdd:f4ad\",\"fe80:0:0:0:1c40:2ff8:b4e1:a4d7\",\"fe80:0:0:0:1cff:d5ea:9462:9a79\",\"fe80:0:0:0:1ca9:72e6:237a:31c0\"],\"dhcpv4\":[\"192.168.1.176\",\"renegademac\"],\"fdb\":[\"eth0\"]},\"18:e8:29:25:f7:0a\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"22:f5:a2:07:a1:33\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"wlan2-1\"]},\"24:f5:a2:07:a1:30\":{\"interface\":\"lan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"24:f5:a2:07:a1:31\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"24:f5:a2:07:a1:32\":{\"interface\":\"guest\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"24:f5:a2:07:a1:33\":{\"interface\":\"nat200\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"26:f5:a2:07:a1:33\":{\"interface\":\"lan\",\"last_seen\":0,\"fdb\":[\"wlan2-1\"]},\"26:f5:a2:07:a1:34\":{\"interface\":\"lan\",\"offline\":true},\"28:11:a5:f5:79:f3\":{\"interface\":\"wan\",\"last_seen\":0,\"ipv6\":[\"fe80:0:0:0:2a11:a5ff:fef5:79f3\"],\"fdb\":[\"eth0\"]},\"38:c9:86:33:e2:e6\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"40:62:31:07:e4:6b\":{\"interface\":\"wan\",\"offline\":true},\"40:62:31:07:e4:8f\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"4a:e1:e3:b7:7e:6b\":{\"interface\":\"gretun_50\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"6a:24:99:dc:3c:28\":{\"interface\":\"lan\",\"last_seen\":0,\"ipv6\":[\"fe80:0:0:0:d2:e972:f1c7:34c6\"],\"dhcpv4\":[\"192.168.1.101\",\"StephaneiPhone\"]},\"6c:33:a9:27:8e:c6\":{\"interface\":\"wan\",\"offline\":true},\"74:83:c2:0d:f6:a5\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"74:83:c2:11:3a:a1\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"74:83:c2:39:e8:76\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"74:83:c2:3b:e8:76\":{\"interface\":\"wan\",\"last_seen\":0,\"ipv6\":[\"fe80:0:0:0:7683:c2ff:fe3b:e876\"]},\"74:83:c2:6d:91:ef\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"74:83:c2:73:9e:a6\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"74:83:c2:78:fb:13\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"74:83:c2:d0:a7:ee\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"74:83:c2:d0:a7:ef\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"74:83:c2:f9:a8:c6\":{\"interface\":\"wan\",\"last_seen\":0,\"ipv4\":[\"10.2.49.142\"],\"fdb\":[\"eth0\"]},\"76:83:c2:78:fb:13\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"b4:fb:e4:46:41:ba\":{\"interface\":\"wan\",\"offline\":true},\"b4:fb:e4:b7:d9:4b\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"dc:a6:32:0f:6a:0e\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"e0:63:da:57:61:84\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"e0:63:da:86:64:8d\":{\"interface\":\"wan\",\"offline\":true},\"e0:63:da:86:64:8e\":{\"interface\":\"wan\",\"offline\":true},\"e0:63:da:86:64:8f\":{\"interface\":\"wan\",\"offline\":true},\"e0:63:da:86:64:90\":{\"interface\":\"wan\",\"offline\":true},\"e0:63:da:86:64:91\":{\"interface\":\"wan\",\"offline\":true},\"e0:63:da:86:64:92\":{\"interface\":\"wan\",\"offline\":true},\"e0:63:da:86:64:93\":{\"interface\":\"wan\",\"offline\":true},\"e0:63:da:86:64:94\":{\"interface\":\"wan\",\"offline\":true},\"e2:63:da:86:64:8e\":{\"interface\":\"wan\",\"last_seen\":0,\"ipv4\":[\"10.2.0.1\"],\"ipv6\":[\"fe80:0:0:0:5c99:d6ff:fea0:9b50\",\"fe80:0:0:0:5c40:bdff:fe67:8961\",\"2604:3d08:9680:bd00:0:0:0:1\"],\"fdb\":[\"eth0\"]},\"fc:ec:da:46:db:06\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"fc:ec:da:7c:d8:8a\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"fc:ec:da:f3:12:62\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]},\"fc:ec:da:f3:18:92\":{\"interface\":\"wan\",\"last_seen\":0,\"fdb\":[\"eth0\"]}},\"traffic\":{\"gretun_50\":{\"hwaddr\":\"4a:e1:e3:b7:7e:6b\",\"stats\":{\"rx_packets\":0,\"tx_packets\":8,\"rx_bytes\":0,\"tx_bytes\":896,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":0,\"tx_dropped\":0,\"multicast\":0,\"collisions\":0},\"bridge\":[\"gre4t-gre.50\"]},\"guest\":{\"hwaddr\":\"24:f5:a2:07:a1:32\",\"ipv4\":[\"192.168.12.11\"],\"ipv6\":[\"fe80::26f5:a2ff:fe07:a132%br-guest\"],\"stats\":{\"rx_packets\":0,\"tx_packets\":64692,\"rx_bytes\":0,\"tx_bytes\":13688092,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":0,\"tx_dropped\":0,\"multicast\":0,\"collisions\":0},\"bridge\":[\"wlan1\"]},\"lan\":{\"hwaddr\":\"24:f5:a2:07:a1:30\",\"ipv4\":[\"192.168.1.1\"],\"ipv6\":[\"fe80::26f5:a2ff:fe07:a130%br-lan\",\"fd50:1356:f3ff::1\"],\"stats\":{\"rx_packets\":2414716,\"tx_packets\":2285933,\"rx_bytes\":2032533370,\"tx_bytes\":2913005975,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":0,\"tx_dropped\":0,\"multicast\":94325,\"collisions\":0},\"bridge\":[\"eth0\",\"wlan2-1\"]},\"nat200\":{\"hwaddr\":\"24:f5:a2:07:a1:33\",\"ipv4\":[\"192.168.16.1\"],\"ipv6\":[\"fe80::26f5:a2ff:fe07:a133%br-nat200\"],\"stats\":{\"rx_packets\":0,\"tx_packets\":64692,\"rx_bytes\":0,\"tx_bytes\":13688092,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":0,\"tx_dropped\":0,\"multicast\":0,\"collisions\":0},\"bridge\":[\"wlan2\"]},\"wan\":{\"hwaddr\":\"24:f5:a2:07:a1:31\",\"ipv4\":[\"10.2.155.233\"],\"ipv6\":[\"fe80::26f5:a2ff:fe07:a131%br-wan\",\"2604:3d08:9680:bd00::29d\",\"2604:3d08:9680:bd00:26f5:a2ff:fe07:a131\"],\"stats\":{\"rx_packets\":9357911,\"tx_packets\":4074275,\"rx_bytes\":3604333614,\"tx_bytes\":3644920217,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":333,\"tx_dropped\":0,\"multicast\":2636176,\"collisions\":0},\"bridge\":[\"wlan2-2\",\"eth1\"]},\"eth0\":{\"hwaddr\":\"24:f5:a2:07:a1:30\",\"stats\":{\"rx_packets\":2045320,\"tx_packets\":1672178,\"rx_bytes\":1848867981,\"tx_bytes\":1710677798,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":0,\"tx_dropped\":0,\"multicast\":0,\"collisions\":0}},\"eth1\":{\"hwaddr\":\"24:f5:a2:07:a1:31\",\"stats\":{\"rx_packets\":9928186,\"tx_packets\":4154021,\"rx_bytes\":3773908880,\"tx_bytes\":3650153837,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":43736,\"tx_dropped\":0,\"multicast\":0,\"collisions\":0}},\"gre4t-gre\":{\"hwaddr\":\"4a:e1:e3:b7:7e:6b\",\"stats\":{\"rx_packets\":0,\"tx_packets\":14,\"rx_bytes\":0,\"tx_bytes\":1316,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":0,\"tx_dropped\":1,\"multicast\":0,\"collisions\":0}},\"gre4t-gre.50\":{\"hwaddr\":\"4a:e1:e3:b7:7e:6b\",\"stats\":{\"rx_packets\":0,\"tx_packets\":8,\"rx_bytes\":0,\"tx_bytes\":896,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":0,\"tx_dropped\":0,\"multicast\":0,\"collisions\":0}},\"wlan1\":{\"hwaddr\":\"24:f5:a2:07:a1:32\",\"stats\":{\"rx_packets\":0,\"tx_packets\":64699,\"rx_bytes\":0,\"tx_bytes\":14853420,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":0,\"tx_dropped\":0,\"multicast\":0,\"collisions\":0}},\"wlan2\":{\"hwaddr\":\"24:f5:a2:07:a1:33\",\"stats\":{\"rx_packets\":0,\"tx_packets\":64699,\"rx_bytes\":0,\"tx_bytes\":14853420,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":0,\"tx_dropped\":0,\"multicast\":0,\"collisions\":0}},\"wlan2-1\":{\"hwaddr\":\"26:f5:a2:07:a1:33\",\"stats\":{\"rx_packets\":0,\"tx_packets\":216740,\"rx_bytes\":0,\"tx_bytes\":45023656,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":0,\"tx_dropped\":0,\"multicast\":0,\"collisions\":0}},\"wlan2-2\":{\"hwaddr\":\"22:f5:a2:07:a1:33\",\"stats\":{\"rx_packets\":0,\"tx_packets\":7712835,\"rx_bytes\":0,\"tx_bytes\":2573638616,\"rx_errors\":0,\"tx_errors\":0,\"rx_dropped\":0,\"tx_dropped\":0,\"multicast\":0,\"collisions\":0}}},\"lldp\":{}}");
}

bool uCentralClient::SendCommand(const std::string &Cmd) {
    return true;
}

void uCentralClient::Disconnect() {
    Reactor_.removeEventHandler(*WS_, Poco::NObserver<uCentralClient,Poco::Net::ReadableNotification>(*this,&uCentralClient::OnSocketReadable));
    Reactor_.removeEventHandler(*WS_, Poco::NObserver<uCentralClient,Poco::Net::ShutdownNotification>(*this,&uCentralClient::OnSocketShutdown));
    WS_->shutdown();
    Connected_ = false;
    Simulator::instance()->Reconnect(SerialNumber_);
}

void uCentralClient::OnSocketReadable(const Poco::AutoPtr<Poco::Net::ReadableNotification>& pNf) {
    // std::cout << "Serial:" << SerialNumber_ << " received message" << std::endl;
    try {
        char        Message[16000];
        uint64_t    MessageSize;
        int         Flags;

        MessageSize = WS_->receiveFrame(Message,sizeof(Message),Flags);

        if(MessageSize==0)
        {
            std::cout << "Serial: " << SerialNumber_ << " shutting down socket." << std::endl;
            Disconnect();
        }
    }
    catch ( const Poco::Net::SSLException & E )
    {
        std::cout << "Caught SSL exception: " << E.displayText() << std::endl;
        Disconnect();
    }
    catch ( const Poco::Exception & E )
    {
        std::cout << "Caught exception: " << E.displayText() << std::endl;
    }
}

void uCentralClient::OnSocketShutdown(const Poco::AutoPtr<Poco::Net::ShutdownNotification>& pNf) {
    std::cout << "Serial:" << SerialNumber_ << " disconnecting" << std::endl;
    // delete this;
};

void uCentralClient::Terminate() {
    Disconnect();
}

void uCentralClient::Connect() {
    Poco::URI   uri(URI_);

    Poco::Net::HTTPSClientSession Session(uri.getHost(),
                                     uri.getPort(),
                                     new Poco::Net::Context( Poco::Net::Context::CLIENT_USE,
                                                             CertFileName_));
    Poco::Net::HTTPRequest Request(Poco::Net::HTTPRequest::HTTP_GET, "/?encoding=text",Poco::Net::HTTPMessage::HTTP_1_1);
    Request.set("origin", "http://www.websocket.org");
    Poco::Net::HTTPResponse Response;

    try {
        // WS_ = std::shared_ptr<Poco::Net::WebSocket>(new Poco::Net::WebSocket( Session, Request, Response ));
        WS_.reset(new Poco::Net::WebSocket( Session, Request, Response ));

        if(Protocol_==legacy) {
            std::stringstream Message;
            Message << "{ \"serial\":\"" << SerialNumber_ << "\", \"capab\":" << DefaultCapabilities() << "}";
            WS_->sendFrame(Message.str().c_str(), Message.str().size());

            //  Register for notifications...
            Reactor_.addEventHandler(*WS_, Poco::NObserver<uCentralClient, Poco::Net::ReadableNotification>(*this,
                                                                                                            &uCentralClient::OnSocketReadable));
            Reactor_.addEventHandler(*WS_, Poco::NObserver<uCentralClient, Poco::Net::ShutdownNotification>(*this,
                                                                                                            &uCentralClient::OnSocketShutdown));
            Connected_ = true;

            Simulator::instance()->HeartBeat(SerialNumber_);
        }
        else
        {

        }
    }
    catch ( const Poco::Exception & E )
    {
        std::cout << "Could not connect" << std::endl;
        Simulator::instance()->Reconnect(SerialNumber_);
    }
}

void uCentralClient::SendHeartBeat() {
    if(Protocol_==legacy) {
        char Message[256];
        int MsgSize = std::snprintf( Message, sizeof(Message), "{ \"serial\":\"%s\", \"uuid\" : %llu }" , SerialNumber_.c_str(),CurrentConfigUUID_);
        WS_->sendFrame(Message,MsgSize);
        Simulator::instance()->SendState(SerialNumber_);
    }
}

void uCentralClient::SendState() {
    if(Protocol_==legacy) {

    }
    else
    {

    }
    Simulator::instance()->SendState(SerialNumber_);
}
