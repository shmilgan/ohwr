/*
 * White Rabbit Switch RSTP
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizán (miguel.baizan@integrasys-sa.com)
 *
 *              This code is based on the code found at git://git.kernel.org/
 *              pub/scm/linux/kernel/git/shemminger/rstp.git, by
 *              Srinivas Aji <Aji_Srinivas@emc.com>
 *
 *
 * Description: Set raw sockets to send/receive only BPDUs. The socket is
 *              handled with the epoll facility.
 *
 * Fixes:
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

#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/filter.h>

#include "rstp_data.h"
#include "rstp_frame.h"
#include "rstp_epoll_loop.h"


/* Handler for epoll facility */
static struct epoll_event_handler frame_event;


/* Only for debug */
static void dump_frame(const unsigned char *buf, int octets)
{
    int i, j;
    for (i = 0; i < octets; i += 16) {
        for (j = 0; j < 16 && i + j < octets; j++)
            printf(" %02x", buf[i + j]);
        printf("\n");
    }
    printf("\n");
}


/*
 * To send/receive Spanning Tree BPDUs we use PF_PACKET because
 * it allows the filtering we want but gives raw data
 */
void frame_send(int ifindex, const unsigned char *data, int len)
{
    int octets;
    static struct sockaddr_ll sl = {   /* Link level information */
        .sll_family = AF_PACKET,
        .sll_halen = ETH_ALEN,
    }; /* TODO Pass this structure as argument */

    sl.sll_protocol = htons(ETH_P_802_2),
    sl.sll_ifindex = ifindex;

    /* The first bytes of data pointer should be the physical layer address */
    memcpy(&sl.sll_addr, data, ETH_ALEN);

    /* For debug */
    TRACEV(TRACE_INFO,
          "Transmit Dst index %d %02x:%02x:%02x:%02x:%02x:%02x\n",
          sl.sll_ifindex,
          sl.sll_addr[0], sl.sll_addr[1], sl.sll_addr[2],
          sl.sll_addr[3], sl.sll_addr[4], sl.sll_addr[5]);
    dump_frame(data, len);

    octets = sendto(frame_event.fd, data, len, 0,
               (struct sockaddr *) &sl, sizeof(sl));

    if (octets < 0) {
        TRACE(TRACE_ERROR, "frame send failed: %d", octets);
    } else if (octets != len) {
        TRACE(TRACE_ERROR, "short write in sendto: %d instead of %d",
              octets, len);
    }
}

/* Internal function to receive the BPDUs */
static void frame_recv(uint32_t events, struct epoll_event_handler *h)
{
    int octets;
    unsigned char buf[2048];
    struct sockaddr_ll sl;  /* Link level information */
    socklen_t salen = sizeof sl;

    /* We read one frame */
    octets =
        recvfrom(h->fd, &buf, sizeof(buf), 0, (struct sockaddr *) &sl, &salen);
    if (octets <= 0) {
        TRACE(TRACE_ERROR, "frame received failed: %d", octets);
        return;
    }

    /* For debug */
    TRACEV(TRACE_INFO,
          "Receive Src ifindex %d %02x:%02x:%02x:%02x:%02x:%02x",
          sl.sll_ifindex,
          sl.sll_addr[0], sl.sll_addr[1], sl.sll_addr[2],
          sl.sll_addr[3], sl.sll_addr[4], sl.sll_addr[5]);
    dump_frame(buf, octets);

    /* TODO Parse the received frame. Fill in port->bpdu with data. Then
       recompute state machines */
    //bpdu_rcv(sl.sll_ifindex, buf, octets);
}


/*
 * Open up a raw packet socket to catch all 802.2 PDUs
 * and install a packet filter to only see STP (DSAP 0x42)
 *
 * Berkeley Packet Filter (BPF) code to filter out spanning tree BPDUs has
 * been generated with the tcpdump program filtering stp protocol plus the -dd
 * option (tcpdump [-i interface] -dd stp). It basically compares first the
 * Ethertype value (in offset 12, value must be less than 1500), and then the
 * DSAP field (in offset 14, value must be equal to 0x42). See the filter.h for
 * instruction codes.
 */
int frame_socket_init(void)
{
    int sockfd;

    static struct sock_filter stp_filter[] = { /* BPF code */
        { 0x28, 0, 0, 0x0000000c },
        { 0x25, 3, 0, 0x000005dc },
        { 0x30, 0, 0, 0x0000000e },
        { 0x15, 0, 1, 0x00000042 },
        { 0x6, 0, 0, 0x00000060 },
        { 0x6, 0, 0, 0x00000000 },
    };

    struct sock_fprog fprog = {
        .len = sizeof(stp_filter) / sizeof(stp_filter[0]),
        .filter = stp_filter,
    };


    /* We need raw sockets to get link level information */
    sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_802_2));
    if (sockfd < 0) {
        TRACE(TRACE_FATAL, "frame socket failed: error %d: %s\n",
              errno, strerror(errno));
        return -1;
    }

    /* Attach the filter to relay only STP BPDUs */
    if (setsockopt(sockfd, SOL_SOCKET, SO_ATTACH_FILTER,
        &fprog, sizeof(fprog)) < 0) {
        TRACE(TRACE_FATAL, "setsockopt frame filter failed: error %d: %s\n",
              errno, strerror(errno));
        return -1;
    }

    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0) {
        /* We want non-block sockets */
        TRACE(TRACE_FATAL, "fcntl set nonblock failed: error %d: %s\n",
              errno, strerror(errno));
        return -1;
    }

    /* Register this socket and its handler in the epoll facility */
    frame_event.fd = sockfd;
    frame_event.handler = frame_recv;

    if (epoll_add(&frame_event) != 0) {
        TRACE(TRACE_FATAL, "adding to epoll failed: error %d: %s\n",
              errno, strerror(errno));
        return -1;
    }

    return 0;
}
