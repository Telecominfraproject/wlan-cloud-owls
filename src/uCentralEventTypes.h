//
// Created by stephane bourque on 2021-04-03.
//

#ifndef UCENTRAL_CLNT_UCENTRALEVENTTYPES_H
#define UCENTRAL_CLNT_UCENTRALEVENTTYPES_H

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

#endif //UCENTRAL_CLNT_UCENTRALEVENTTYPES_H
