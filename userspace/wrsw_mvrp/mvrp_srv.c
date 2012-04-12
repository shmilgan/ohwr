#include "mvrp_srv.h"

static int mvrp_srv_enable(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    mvrp_enable();
    return 0;
}

static int mvrp_srv_disable(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    mvrp_disable();
    return 0;
}

static int mvrp_srv_enable_port(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct mvrp_enable_port_argdata *in;
    struct mvrp_enable_port_retdata *out;
    in  = (struct mvrp_enable_port_argdata*)args;
    out = (struct mvrp_enable_port_retdata*)ret;

    out->retval = mvrp_enable_port(in->port_no);

    return 0;
}

static int mvrp_srv_disable_port(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct mvrp_disable_port_argdata *in;
    struct mvrp_disable_port_retdata *out;
    in  = (struct mvrp_disable_port_argdata*)args;
    out = (struct mvrp_disable_port_retdata*)ret;

    out->retval = mvrp_disable_port(in->port_no);

    return 0;
}

static int mvrp_srv_get_failed_registrations(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct mvrp_get_failed_registrations_argdata *in;
    struct mvrp_get_failed_registrations_retdata *out;
    in  = (struct mvrp_get_failed_registrations_argdata*)args;
    out = (struct mvrp_get_failed_registrations_retdata*)ret;

    out->retval = mvrp_get_failed_registrations(in->port_no);

    return 0;
}

static int mvrp_srv_get_last_pdu_origin(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct mvrp_get_last_pdu_origin_argdata *in;
    struct mvrp_get_last_pdu_origin_retdata *out;
    in  = (struct mvrp_get_last_pdu_origin_argdata*)args;
    out = (struct mvrp_get_last_pdu_origin_retdata*)ret;

    out->retval = mvrp_get_last_pdu_origin(in->port_no, &out->mac);

    return 0;
}

static int mvrp_srv_is_enabled(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct mvrp_is_enabled_retdata *out;
    out = (struct mvrp_is_enabled_retdata*)ret;

    out->retval = mvrp_is_enabled();

    return 0;
}

static int mvrp_srv_is_enabled_port(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct mvrp_is_enabled_port_argdata *in;
    struct mvrp_is_enabled_port_retdata *out;
    in  = (struct mvrp_is_enabled_port_argdata*)args;
    out = (struct mvrp_is_enabled_port_retdata*)ret;

    out->retval = mvrp_is_enabled_port(in->port_no);

    return 0;
}


const struct minipc_pd mvrp_srv_enable_struct = {
    .f      = mvrp_srv_enable,
    .name   = "0",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_enable_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_enable_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd mvrp_srv_disable_struct = {
    .f      = mvrp_srv_disable,
    .name   = "1",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_disable_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_disable_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd mvrp_srv_enable_port_struct = {
    .f      = mvrp_srv_enable_port,
    .name   = "2",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_enable_port_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_enable_port_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd mvrp_srv_disable_port_struct = {
    .f      = mvrp_srv_disable_port,
    .name   = "3",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_disable_port_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_disable_port_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd mvrp_srv_get_failed_registrations_struct = {
    .f      = mvrp_srv_get_failed_registrations,
    .name   = "4",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_get_failed_registrations_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_get_failed_registrations_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd mvrp_srv_get_last_pdu_origin_struct = {
    .f      = mvrp_srv_get_last_pdu_origin,
    .name   = "5",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_get_last_pdu_origin_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_get_last_pdu_origin_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd mvrp_srv_is_enabled_struct = {
    .f      = mvrp_srv_is_enabled,
    .name   = "6",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_is_enabled_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_is_enabled_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd mvrp_srv_is_enabled_port_struct = {
    .f      = mvrp_srv_is_enabled_port,
    .name   = "7",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_is_enabled_port_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct mvrp_is_enabled_port_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_ch *mvrp_srv_create(char *name)
{
    struct minipc_ch *server;

    server = minipc_server_create(name, 0);
    if (server) {
        minipc_set_logfile(server, stderr);

        minipc_export(server, &mvrp_srv_enable_struct);
        minipc_export(server, &mvrp_srv_disable_struct);
        minipc_export(server, &mvrp_srv_enable_port_struct);
        minipc_export(server, &mvrp_srv_disable_port_struct);
        minipc_export(server, &mvrp_srv_get_failed_registrations_struct);
        minipc_export(server, &mvrp_srv_get_last_pdu_origin_struct);
        minipc_export(server, &mvrp_srv_is_enabled_struct);
        minipc_export(server, &mvrp_srv_is_enabled_port_struct);
    }
    return server;
}
