#ifndef __WHITERABBIT_MVRP_PROXY_H
#define __WHITERABBIT_MVRP_PROXY_H

#include "minipc.h"

#define MILLISEC_TIMEOUT 1000

void mvrp_proxy_enable(void);
void mvrp_proxy_disable(void);
int mvrp_proxy_is_enabled(void);
int mvrp_proxy_enable_port(int port_no);
int mvrp_proxy_disable_port(int port_no);
int mvrp_proxy_is_enabled_port(int port_no);
int mvrp_proxy_get_failed_registrations(int port_no);
int mvrp_proxy_get_last_pdu_origin(int port_no, uint8_t (*mac)[ETH_ALEN]);

struct minipc_ch *mvrp_proxy_create(char *name);

#endif /*__WHITERABBIT_MVRP_PROXY_H*/
