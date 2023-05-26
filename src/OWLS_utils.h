//
// Created by stephane bourque on 2023-04-12.
//

#pragma once

#include <random>

#include <framework/MicroServiceFuncs.h>
#include <framework/ow_constants.h>

namespace OpenWifi {

    enum class radio_bands {
        band_2g, band_5g, band_6g
    };

   inline  std::string to_string(radio_bands b) {
        switch(b) {
            case radio_bands::band_5g: return "5G";
            case radio_bands::band_6g: return "6G";
            default:
                return "2G";
        }
    }

    namespace OWLSutils {

/*        template<typename T>
        void AssignIfPresent(const nlohmann::json &doc, const char *name, T &Value, T default_value) {
            if (doc.contains(name) && !doc[name].is_null())
                Value = doc[name];
            else
                Value = default_value;
        }
*/
        template<typename T>
        void AssignIfPresent(const Poco::JSON::Object::Ptr &doc, const char *name, T &Value, T default_value) {
            if (doc->has(name) && !doc->isNull(name)) {
                Value = doc->get(name);
            } else {
                Value = default_value;
            }
        }

        inline std::string MakeMac(const char *S, int offset) {
            char b[256];

            int j = 0, i = 0;
            for (int k = 0; k < 6; ++k) {
                b[j++] = S[i++];
                b[j++] = S[i++];
                b[j++] = ':';
            }
            b[--j] = 0;
            b[--j] = '0' + offset;
            return b;
        }

        inline std::string RandomMAC() {
            char b[64];
            snprintf(b, sizeof(b), "%02x:%02x:%02x:%02x:%02x:%02x", (int) MicroServiceRandom(255),
                     (int) MicroServiceRandom(255), (int) MicroServiceRandom(255),
                     (int) MicroServiceRandom(255), (int) MicroServiceRandom(255),
                     (int) MicroServiceRandom(255));
            return b;
        }

        inline std::string RandomIPv4() {
            char b[64];
            snprintf(b, sizeof(b), "%d.%d.%d.%d", (int) MicroServiceRandom(255),
                     (int) MicroServiceRandom(255), (int) MicroServiceRandom(255),
                     (int) MicroServiceRandom(255));
            return b;
        }

        inline std::string RandomIPv6() {
            char b[128];
            snprintf(b, sizeof(b), "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
                     (uint) MicroServiceRandom(0x0ffff), (uint) MicroServiceRandom(0x0ffff),
                     (uint) MicroServiceRandom(0x0ffff), (uint) MicroServiceRandom(0x0ffff),
                     (uint) MicroServiceRandom(0x0ffff), (uint) MicroServiceRandom(0x0ffff),
                     (uint) MicroServiceRandom(0x0ffff), (uint) MicroServiceRandom(0x0ffff));
            return b;
        }

        inline std::int64_t local_random(std::int64_t min, std::int64_t max) {
            static std::random_device rd;
            static std::mt19937_64 gen{rd()};
            if (min > max)
                std::swap(min, max);
            std::uniform_int_distribution<> dis(min, max);
            return dis(gen);
        }

        inline auto local_random(std::int64_t max) { return local_random(0, max); }

        static std::vector<std::uint64_t> Channels_2G{1, 6, 11};
        static std::vector<std::uint64_t>
                Channels_5G{36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 96, 100, 102, 104, 106,
                            108,
                            110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 132, 136, 138, 140, 142, 144, 149, 151,
                            153, 155, 157, 159, 161};
        static std::vector<std::uint64_t> Channels_6G{1, 5, 9, 13, 17, 21, 25, 29, 33, 37, 41, 45, 49, 53, 57, 61, 65,
                                                      69, 73, 77, 81, 85, 89, 93, 97, 101, 105, 109, 113, 117,
                                                      121, 125, 129, 133, 137, 141, 145, 149, 153, 157, 161, 165, 169,
                                                      173, 177, 181, 185, 189, 193, 197, 201, 205, 209,
                                                      213, 221, 225, 229, 233};

        inline void FillinFrequencies(std::uint64_t channel, radio_bands band, std::uint64_t width,
                                      std::vector<std::uint64_t> &channels, std::vector<std::uint64_t> &frequencies) {
            if (band == radio_bands::band_2g) { // 2.4GHz band
                if (channel >= 1 && channel <= 11) {
                    channels.push_back(channel);
                    std::uint64_t frequency = 2401 + (channel - 1) * 5;
                    frequencies.push_back(frequency);
                    if (width == 20) {
                        frequencies.push_back(frequency + 22);
                    } else if (width == 40) {
                        frequencies.push_back(frequency + 32);
                    } else if (width == 80) {
                        frequencies.push_back(frequency + 52);
                    } else {
                        throw std::invalid_argument("Invalid channel width for 2.4GHz band.");
                    }
                } else {
                    throw std::invalid_argument("Invalid channel number for 2.4GHz band.");
                }
            } else if (band == radio_bands::band_5g) { // 5GHz band
                if (channel >= 36 && channel <= 165) {
                    std::uint64_t frequency = 5170 + (channel - 36) * 5;
                    channels.push_back(channel);
                    if (width == 20) {
                        frequencies.push_back(frequency);
                        frequencies.push_back(frequency + 20);
                    } else if (width == 40) {
                        channels.push_back(channel + 2);
                        frequencies.push_back(frequency - 10);
                        frequencies.push_back(frequency + 30);
                    } else if (width == 80) {
                        channels.push_back(channel + 6);
                        frequencies.push_back(frequency - 30);
                        frequencies.push_back(frequency + 50);
                    } else if (width == 160) {
                        channels.push_back(channel + 12);
                        frequencies.push_back(frequency - 70);
                        frequencies.push_back(frequency + 90);
                    } else {
                        throw std::invalid_argument("Invalid channel width for 5GHz band.");
                    }
                } else {
                    throw std::invalid_argument("Invalid channel number for 5GHz band.");
                }
            } else if (band == radio_bands::band_6g) { // 6GHz band
                if (channel >= 1 && channel <= 233) {
                    std::uint64_t frequency = 5945 + (channel - 1) * 5;
                    channels.push_back(channel);
                    if (width == 20) {
                        frequencies.push_back(frequency);
                        frequencies.push_back(frequency + 19);
                    } else if (width == 40) {
                        channels.push_back(channel + 2);
                        frequencies.push_back(frequency - 10);
                        frequencies.push_back(frequency + 29);
                    } else if (width == 80) {
                        channels.push_back(channel + 6);
                        frequencies.push_back(frequency - 30);
                        frequencies.push_back(frequency + 49);
                    } else if (width == 160) {
                        channels.push_back(channel + 12);
                        frequencies.push_back(frequency - 70);
                        frequencies.push_back(frequency + 89);
                    } else {
                        throw std::invalid_argument("Invalid channel width for 6GHz band.");
                    }
                } else {
                    throw std::invalid_argument("Invalid channel number for 6GHz band.");
                }
            } else {
                throw std::invalid_argument("Invalid band number.");
            }
        }


        inline std::uint64_t FindAutoChannel(radio_bands band, [[maybe_unused]] std::uint64_t channel_width) {
            std::uint64_t num_chan = 1;
            if (channel_width == 20) {
                num_chan = 1;
            } else if (channel_width == 40) {
                num_chan = 2;
            } else if (channel_width == 80) {
                num_chan = 3;
            } else if (channel_width == 160) {
                num_chan = 4;
            } else if (channel_width == 320) {
                num_chan = 5;
            }
            switch (band) {
                case radio_bands::band_2g:
                    return Channels_2G[std::rand() % Channels_2G.size()];
                case radio_bands::band_5g:
                    return Channels_5G[std::rand() % (Channels_5G.size() - num_chan)];
                case radio_bands::band_6g:
                    return Channels_6G[std::rand() % (Channels_6G.size() - num_chan)];
            }
            return Channels_2G[std::rand() % Channels_2G.size()];
        }

        inline void MakeHeader(Poco::JSON::Object &Message, const char *method, const Poco::JSON::Object &Params) {
            Message.set(uCentralProtocol::JSONRPC, "2.0");
            Message.set(uCentralProtocol::METHOD, method);
            Message.set(uCentralProtocol::PARAMS, Params);
        }

        inline void MakeRPCHeader(Poco::JSON::Object &Message, std::uint64_t Id, const Poco::JSON::Object &Result) {
            Message.set(uCentralProtocol::JSONRPC, "2.0");
            Message.set(uCentralProtocol::ID, Id);
            Message.set(uCentralProtocol::RESULT, Result);
        }

        inline bool is_integer(const std::string &s) {
            return std::all_of(s.begin(),s.end(),[](char c) { return std::isdigit(c); });
        }

    }
}