//
// Created by stephane bourque on 2023-04-12.
//

#pragma once

#include <framework/MicroServiceFuncs.h>
#include <nlohmann/json.hpp>
#include "OWLS_utils.h"

namespace OpenWifi {

    struct MockElement {
        explicit MockElement(std::uint64_t S) : size(S) {

        }

        virtual void next() { last_update = Utils::Now(); };
        virtual void reset() = 0;
        virtual void to_json(nlohmann::json &json ) const = 0;
        virtual void to_json(Poco::JSON::Object &json ) const = 0;
        virtual ~MockElement() = default;
        void SetSize(std::uint64_t S) { size = S; }
        std::uint64_t   last_update = Utils::Now();
        std::uint64_t   size = 0;
    };

    struct MockMemory : public MockElement {
        std::uint64_t       total=973131776, free=0, cached=0, buffered=0;

        explicit MockMemory(std::uint64_t S) : MockElement(S) {

        }

        void next() final {
            MockElement::next();
        }

        void reset() final {

        }

        void to_json(nlohmann::json &json) const final {
            json["unit"]["memory"]["buffered"] = buffered;
            json["unit"]["memory"]["cached"] = cached;
            json["unit"]["memory"]["free"] = free;
            json["unit"]["memory"]["total"] = total;
        }

        void to_json(Poco::JSON::Object &json ) const final {
            Poco::JSON::Object  Memory;
            Memory.set("buffered", buffered);
            Memory.set("cached", cached);
            Memory.set("free", free);
            Memory.set("total", total);
            Poco::JSON::Object  Unit;
            Unit.set("memory", Memory);
            json.set("unit", Unit);
        }

    };

    struct MockCPULoad : public MockElement {
        std::double_t       load_1=0.0, load_5=0.0, load_15=0.0;

        explicit MockCPULoad(std::uint64_t S) : MockElement(S) {

        }

        void next() final {
            MockElement::next();

        }

        void reset() final {

        }

        void to_json(nlohmann::json &json) const final {
            json["unit"]["load"] = std::vector<std::double_t> { load_1, load_5, load_15};
        }

        void to_json(Poco::JSON::Object &json) const final {
            auto LoadArray = std::vector<std::double_t> { load_1, load_5, load_15};
            Poco::JSON::Object  ObjArr;
            ObjArr.set("load", LoadArray);
            json.set("unit",ObjArr);
        }
    };

    struct MockLanClient : public MockElement {
        std::vector<std::string> ipv6_addresses, ipv4_addresses, ports;
        std::string mac;

        explicit MockLanClient() : MockElement(1) {

        }

        void next() final {
            MockElement::next();

        }

        void reset() final {

        }

        void to_json(nlohmann::json &json) const final {
            json["ipv4_addresses"] = ipv4_addresses;
            json["ipv6_addresses"] = ipv6_addresses;
            json["ports"] = ports;
            json["mac"] = mac;
        }

        void to_json(Poco::JSON::Object &json) const final {
            json.set("ipv4_addresses", ipv4_addresses);
            json.set("ipv6_addresses", ipv6_addresses);
            json.set("ports", ports);
            json.set("mac", mac);
        }

    };
    typedef std::vector<MockLanClient> MockLanClients;

    struct MockAssociation : public MockElement {
        std::string station;
        std::string bssid;
        std::string ipaddr_v4;
        std::string ipaddr_v6;
        uint64_t tx_bytes = 0, rx_bytes = 0, rx_duration = 0, rx_packets = 0, connected = 0,
                inactive = 0, tx_duration = 0, tx_failed = 0, tx_packets = 0, tx_retries = 0;
        int64_t ack_signal = 0, ack_signal_avg = OWLSutils::local_random(-65, -75),
                rssi = OWLSutils::local_random(-40, -90);

        MockAssociation() : MockElement(1) {

        }

        void to_json(nlohmann::json &json) const final {
            json["ack_signal"] = ack_signal;
            json["ack_signal_avg"] = ack_signal_avg;
            json["bssid"] = bssid;
            json["station"] = station;
            json["connected"] = connected;
            json["inactive"] = inactive;
            json["ipaddr_v4"] = ipaddr_v4;
            json["rssi"] = rssi;
            json["rx_bytes"] = rx_bytes;
            json["rx_duration"] = rx_duration;
            json["rx_packets"] = rx_packets;
            json["rx_rate"]["bitrate"] = 200000;
            json["rx_rate"]["chwidth"] = 40;
            json["rx_rate"]["mcs"] = 9;
            json["rx_rate"]["nss"] = 9;
            json["rx_rate"]["sgi"] = true;
            json["rx_rate"]["vht"] = true;

            json["tx_bytes"] = tx_bytes;
            json["tx_duration"] = tx_duration;
            json["tx_failed"] = tx_failed;
            json["tx_packets"] = tx_packets;
            json["tx_retries"] = tx_retries;

            json["tx_rate"]["bitrate"] = 200000;
            json["tx_rate"]["chwidth"] = 40;
            json["tx_rate"]["mcs"] = 9;
            json["tx_rate"]["sgi"] = true;
            json["tx_rate"]["ht"] = true;
        }

        void to_json(Poco::JSON::Object &json) const final {
            json.set("ack_signal", ack_signal);
            json.set("ack_signal_avg", ack_signal_avg);
            json.set("bssid", bssid);
            json.set("station", station);
            json.set("connected", connected);
            json.set("inactive", inactive);
            json.set("ipaddr_v4", ipaddr_v4);
            json.set("rssi", rssi);
            json.set("rx_bytes", rx_bytes);
            json.set("rx_duration", rx_duration);
            json.set("rx_packets", rx_packets);
            json.set("tx_packets", tx_packets);
            json.set("tx_retries", tx_retries);

            Poco::JSON::Object  rx_rate;
            rx_rate.set("bitrate", 200000);
            rx_rate.set("chwidth", 40);
            rx_rate.set("mcs", 9);
            rx_rate.set("nss", 9);
            rx_rate.set("sgi", true);
            rx_rate.set("vht", true);
            json.set("rx_rate", rx_rate);

            json.set("tx_bytes", tx_bytes);
            json.set("tx_duration", tx_duration);
            json.set("tx_failed", tx_failed);
            json.set("tx_packets", tx_packets);
            json.set("tx_retries", tx_retries);

            Poco::JSON::Object  tx_rate;
            tx_rate.set("bitrate", 200000);
            tx_rate.set("chwidth", 40);
            tx_rate.set("mcs", 9);
            tx_rate.set("sgi", true);
            tx_rate.set("ht", true);
            json.set("tx_rate", tx_rate);
        }

        void next() final {
            MockElement::next();

            ack_signal = ack_signal_avg + OWLSutils::local_random(-5, 5);
            connected += OWLSutils::local_random(100, 500);
            inactive += OWLSutils::local_random(100, 500);
            rssi += OWLSutils::local_random(-2, 2);

            auto new_rx_packets = OWLSutils::local_random(200, 2000);
            rx_packets += new_rx_packets;
            rx_bytes += new_rx_packets * OWLSutils::local_random(500, 1000);
            rx_duration += OWLSutils::local_random(400, 1750);

            auto new_tx_packets = OWLSutils::local_random(100, 300);
            tx_packets += new_tx_packets;
            tx_bytes += new_tx_packets * OWLSutils::local_random(500, 1000);
            tx_duration += OWLSutils::local_random(400, 1750);
            tx_failed += OWLSutils::local_random(3) * OWLSutils::local_random(800, 1200);
            tx_retries += OWLSutils::local_random(3);
        }

        void reset() final {
            ack_signal = ack_signal_avg;
            connected = 0;
            inactive = 0;

            rx_packets = 0;
            rx_bytes = 0;
            rx_duration = 0;

            tx_packets = 0;
            tx_bytes = 0;
            tx_duration = 0;
            tx_failed = 0;
            tx_retries = 0;
        }
    };
    typedef std::vector<MockAssociation> MockAssociations;

    struct MockRadio : public MockElement {
        std::uint64_t   active_ms = 0,
                busy_ms = 0,
                channel = 0,
                channel_width = 40,
                receive_ms = 0,
                transmit_ms = 0,
                tx_power = 23;
        std::int64_t    noise = -100,
                temperature = 40;
        std::string     phy;
        uint64_t        index = 0;
        radio_bands     radioBands = radio_bands::band_2g;
        std::vector<std::uint64_t>  channels;
        std::vector<std::uint64_t>  frequency;
        std::vector<std::string>    band;

        explicit MockRadio() : MockElement(1) {

        }

        void to_json(nlohmann::json &json) const final {
            json["active_ms"] = active_ms;
            json["busy_ms"] = busy_ms;
            json["receive_ms"] = receive_ms;
            json["transmit_ms"] = transmit_ms;
            json["noise"] = noise;
            json["temperature"] = temperature;
            json["channel"] = channel;
            json["channel_width"] = std::to_string(channel_width);
            json["tx_power"] = tx_power;
            json["phy"] = phy;
            json["channels"] = channels;
            json["frequency"] = frequency;
            json["band"] = band;
        }

        void to_json(Poco::JSON::Object &json) const final {
            json.set("active_ms", active_ms);
            json.set("busy_ms", busy_ms);
            json.set("receive_ms", receive_ms);
            json.set("transmit_ms", transmit_ms);
            json.set("noise", noise);
            json.set("temperature", temperature);
            json.set("channel", channel);
            json.set("channel_width", std::to_string(channel_width));
            json.set("tx_power", tx_power);
            json.set("phy", phy);
            json.set("channels", channels);
            json.set("frequency", frequency);
            json.set("band", band);
        }

        void next() final {
            MockElement::next();
            temperature = 50 + OWLSutils::local_random(-7, 7);
            noise = -95 + OWLSutils::local_random(-3, 3);
            active_ms += OWLSutils::local_random(100, 2000);
            busy_ms += OWLSutils::local_random(200, 3000);
            receive_ms += OWLSutils::local_random(500, 1500);
            transmit_ms += OWLSutils::local_random(250, 100);
        }

        void reset() final {
            active_ms = 0;
            busy_ms = 0;
            receive_ms = 0;
            transmit_ms = 0;
        }
    };

    struct MockCounters : public MockElement {
        std::uint64_t collisions = 0, multicast = 0, rx_bytes = 0, rx_dropped = 0, rx_errors = 0,
                rx_packets = 0, tx_bytes = 0, tx_dropped = 0, tx_errors = 0, tx_packets = 0;

        MockCounters() : MockElement(1) {

        }

        void to_json(nlohmann::json &json) const final {
            json["collisions"] = collisions;
            json["multicast"] = multicast;
            json["rx_bytes"] = rx_bytes;
            json["rx_dropped"] = rx_dropped;
            json["rx_errors"] = rx_errors;
            json["rx_packets"] = rx_packets;
            json["tx_bytes"] = tx_bytes;
            json["tx_dropped"] = tx_dropped;
            json["tx_errors"] = tx_errors;
            json["tx_packets"] = tx_packets;
       }

        void to_json(Poco::JSON::Object &json) const final {
            json.set("collisions", collisions);
            json.set("multicast", multicast);
            json.set("rx_bytes", rx_bytes);
            json.set("rx_dropped", rx_dropped);
            json.set("rx_errors", rx_errors);
            json.set("rx_packets", rx_packets);
            json.set("tx_bytes", tx_bytes);
            json.set("tx_dropped", tx_dropped);
            json.set("tx_errors", tx_errors);
            json.set("tx_packets", tx_packets);
        }

        void next() final {
            MockElement::next();
            multicast += OWLSutils::local_random(10, 100);

            collisions += OWLSutils::local_random(1);
            rx_dropped += OWLSutils::local_random(2);
            rx_errors += OWLSutils::local_random(3);
            auto new_rx_packets = OWLSutils::local_random(300, 2000);
            rx_packets += new_rx_packets;
            rx_bytes += new_rx_packets * OWLSutils::local_random(900, 1400);

            tx_dropped += OWLSutils::local_random(2);
            tx_errors += OWLSutils::local_random(3);
            auto new_tx_packets = OWLSutils::local_random(300, 2000);
            tx_packets += new_tx_packets;
            tx_bytes += new_tx_packets * OWLSutils::local_random(900, 1400);
        }

        void reset() final {
            multicast = 0;
            collisions = 0;
            rx_dropped = 0;
            rx_errors = 0;
            rx_packets = 0;
            rx_bytes = 0;
            tx_dropped = 0;
            tx_errors = 0;
            tx_packets = 0;
            tx_bytes = 0;
        }
    };

}

