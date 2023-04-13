//
// Created by stephane bourque on 2023-04-13.
//

#pragma once

#include <cstdint>

namespace OpenWifi {
    struct CensusReport {
        std::uint64_t
            ev_none,
            ev_reconnect,
            ev_connect,
            ev_state,
            ev_healthcheck,
            ev_log,
            ev_crashlog,
            ev_configpendingchange,
            ev_keepalive,
            ev_reboot,
            ev_disconnect,
            ev_wsping,
            ev_update,
            ev_configure,
            ev_firmwareupgrade,
            ev_factory,
            ev_leds,
            ev_trace,
            ev_perform,
            ev_establish_connection,
            protocol_tx,
            protocol_rx,
            client_tx,
            client_rx
            ;

        void Reset() {
            ev_none =
            ev_reconnect =
            ev_connect =
            ev_state =
            ev_healthcheck =
            ev_log =
            ev_crashlog =
            ev_configpendingchange =
            ev_keepalive =
            ev_reboot =
            ev_disconnect =
            ev_wsping =
            ev_update =
            ev_configure =
            ev_firmwareupgrade =
            ev_factory =
            ev_leds =
            ev_trace =
            ev_perform =
            protocol_tx =
            protocol_rx =
            client_tx =
            client_rx =
            ev_establish_connection = 0;
        }
    };
}

