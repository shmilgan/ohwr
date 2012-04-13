#include <stdlib.h>

#include "mac.h"

#include "mvrp_proxy.h"
#include "mvrp_structs.h"

struct minipc_ch *client;

void mvrp_proxy_enable(void)
{
    int out;

    minipc_call(client, MILLISEC_TIMEOUT, &mvrp_enable_struct, &out);
}

void mvrp_proxy_disable(void)
{
    int out;

    minipc_call(client, MILLISEC_TIMEOUT, &mvrp_disable_struct, &out);
}

int mvrp_proxy_enable_port(int port_no)
{
    int out;

    minipc_call(client, MILLISEC_TIMEOUT, &mvrp_enable_port_struct, &out, port_no);
    return out;
}

int mvrp_proxy_disable_port(int port_no)
{
    int out;

    minipc_call(client, MILLISEC_TIMEOUT, &mvrp_disable_port_struct, &out, port_no);
    return out;
}

int mvrp_proxy_get_failed_registrations(int port_no)
{
    int out;

    minipc_call(client, MILLISEC_TIMEOUT,
        &mvrp_get_failed_registrations_struct, &out, port_no);
    return out;
}

int mvrp_proxy_get_last_pdu_origin(int port_no, uint8_t (*mac)[ETH_ALEN])
{
    struct mvrp_get_last_pdu_origin_retdata out;

    if (minipc_call(client, MILLISEC_TIMEOUT,
        &mvrp_get_last_pdu_origin_struct, &out, port_no) == 0)
        mac_copy(*mac, out.mac);

    return out.retval;
}

int mvrp_proxy_is_enabled(void)
{
    int out;

    minipc_call(client, MILLISEC_TIMEOUT, &mvrp_is_enabled_struct, &out);
    return out;
}

int mvrp_proxy_is_enabled_port(int port_no)
{
    int out;

    minipc_call(client, MILLISEC_TIMEOUT, &mvrp_is_enabled_port_struct, &out, port_no);
    return out;
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
