//
// Created by stephane bourque on 2021-04-03.
//

#ifndef UCENTRAL_CLNT_UCENTRALEVENTTYPES_H
#define UCENTRAL_CLNT_UCENTRALEVENTTYPES_H

#include <mutex>

enum uCentralEventType {
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
	ev_wsping
};

using my_mutex = std::recursive_mutex;
using my_guard = std::lock_guard<my_mutex>;

#endif // UCENTRAL_CLNT_UCENTRALEVENTTYPES_H
