#include <stdlib.h>

#include "mac.h"
#include "mvrp_proxy.h"

struct minipc_ch *client;

const struct minipc_pd mvrp_proxy_enable_struct = {
    .name   = "0",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_enable_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_enable_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd mvrp_proxy_disable_struct = {
    .name   = "1",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_disable_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_disable_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd mvrp_proxy_enable_port_struct = {
    .name   = "2",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_enable_port_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_enable_port_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd mvrp_proxy_disable_port_struct = {
    .name   = "3",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_disable_port_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_disable_port_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd mvrp_proxy_get_failed_registrations_struct = {
    .name   = "4",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_get_failed_registrations_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_get_failed_registrations_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd mvrp_proxy_get_last_pdu_origin_struct = {
    .name   = "5",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_get_last_pdu_origin_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_get_last_pdu_origin_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd mvrp_proxy_is_enabled_struct = {
    .name   = "6",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_is_enabled_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_is_enabled_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd mvrp_proxy_is_enabled_port_struct = {
    .name   = "7",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_is_enabled_port_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_is_enabled_port_argdata),
        MINIPC_ARG_END,
    }
};


void mvrp_proxy_enable(void)
{
    struct mvrp_enable_argdata in;
    struct mvrp_enable_retdata out;

    minipc_call(client, MILLISEC_TIMEOUT, &mvrp_proxy_enable_struct, &out, &in);
}

void mvrp_proxy_disable(void)
{
    struct mvrp_disable_argdata in;
    struct mvrp_disable_retdata out;

    minipc_call(client, MILLISEC_TIMEOUT, &mvrp_proxy_disable_struct, &out, &in);
}

int mvrp_proxy_enable_port(int port_no)
{
    struct mvrp_enable_port_argdata in;
    struct mvrp_enable_port_retdata out;

    in.port_no = port_no;

    minipc_call(client, MILLISEC_TIMEOUT, &mvrp_proxy_enable_port_struct, &out, &in);

    return out.retval;
}

int mvrp_proxy_disable_port(int port_no)
{
    struct mvrp_disable_port_argdata in;
    struct mvrp_disable_port_retdata out;

    in.port_no = port_no;

    minipc_call(client, MILLISEC_TIMEOUT, &mvrp_proxy_disable_port_struct, &out, &in);

    return out.retval;
}

int mvrp_proxy_get_failed_registrations(int port_no)
{
    struct mvrp_get_failed_registrations_argdata in;
    struct mvrp_get_failed_registrations_retdata out;

    in.port_no = port_no;

    minipc_call(client, MILLISEC_TIMEOUT, &mvrp_proxy_get_failed_registrations_struct, &out, &in);

    return out.retval;
}

int mvrp_proxy_get_last_pdu_origin(int port_no, uint8_t (*mac)[ETH_ALEN])
{
    int ret;
    struct mvrp_get_last_pdu_origin_argdata in;
    struct mvrp_get_last_pdu_origin_retdata out;

    in.port_no = port_no;

    ret = minipc_call(client, MILLISEC_TIMEOUT, &mvrp_proxy_get_last_pdu_origin_struct, &out, &in);

    if (ret == 0) {
        mac_copy(*mac, out.mac);
    }

    return out.retval;
}

int mvrp_proxy_is_enabled(void)
{
    struct mvrp_is_enabled_argdata in;
    struct mvrp_is_enabled_retdata out;

    minipc_call(client, MILLISEC_TIMEOUT, &mvrp_proxy_is_enabled_struct, &out, &in);

    return out.retval;
}

int mvrp_proxy_is_enabled_port(int port_no)
{
    struct mvrp_is_enabled_port_argdata in;
    struct mvrp_is_enabled_port_retdata out;

    in.port_no = port_no;

    minipc_call(client, MILLISEC_TIMEOUT, &mvrp_proxy_is_enabled_port_struct, &out, &in);

    return out.retval;
}



struct minipc_ch *mvrp_proxy_create(char *name)
{
    if (!client) {
    	client = minipc_client_create(name, 0);
        if (client)
    	    minipc_set_logfile(client, stderr);
    }
    return client;
}
