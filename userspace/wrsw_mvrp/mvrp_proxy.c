#include <stdlib.h>
#include <errno.h>

#include "mac.h"

#include "mvrp_proxy.h"
#include "mvrp_structs.h"

struct minipc_ch *client;

char name[MINIPC_MAX_NAME];

static struct minipc_ch *proxy_create()
{
    if (!client) {
    	client = minipc_client_create(name, 0);
        if (client)
    	    minipc_set_logfile(client, stderr);
    	else
    	    fprintf(stderr, "%s minipc proxy: error %s\n", name, strerror(errno));
    }
    return client;
}

static inline void check_conn(void)
{
    int err = errno;
    if (err == EPIPE || err == ENOTCONN) {
        minipc_close(client); 
        client = NULL;
        errno = err;
    }
}

void mvrp_proxy_enable(void)
{
    int out;

    if (!proxy_create())
        return;
    
    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT, &mvrp_enable_struct, &out) != 0)
        check_conn();    
}

void mvrp_proxy_disable(void)
{
    int out;

    if (!proxy_create())
        return -1;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT, &mvrp_disable_struct, &out) != 0)
        check_conn();    
}

int mvrp_proxy_enable_port(int port_no)
{
    int out;

    if (!proxy_create())
        return -1;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT, 
        &mvrp_enable_port_struct, &out, port_no) != 0)
        check_conn();    
    return out;
}

int mvrp_proxy_disable_port(int port_no)
{
    int out;

    if (!proxy_create())
        return -1;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT, 
        &mvrp_disable_port_struct, &out, port_no) != 0)
        check_conn();    
    return out;
}

int mvrp_proxy_get_failed_registrations(int port_no)
{
    int out;

    if (!proxy_create())
        return -1;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
        &mvrp_get_failed_registrations_struct, &out, port_no) != 0)
        check_conn();    
    return out;
}

int mvrp_proxy_get_last_pdu_origin(int port_no, uint8_t (*mac)[ETH_ALEN])
{
    struct mvrp_get_last_pdu_origin_retdata out;

    if (!proxy_create())
        return -1;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
        &mvrp_get_last_pdu_origin_struct, &out, port_no) == 0)
        mac_copy(*mac, out.mac);
    else
        check_conn();    

    return out.retval;
}

int mvrp_proxy_is_enabled(void)
{
    int out;

    if (!proxy_create())
        return -1;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT, &mvrp_is_enabled_struct, &out) != 0)
        check_conn();    
    return out;
}

int mvrp_proxy_is_enabled_port(int port_no)
{
    int out;

    if (!proxy_create())
        return -1;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT, 
        &mvrp_is_enabled_port_struct, &out, port_no) != 0)
        check_conn();    
    return out;
}

int mvrp_proxy_register_vlan(int vid, uint32_t egress_ports, uint32_t forbidden_ports)
{
    struct mvrp_register_vlan_argdata in;
    int out;

    if (!proxy_create())
        return -1;

    in.vid = vid;
    in.egress_ports = egress_ports;
    in.forbidden_ports = forbidden_ports;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT, 
        &mvrp_register_vlan_struct, &out, &in) != 0)
        check_conn();    
    return out;
}

int mvrp_proxy_deregister_vlan(int vid)
{
    int out;

    if (!proxy_create())
        return -1;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT, 
        &mvrp_deregister_vlan_struct, &out, vid) != 0)
        check_conn();    
    return out;    
}

void mvrp_proxy_init(char *_name)
{
    strncpy(name, _name, sizeof(name) - 1);
}
