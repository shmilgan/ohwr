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
#include <linux/if_packet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <hw/trace.h>

#include "mrp.h"

/* VLAN header is not in standard Linux headers. We rename the structure to
avoid future conflicts in case VLAN support be compiled in the kernel. */
struct _vlan_ethhdr {
    struct ethhdr   h_ether;
    __be16          h_vlan_TCI;
    __be16          h_vlan_encapsulated_proto;
}__attribute__((packed));

/* Set the socket and (if needed) append a filter to get only the traffic we
 * are interested in.
 * @param filter Pointer to the BPF to filter the traffic. This filter
 * is optional, so it may be NULL.
 * @param proto_id Ethertype value in host byte order.
 * @return socket descriptor. -1 if error
*/
static int mrp_socket(struct sock_filter *filter, uint16_t proto_id)
{
    int fd;

    /* PF_PACKET:   Packet socket
       Socket type: SOCK_RAW (include the link level headers)
       protocol:    proto_id */
    fd = socket(PF_PACKET, SOCK_RAW, htons(proto_id));
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    if (filter) {
        struct sock_fprog fprog = {
            .len = sizeof(filter) / sizeof(filter[0]),
            .filter = filter,
        };

        /* Attach the filter */
        if (setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER,
            &fprog, sizeof(fprog)) < 0) {
            perror("socket filter");
            close(fd);
            return -1;
        }
    }

    return fd;
}

/**
 * Send an MRPDU.
 * @param p MRP participant that holds the information to build the frame.
*/
void mrp_socket_send(struct mrp_participant *p)
{
    uint8_t buffer[VLAN_ETH_FRAME_LEN]; /* Buffer to build the frame */
    struct mrpdu *pdu = &p->pdu;
    struct mrp_application *app = p->port->app;
    struct map_context *ctx;    /* Only for applications using tagged frames */
    struct ethhdr       hdr;
    struct _vlan_ethhdr vhdr;
    int len, hdr_len, frame_len;
    struct map_context_list *cnode;

    /* The sockaddr_ll structure is needed (even for Raw Sockets), since our
       socket has not been bound to any interface */
    static struct sockaddr_ll sl = { /* Link level info */
        .sll_family = PF_PACKET,
        .sll_halen  = ETH_ALEN
    };
    sl.sll_ifindex = p->port->hw_index;

    /* Base Ethernet header */
    memcpy(hdr.h_dest, app->proto.address, ETH_ALEN);
    memcpy(hdr.h_source, p->port->hwaddr, ETH_ALEN);

    if (app->proto.tagged) {
        hdr.h_proto = htons(ETH_P_8021Q);
        hdr_len = VLAN_ETH_HLEN;
        memcpy(&vhdr.h_ether, &hdr, ETH_HLEN);
        /* Get VID and TPID from associated context */
        if (list_empty(&p->contexts)) {
            fprintf(stderr,  "mrp: no context found");
            return;
        }
        cnode = list_first_entry(&p->contexts, struct map_context_list, node);
        ctx = cnode->context;
        vhdr.h_vlan_TCI = htons(ctx->cid);
        vhdr.h_vlan_encapsulated_proto = htons(app->proto.ethertype);
        memcpy(buffer, &vhdr, VLAN_ETH_HLEN);
    } else {
        hdr.h_proto = htons(app->proto.ethertype);
        hdr_len = ETH_HLEN;
        memcpy(buffer, &hdr, ETH_HLEN);
    }

    /* Fill the Data field of the frame */
    memcpy(&buffer[hdr_len], pdu->buf, pdu->len);

    /* Send the frame */
    frame_len = hdr_len + pdu->len;
    
    fprintf(stderr,  
        "mrp: transmit pdu (port %d)\n", p->port->port_no);
    
    len = sendto(app->proto.fd, &buffer, frame_len, 0,
                 (struct sockaddr*)&sl, sizeof(struct sockaddr_ll));

    if (len < frame_len)
        perror("mrp_socket_send");
}

/**
 * Receive an MRPDU. The function locates the participant that should process
 * the PDU and fills its PDU buffer with the payload of the frame.
 * @param app contains MRP application configuration (ether_type, address)
 * @param pdu pointer to the PDU structure which buffer will be filled
 * @return pointer to the participant that should proccess the PDU. NULL if no
 * such participant is found.
*/
struct mrp_participant *mrp_socket_rcv(struct mrp_application *app,
                                       struct mrpdu *pdu)
{
    unsigned char buffer[VLAN_ETH_FRAME_LEN];  /* Buffer */
    struct mrp_port *port;
    struct sockaddr_ll sl;                     /* Link level info */
    socklen_t sl_len = sizeof(sl);
    int len, hdr_len;
    struct _vlan_ethhdr vhdr;   /* Only for applications using tagged frames */
    uint16_t vid;               /* Only for applications using tagged frames */

    /* Receive frame (including link level headers!) */
    len = recvfrom(app->proto.fd, &buffer, VLAN_ETH_FRAME_LEN, 0,
                   (struct sockaddr*)&sl, &sl_len);
    if (len <= 0)
        return NULL;

    fprintf(stderr,  
        "mrp: pdu received (ifindex %d)\n", sl.sll_ifindex);

    /* Find the port that has received the PDU */
    port = mrp_find_port(app, sl.sll_ifindex);
    if (!port)
        return NULL;

    /* Store the Last PDU Origin parameter */
    memcpy(port->last_pdu_origin, sl.sll_addr, ETH_ALEN);

    /* Copy frame Data to the PDU buffer */
    hdr_len = (app->proto.tagged) ? VLAN_ETH_HLEN : ETH_HLEN;
    pdu->len = len - hdr_len;
    memcpy(pdu->buf, &buffer[hdr_len], pdu->len);

    /* Find the participant */
    if (app->proto.tagged) {
        memcpy((void *)&vhdr, buffer, VLAN_ETH_HLEN);
        vid = ntohs(vhdr.h_vlan_TCI) & 0x0fff;
        return mrp_find_participant(port, mrp_find_context(app, vid));
    }
    /* Only one participant can be attached to any given port for a vlan-unaware
       protocol */
    return list_first_entry(&port->participants,
        struct mrp_participant, port_participant);
}

/**
 * Open packet socket to send and receive MRP PDUs for a given application
 * @param app contains MRP application configuration (ether_type, address)
 * @return -1 if error. 0 otherwise.
*/
int mrp_open_socket(struct mrp_application *app)
{
    int fd;

    /* Berkeley Packet Filter (BPF) code to filter 1Q tagged MRPDUs. We check
       the TPID (0x8100) and Ethertype fields to identify these MRPDUs. */
    struct sock_filter proto_tag_filter[] = {
        { 0x28, 0, 0, 0x0000000c },           // [0] LDH, copy halfword in
                                              //     offset 0x0c (12)
        { 0x15, 0, 3, ETH_P_8021Q },          // [1] JEQ, compare with 0x8100,
                                              //     TRUE: continue (0),
                                              //     FALSE: jump 3 (i.e. to [5])
        { 0x28, 0, 0, 0x00000010 },           // [2] LDH, copy halfword in
                                              //     offset 0x10 (16)
        { 0x15, 0, 1, app->proto.ethertype }, // [3] JEQ, compare with Ethertype
                                              //     TRUE: continue (0), FALSE:
                                              //     jump 1 (i.e. to [5])
        { 0x06, 0, 0, VLAN_ETH_FRAME_LEN },   // [4] return bytes to accept
                                              //     if matched
        { 0x06, 0, 0, 0x00000000 },           // [5] return 0 if not matched
    };

    /* Some MRP applications work with VLAN tagged frames, so we need SOCK_RAW.
       However the socket must be configured in a different way depending on
       the expected format of the received MRPDUs (tagged or untagged) */
    fd = app->proto.tagged
        ? mrp_socket(proto_tag_filter, ETH_P_8021Q)
        : mrp_socket(NULL, app->proto.ethertype);

    if (fd < 0)
        return -1;

    /* O_NONBLOCK: Non-blocking socket */
    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        perror("mrp_open_socket: non-blocking socket");
        close(fd);
        return -1;
    }

    app->proto.fd = fd;
    return 0;
}

void mrp_close_socket(struct mrp_application *app)
{
    close(app->proto.fd);
}
