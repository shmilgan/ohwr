#ifndef __WHITERABBIT_MVRP_STRUCTS_H
#define __WHITERABBIT_MVRP_STRUCTS_H

#include "mvrp.h"

struct mvrp_get_last_pdu_origin_retdata {
    int retval;
    uint8_t mac[ETH_ALEN];
};

struct mvrp_register_vlan_argdata {
    int vid;
    uint32_t egress_ports;
    uint32_t forbidden_ports;
};

extern struct minipc_pd
    mvrp_enable_struct,
    mvrp_disable_struct,
    mvrp_enable_port_struct,
    mvrp_disable_port_struct,
    mvrp_get_failed_registrations_struct,
    mvrp_get_last_pdu_origin_struct,
    mvrp_is_enabled_struct,
    mvrp_is_enabled_port_struct,
    mvrp_register_vlan_struct,
    mvrp_deregister_vlan_struct;

#endif /*__WHITERABBIT_MVRP_STRUCTS_H*/
