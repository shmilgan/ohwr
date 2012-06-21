#include "minipc.h"

#include "mvrp_structs.h"

struct minipc_pd mvrp_enable_struct = {
    .name   = "enable",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_END,
    }
};

struct minipc_pd mvrp_disable_struct = {
    .name   = "disable",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_END,
    }
};

struct minipc_pd mvrp_enable_port_struct = {
    .name   = "enable_port",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
        MINIPC_ARG_END,
    }
};

struct minipc_pd mvrp_disable_port_struct = {
    .name   = "disable_port",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
        MINIPC_ARG_END,
    }
};

struct minipc_pd mvrp_get_failed_registrations_struct = {
    .name   = "get_fail_reg",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
        MINIPC_ARG_END,
    }
};

struct minipc_pd mvrp_get_last_pdu_origin_struct = {
    .name   = "get_last_pdu_origin",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_get_last_pdu_origin_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
        MINIPC_ARG_END,
    }
};

struct minipc_pd mvrp_is_enabled_struct = {
    .name   = "is_enabled",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_END,
    }
};

struct minipc_pd mvrp_is_enabled_port_struct = {
    .name   = "is_enabled_port",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
        MINIPC_ARG_END,
    }
};

struct minipc_pd mvrp_register_vlan_struct = {
    .name   = "register_vlan",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_register_vlan_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd mvrp_deregister_vlan_struct = {
    .name   = "deregister_vlan",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
        MINIPC_ARG_END,
    }
};

