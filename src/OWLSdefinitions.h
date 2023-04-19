//
// Created by stephane bourque on 2021-04-03.
//

#pragma once

#include <mutex>

namespace OpenWifi {
    enum OWLSeventType {
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
        ev_update
    };

    using my_mutex = std::recursive_mutex;
    using my_guard = std::lock_guard<my_mutex>;

    enum ap_interface_types { upstream, downstream };

#define DEBUG_LINE(X)      std::cout << __LINE__ << ": " << __func__ << "  :" << X << std::endl;

}
