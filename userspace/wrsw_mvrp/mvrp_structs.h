#ifndef __WHITERABBIT_MVRP_STRUCTS_H
#define __WHITERABBIT_MVRP_STRUCTS_H

#include "mvrp.h"

struct mvrp_get_last_pdu_origin_retdata {
    int retval;
    uint8_t mac[ETH_ALEN];
};

extern struct minipc_pd
    mvrp_enable_struct,
    mvrp_disable_struct,
    mvrp_enable_port_struct,
    mvrp_disable_port_struct,
    mvrp_get_failed_registrations_struct,
    mvrp_get_last_pdu_origin_struct,
    mvrp_is_enabled_struct,
    mvrp_is_enabled_port_struct;

#endif /*__WHITERABBIT_MVRP_STRUCTS_H*/
