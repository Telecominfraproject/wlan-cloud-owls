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
        [[nodiscard]] virtual nlohmann::json to_json() const = 0;
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

        [[nodiscard]] nlohmann::json to_json() const final {
            nlohmann::json result;
            result["memory"]["buffered"] = buffered;
            result["memory"]["cached"] = cached;
            result["memory"]["free"] = free;
            result["memory"]["total"] = total;
            return result;
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

        [[nodiscard]] nlohmann::json to_json() const final {
            nlohmann::json result;
            result["load"] = std::vector<std::double_t> { load_1, load_5, load_15};
            return result;
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

        [[nodiscard]] nlohmann::json to_json() const final {
            nlohmann::json res;
            res["ipv4_addresses"] = ipv4_addresses;
            res["ipv6_addresses"] = ipv6_addresses;
            res["ports"] = ports;
            res["mac"] = mac;
            return res;
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

        [[nodiscard]] nlohmann::json to_json() const final {
            nlohmann::json res;
            res["ack_signal"] = ack_signal;
            res["ack_signal_avg"] = ack_signal_avg;
            res["bssid"] = bssid;
            res["station"] = station;
            res["connected"] = connected;
            res["inactive"] = inactive;
            res["ipaddr_v4"] = ipaddr_v4;
            res["rssi"] = rssi;
            res["rx_bytes"] = rx_bytes;
            res["rx_duration"] = rx_duration;
            res["rx_packets"] = rx_packets;
            res["rx_rate"]["bitrate"] = 200000;
            res["rx_rate"]["chwidth"] = 40;
            res["rx_rate"]["mcs"] = 9;
            res["rx_rate"]["nss"] = 9;
            res["rx_rate"]["sgi"] = true;
            res["rx_rate"]["vht"] = true;

            res["tx_bytes"] = tx_bytes;
            res["tx_duration"] = tx_duration;
            res["tx_failed"] = tx_failed;
            res["tx_packets"] = tx_packets;
            res["tx_retries"] = tx_retries;

            res["tx_rate"]["bitrate"] = 200000;
            res["tx_rate"]["chwidth"] = 40;
            res["tx_rate"]["mcs"] = 9;
            res["tx_rate"]["sgi"] = true;
            res["tx_rate"]["ht"] = true;

            return res;
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

        [[nodiscard]] nlohmann::json to_json() const final {
            nlohmann::json res;
            res["active_ms"] = active_ms;
            res["busy_ms"] = busy_ms;
            res["receive_ms"] = receive_ms;
            res["transmit_ms"] = transmit_ms;
            res["noise"] = noise;
            res["temperature"] = temperature;
            res["channel"] = channel;
            res["channel_width"] = std::to_string(channel_width);
            res["tx_power"] = tx_power;
            res["phy"] = phy;
            res["channels"] = channels;
            res["frequency"] = frequency;
            res["band"] = band;
            return res;
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

        [[nodiscard]] nlohmann::json to_json() const final {
            nlohmann::json res;

            res["collisions"] = collisions;
            res["multicast"] = multicast;
            res["rx_bytes"] = rx_bytes;
            res["rx_dropped"] = rx_dropped;
            res["rx_errors"] = rx_errors;
            res["rx_packets"] = rx_packets;
            res["tx_bytes"] = tx_bytes;
            res["tx_dropped"] = tx_dropped;
            res["tx_errors"] = tx_errors;
            res["tx_packets"] = tx_packets;

            return res;
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

