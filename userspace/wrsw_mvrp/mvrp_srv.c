#include "mvrp_srv.h"
#include "mvrp_structs.h"

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
    int port_no = args[0];

    *(int *)ret = mvrp_enable_port(port_no);
    return 0;
}

static int mvrp_srv_disable_port(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    int port_no = args[0];

    *(int *)ret = mvrp_disable_port(port_no);
    return 0;
}

static int mvrp_srv_get_failed_registrations(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    int port_no = args[0];

    *(int *)ret = mvrp_get_failed_registrations(port_no);
    return 0;
}

static int mvrp_srv_get_last_pdu_origin(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    int port_no = args[0];
    struct mvrp_get_last_pdu_origin_retdata *out;
    out = (struct mvrp_get_last_pdu_origin_retdata*)ret;

    out->retval = mvrp_get_last_pdu_origin(port_no, &out->mac);

    return 0;
}

static int mvrp_srv_is_enabled(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    *(int *)ret = mvrp_is_enabled();
    return 0;
}

static int mvrp_srv_is_enabled_port(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    int port_no = args[0];

    *(int *)ret = mvrp_is_enabled_port(port_no);
    return 0;
}

static int mvrp_srv_register_vlan(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct mvrp_register_vlan_argdata *in;
    in  = (struct mvrp_register_vlan_argdata*)args;
    
    *(int *)ret = mvrp_register_vlan(in->vid, in->egress_ports, in->forbidden_ports);
    return 0;
}

static int mvrp_srv_deregister_vlan(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    int vid = args[0];

    *(int *)ret = mvrp_deregister_vlan(vid);
    return 0;
}

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

struct minipc_ch *mvrp_srv_create(char *name)
{
    int i;
    struct minipc_ch *server;

    static struct {
        struct minipc_pd *desc;
        minipc_f *f;
    } export_list [] = {
        {&mvrp_enable_struct, mvrp_srv_enable},
        {&mvrp_disable_struct, mvrp_srv_disable},
        {&mvrp_enable_port_struct, mvrp_srv_enable_port},
        {&mvrp_disable_port_struct, mvrp_srv_disable_port},
        {&mvrp_get_failed_registrations_struct, mvrp_srv_get_failed_registrations},
        {&mvrp_get_last_pdu_origin_struct, mvrp_srv_get_last_pdu_origin},
        {&mvrp_is_enabled_struct, mvrp_srv_is_enabled},
        {&mvrp_is_enabled_port_struct, mvrp_srv_is_enabled_port},
        {&mvrp_register_vlan_struct, mvrp_srv_register_vlan},
        {&mvrp_deregister_vlan_struct, mvrp_srv_deregister_vlan},        
    };

    server = minipc_server_create(name, 0);
    if (server) {
        minipc_set_logfile(server, stderr);

	    for (i = 0; i < ARRAY_SIZE(export_list); i++) {
		    export_list[i].desc->f = export_list[i].f;
		    if (minipc_export(server, export_list[i].desc) < 0)
		        return NULL;
        }
    }
    return server;
}
