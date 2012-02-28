#ifndef __WHITERABBIT_MVRP_H
#define __WHITERABBIT_MVRP_H

#include <linux/if_ether.h>

void mvrp_enable(void);
void mvrp_disable(void);
int mvrp_is_enabled(void);
int mvrp_enable_port(int port_no);
int mvrp_disable_port(int port_no);
int mvrp_is_enabled_port(int port_no);
int mvrp_get_failed_registrations(int port_no);
int mvrp_get_last_pdu_origin(int port_no, uint8_t (*mac)[ETH_ALEN]);

#endif /*__WHITERABBIT_MVRP_H*/
