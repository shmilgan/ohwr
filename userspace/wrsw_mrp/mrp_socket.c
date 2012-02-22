/*
 * White Rabbit MRP (Multiple Registration Protocol)
 *
 * Authors:     Miguel Baizan   (miguel.baizan@integrasys.es)
 *              Juan Luis Manas (juan.manas@integrasys.es)
 *
 *
 * Description: Handling of Raw and Datagram packet sockets (Tx/Rx of PDUs).
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <unistd.h>
#include <fcntl.h>

#include <arpa/inet.h>

#include <linux/filter.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>

#include <net/if.h>
#include <netinet/in.h>

#include <sys/socket.h>
#include <sys/ioctl.h>

#include "mrp.h"

#ifndef VLAN_ETH_FRAME_LEN
#define VLAN_ETH_FRAME_LEN  1518    /* Max. octets in frame (without FCS) */
#endif

#ifndef VLAN_ETH_HLEN
#define VLAN_ETH_HLEN   18          /* Header length for a VLAN tagged frame */
#endif


/* If the MRP application works with VLAN tagged PDUs, we need SOCK_RAW. The
   socket receives all the tagged traffic and then filter those frames with
   the encapsulated Ethertype matching the Ethertype defined by the
   application.
   @param app contains MRP application configuration
   @return socket descriptor. -1 if error */
static int mrp_raw_socket(struct mrp_application *app)
{
    int fd;

    /* Berkeley Packet Filter (BPF) code to filter 1Q tagged MRPDUs. We compare
       some fields of the frame to identify these MRPDUs (i.e. TPID (0x8100) and
       the allowed encapsulated Ethertype). */
    struct sock_filter filter[] = {
        { 0x28, 0, 0, 0x0000000c },           // [0] LDH, copy halfword in
                                              //     offset 0x0c (12)
        { 0x15, 0, 3, ETH_P_8021Q },          // [1] JEQ, compare with 0x8100,
                                              //     TRUE: continue (0),
                                              //     FALSE: jump 3 (i.e. to [5])
        { 0x28, 0, 0, 0x00000010 },           // [2] LDH, copy halfword in
                                              //     offset 0x10 (16)
        { 0x15, 0, 1, app->proto.ethertype }, // [3] JEQ, compare with Ethertype,
                                              //     TRUE: continue (0), FALSE:
                                              //     jump 1 (i.e. to [5])
        { 0x06, 0, 0, VLAN_ETH_FRAME_LEN },   // [4] return bytes to accept
                                              //     if matched
        { 0x06, 0, 0, 0x00000000 },           // [5] return 0 if not matched
    };

    struct sock_fprog fprog = {
        .len = sizeof(filter) / sizeof(filter[0]),
        .filter = filter,
    };

    /* PF_PACKET:   Packet socket
       Socket type: SOCK_RAW (include the link level headers)
       protocol:    1Q tagged frames */
    fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_8021Q));
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    /* Attach the filter to relay only encapsulated MRPDUs */
    if (setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER,
        &fprog, sizeof(fprog)) < 0) {
        perror("socket filter");
        close(fd);
        return -1;
    }

    return fd;
}

/* If the MRP application works with untagged PDUs, we may use SOCK_DGRAM. The
   socket receives all those frames with an Ethertype matching the Ethertype
   defined by the application.
   @param app contains MRP application configuration
   @return socket descriptor. -1 if error */
static int mrp_dgram_socket(struct mrp_application *app)
{
    int fd;

    /* PF_PACKET:   Packet socket
       Socket type: SOCK_DGRAM (link level headers removed)
       protocol:    Ethertype */
    fd = socket(PF_PACKET, SOCK_DGRAM, htons(app->proto.ethertype));
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    return fd;
}

/* Send MRPDU through a SOCK_DGRAM socket */
static void mrp_send_dgram(struct sockaddr_ll *sl, struct mrp_participant *p)
{
    int len;
    struct mrpdu *pdu = &p->pdu;
    struct mrp_application *app = p->port->app;

    /* Complete the sockaddr_ll structure so the kernel can build the header
       from the sockaddr_ll structure */
    sl->sll_protocol = htons(app->proto.ethertype);
    memcpy(sl->sll_addr, app->proto.address, ETH_ALEN);

    /* Send the frame */
    len = sendto(app->proto.fd, pdu->buf, pdu->len, 0,
                 (struct sockaddr*)sl, sizeof(struct sockaddr_ll));

    if (len < pdu->len)
        fprintf(stderr, "mrp_send_dgram");
}

/* Send MRPDU through a SOCK_RAW socket */
static void mrp_send_raw(struct sockaddr_ll *sl, struct mrp_participant *p)
{
    uint8_t buffer[VLAN_ETH_FRAME_LEN]; /* Buffer to build the frame */
    int pos = 0;                        /* Buffer encoding position */
    int len;
    uint16_t tpid, tci;
    uint16_t vid;
    uint16_t type;
    struct ifreq ifr;
    struct mrpdu *pdu = &p->pdu;
    struct mrp_application *app = p->port->app;
    struct map_context *ctx;

    /* Get source address. First we need to get the name of the interface we
       want to send the frame to and then we can get its HW address.
       ('Normally, the user specifies which device to affect by setting
        ifr_name... and given the ifr_ifindex, the SIOCGIFNAME is the only ioctl
        which returns its results in ifr_name') */
    ifr.ifr_ifindex = sl->sll_ifindex;
    if (ioctl(app->proto.fd, SIOCGIFNAME, &ifr) == -1) {
		fprintf(stderr, "get interface name error");
        return;
	}
	if (ioctl(app->proto.fd, SIOCGIFHWADDR, &ifr) == -1) {
		fprintf(stderr, "get interface HW address error");
        return;
	}

    /* Get the VID */
    if (!p->contexts || p->contexts->next) {
        fprintf(stderr, "mrp: tagged protocol with no context" \
                        "or multiple contexts per participant");
        return;
    }

    ctx = (struct map_context*)p->contexts->content;
    vid = ctx->cid;

    /* Build the frame header */
    tpid = htons(ETH_P_8021Q);
    tci  = htons(vid);                  /* We consider PCP = 0 and CFI = 0 */
    type = htons(app->proto.ethertype); /* Encapsulated ethertype */

    memcpy(&buffer[pos], app->proto.address, ETH_ALEN);
    pos += ETH_ALEN;
    memcpy(&buffer[pos], ifr.ifr_hwaddr.sa_data, ETH_ALEN);
    pos += ETH_ALEN;
    memcpy(&buffer[pos], &tpid, sizeof(uint16_t));
    pos += sizeof(uint16_t);
    memcpy(&buffer[pos], &tci, sizeof(uint16_t));
    pos += sizeof(uint16_t);
    memcpy(&buffer[pos], &type, sizeof(uint16_t));
    pos += sizeof(uint16_t);

    memcpy(&buffer[pos], pdu->buf, pdu->len);
    pos += pdu->len;

    /* Send frame */
    len = sendto(app->proto.fd, buffer, pos, 0,
          (struct sockaddr*)&sl, sizeof(struct sockaddr_ll));

    if (len < pos)
        fprintf(stderr, "mrp_send_raw");
}


/* Receive MRPDU from SOCK_RAW */
static struct mrp_participant *mrp_rcv_raw(struct mrp_application *app,
                                           struct mrpdu *pdu)
{
    uint8_t buffer[VLAN_ETH_FRAME_LEN]; /* Buffer */
    struct mrp_participant *p;
    struct mrp_port *port;
    struct sockaddr_ll sl;              /* Link level info */
    socklen_t sl_len = sizeof(sl);
    uint16_t vid;
    int len;

    memset(buffer, 0, VLAN_ETH_FRAME_LEN);

    /* Receive frame (including link level headers) */
    len = recvfrom(app->proto.fd, &buffer, VLAN_ETH_FRAME_LEN, 0,
                   (struct sockaddr*)&sl, &sl_len);
    if (len <= 0)
        return NULL;

    /* Find the VID in the header of the received frame */
    vid = buffer[(2 * ETH_ALEN) + 2] & 0x0fff;

    /* Extract the PDU from the frame */
    memcpy(pdu->buf, &buffer[VLAN_ETH_HLEN], sizeof(pdu->buf));
    pdu->len = len - VLAN_ETH_HLEN;
    memcpy(pdu->src_mac, sl.sll_addr, ETH_ALEN);

    port = mrp_find_port(app, sl.sll_ifindex);
    if (!port)
        return NULL;

    p = mrp_find_participant(port, mrp_find_context(app, vid));
    return p;
}

/* Receive MRPDU from SOCK_DGRAM */
static struct mrp_participant *mrp_rcv_dgram(struct mrp_application *app,
                                             struct mrpdu *pdu)
{
    struct sockaddr_ll sl;              /* Link level info */
    socklen_t len = sizeof(sl);
    struct mrp_port *port;

    /* Receive PDU */
    pdu->len = recvfrom(app->proto.fd, pdu->buf, sizeof(pdu->buf), 0,
        (struct sockaddr*)&sl, &len);
    if (pdu->len <= 0)
        return NULL;

    fprintf(stderr, "A %d bytes length frame has been received\n", pdu->len);

    memcpy(pdu->src_mac, sl.sll_addr, ETH_ALEN);

    /* Return the participant: only one participant can be attached to any given
    port for a vlan-unaware protocol (i.e. the first and only one in the list) */
    port = mrp_find_port(app, sl.sll_ifindex);
    if (!port)
        return NULL;

    return (struct mrp_participant*)port->participants->content;
}

/* Send MRPDU through the open socket. If we have SOCK_DGRAM, we let the kernel
   append the link level header. If we have SOCK_RAW we have to build the
   header from scratch */
void mrp_socket_send(struct mrp_participant *p)
{
    /* The sockaddr_ll structure is needed even for Raw Sockets, since they've
       not been bound to any interface */
    static struct sockaddr_ll sl = { /* Link level info */
        .sll_family = PF_PACKET,
        .sll_halen  = ETH_ALEN
    };

    sl.sll_ifindex = p->port->hw_index;

    p->port->app->proto.tagged ? mrp_send_raw(&sl, p) : mrp_send_dgram(&sl, p);
}

/* Receive an MRPDU. The function locates the participant that should process
   the PDU and fills its PDU buffer with the payload of the frame.
   @param app contains MRP application configuration (ether_type, address)
   @param pdu pointer to the PDU structure which buffer will be filled
   @return pointer to the participant that should proccess the PDU. NULL if no
   such participant is found. */
struct mrp_participant *mrp_socket_rcv(struct mrp_application *app,
                                       struct mrpdu *pdu)
{
    struct mrp_participant *p = NULL;

    p = app->proto.tagged ?
        mrp_rcv_raw(app, pdu) : mrp_rcv_dgram(app, pdu);

    return p;
}

/* Open packet socket to send and receive MRP PDUs for a given application
   @param app contains MRP application configuration (ether_type, address)
   @return -1 if error. 0 otherwise. */
int mrp_open_socket(struct mrp_application *app)
{
    int fd;

    /* If the application works with VLAN tagged frames, we need SOCK_RAW;
       SOCK_DGRAM otherwise. */
    fd = app->proto.tagged ? mrp_raw_socket(app) : mrp_dgram_socket(app);

    if (fd < 0)
        return -1;

    /* O_NONBLOCK: Non-blocking socket */
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        perror("non-blocking socket");
        close(fd);
        return -1;
    }

    app->proto.fd = fd;
    return 0;
}
