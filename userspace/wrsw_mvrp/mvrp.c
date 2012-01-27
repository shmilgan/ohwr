#include <stdio.h>
#include <netinet/in.h>

#include <hw/trace.h>

#include "mrp.h"
#include "mrp_pdu.h"

/* Protocol Version as defined (802.1ak-2007) */
#define MVRP_PROTOCOL_VER   0x00

/* MVRP group MAC address */
#define MVRP_ADDRESS        { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x21 }

/* MVRP EtherType value */
#define ETH_P_MVRP          0x88F5

enum mvrp_attributes {
	MVRP_ATTR_INVALID,
	MVRP_ATTR_VID,
	MVRP_ATTR_MAX
};

void mvrp_join_ind(int type, int len, void *value, int is_new)
{
    fprintf(stderr, "received join indication\n");
}

void mvrp_leave_ind(int type, int len, void *value)
{
    fprintf(stderr, "received leave indication\n");
}

/* Returns the next value for the indicated attribute type */
void nextval(int type, int len, void *value)
{
    uint16_t vid;

    vid = ntohs(*((uint16_t*)value));
    *((uint16_t*)value) = htons(vid + 1);
}

static struct mrp_application mvrp_app = {
    .maxattr         = MVRP_ATTR_MAX,
    .mad_join_ind    = mvrp_join_ind,
    .mad_leave_ind   = mvrp_leave_ind,
    .nextval         = nextval,
	.proto.version   = MVRP_PROTOCOL_VER,
    .proto.ethertype = ETH_P_MVRP,
    .proto.address   = MVRP_ADDRESS
};

int main(int argc, char **argv) {

    fprintf(stderr, "mvrp: mrp init\n");

    if (mrp_init() != 0)
        return -1;

    fprintf(stderr, "mvrp: register application\n");

    if (mrp_register_application(&mvrp_app) != 0)
        return -1;

    fprintf(stderr, "mvrp: mrp protocol\n");

    /* Multiple Registration Protocol */
    mrp_protocol(&mvrp_app);

    fprintf(stderr, "unregister application\n");

    if (mrp_unregister_application(&mvrp_app) != 0)
        return -1;

    return 0;
}
