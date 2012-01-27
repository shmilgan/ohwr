#ifndef __WHITERABBIT_MRP_PDU_H
#define __WHITERABBIT_MRP_PDU_H

#include "mrp.h"

void mrp_pdu_init(struct mrpdu *pdu);
int mrp_pdu_rcv(struct mrp_application *app);
void mrp_pdu_send(struct mrp_participant *part);
int mrp_pdu_full(struct mrpdu *pdu);

int mrp_pdu_append_attr(struct mrp_participant *part,
                        struct mrp_attr *attr,
                        enum mrp_event event);
int mrp_pdu_append_endmark(struct mrpdu *pdu);
#endif /*__WHITERABBIT_MRP_PDU_H*/
