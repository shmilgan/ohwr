#ifndef __WHITERABBIT_MVRP_EXPORT_H
#define __WHITERABBIT_MVRP_EXPORT_H

#include "minipc.h"
#include "mvrp.h"

struct mvrp_enable_argdata {
    // void
};

struct mvrp_enable_retdata {
    uint32_t padding;   // TODO remove. Required to pass mini-ipc size check
};


struct mvrp_disable_argdata {
    // void
};

struct mvrp_disable_retdata {
    uint32_t padding;   // TODO remove. Required to pass mini-ipc size check
};


struct mvrp_enable_port_argdata {
    int port_no;
};

struct mvrp_enable_port_retdata {
    int retval;
};


struct mvrp_disable_port_argdata {
    int port_no;
};

struct mvrp_disable_port_retdata {
    int retval;
};


struct mvrp_get_failed_registrations_argdata {
    int port_no;
};

struct mvrp_get_failed_registrations_retdata {
    int retval;
};


struct mvrp_get_last_pdu_origin_argdata {
    int port_no;
};

struct mvrp_get_last_pdu_origin_retdata {
    int retval;
    uint8_t mac[ETH_ALEN];
};


struct mvrp_is_restricted_vlan_registration_argdata {
    int port_no;
};

struct mvrp_is_restricted_vlan_registration_retdata {
    int retval;
};


struct mvrp_set_restricted_vlan_registration_argdata {
    int port_no;
};

struct mvrp_set_restricted_vlan_registration_retdata {
    int retval;
};


struct mvrp_unset_restricted_vlan_registration_argdata {
    int port_no;
};

struct mvrp_unset_restricted_vlan_registration_retdata {
    int retval;
};

struct mvrp_is_enabled_argdata {
    // void
};

struct mvrp_is_enabled_retdata {
    int retval;
};


struct mvrp_is_enabled_port_argdata {
    int port_no;
};

struct mvrp_is_enabled_port_retdata {
    int retval;
};


#endif /*__WHITERABBIT_MVRP_EXPORT_H*/
