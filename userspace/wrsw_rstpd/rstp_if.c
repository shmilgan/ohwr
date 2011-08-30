/*
 * White Rabbit Switch RSTP
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizán (miguel.baizan@integrasys-sa.com)
 *
 * Description: Functions that use exported functions of other daemons and HW,
 *              to get/set some values related with the RSTP operation.
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

#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <unistd.h>

#include <wr_ipc.h>
#include <wrsw_hal.h>
#include <hal_exports.h>
#include <hw/trace.h>

#include "rstp_if.h"
#include "rstp_data.h"


/* Travel through the port list and fill in with non-rstp data (port name, port
 * link status, port number)
*/
int rstp_if_init_port_data(void)
{
    wripc_handle_t              hal_ipc;
    hexp_port_list_t            port_list;
    hexp_port_state_t           port_state;

    struct ifreq                ifr;
    struct wrn_register_req     req;
    int                         sockfd, i;

    /* Create socket interface */
    sockfd = socket(AF_PACKET, SOCK_RAW, 0);
    if (sockfd < 0) {
        TRACE(TRACE_INFO, "socket failed");
        return -1;
    }

    ifr.ifr_addr.sa_family = AF_PACKET;

    /* Connect to HAL */
    hal_ipc = wripc_connect("wrsw_hal");
    if (hal_ipc < 0) {
        TRACE(TRACE_INFO, "Unable to connect to HAL");
        close(sockfd);
        return -1;
    }

    /* Get port list */
    if (wripc_call(hal_ipc, "halexp_query_ports", &port_list, 0) < 0) {
        TRACE(TRACE_INFO, "ports query from HAL has failed");
        wripc_close(hal_ipc);
        close(sockfd);
        return -1;
    }

    /* Iterate through the port list */
    for (i = 0; i < MAX_NUM_PORTS ; i++) {
        /* Get port number */
        strncpy(ifr.ifr_name, port_list.port_names[i], sizeof(ifr.ifr_name));

        /* FIXME I'm not sure if the PORTID value (i.e port number) matches the
           hardware index, so to be sure I read it directly from the endpoint
           registers */
        req.cmd = WRN_ECR_GET_PORTID;
        ifr.ifr_data = &req;

        if (ioctl(sockfd, PRIV_IOCGGETECR, &ifr) < 0) {
            TRACE(TRACE_INFO, "ioctl PRIV_IOCGGETECR failed for port %s\n",
                  port_list.port_names[i]);
            wripc_close(hal_ipc);
            close(sockfd);
            return -1;
        }

        /* Get link status */
        if (wripc_call(hal_ipc, "halexp_get_port_state", &port_state, 1,
            A_STRING(port_list.port_names[i])) < 0) {
            TRACE(TRACE_INFO, "unable to get port state for port %s\n",
                  port_list.port_names[i]);
            wripc_close(hal_ipc);
            close(sockfd);
            return -1;
        }

        /* Fill port structure */
        strncpy(br.ports[i].port_name, port_list.port_names[i],
                sizeof(br.ports[i].port_name));
        if (req.val > 4096) { /* See 802.1D-2004, clause 9.2.7 */
            TRACE(TRACE_INFO, "Port numbers are only allowed to be up to 12 "
                  "bits long");
            wripc_close(hal_ipc);
            close(sockfd);
            return -1;
        }
        br.ports[i].port_number = req.val;
        br.ports[i].link_status = port_state.up;
    }

    wripc_close(hal_ipc);
    close(sockfd);
    return 0;
}


/* Function to get the numerically smallest MAC address of the ports, which will
 * be the default Bridge Address
*/
uint8_t* rstp_if_get_bridge_addr(void) {
    struct ifreq            ifr;
    char                    if_name[16];
    int                     sockfd, i;

    uint32_t                mac_high, mac_low;
    uint32_t                mac_high_prev = 0;
    uint32_t                mac_low_prev = 0;
    uint8_t                 minor_mac[ETH_ALEN];

    /* Create socket interface */
    sockfd = socket(AF_PACKET, SOCK_RAW, 0);
    if (sockfd < 0) {
        TRACE(TRACE_INFO, "socket failed");
        return memset(minor_mac, 0x00, ETH_ALEN);
    }

    ifr.ifr_addr.sa_family = AF_PACKET;

    /* Iterate port list */
    for (i = 0; i < MAX_NUM_PORTS ; i++) {
        /* Find the numerically smallest MAC address */
        strncpy(ifr.ifr_name, br.ports[i].port_name, sizeof(ifr.ifr_name));
        if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
            continue;
        } else {
            mac_high =  ifr.ifr_hwaddr.sa_data[0] << 24 |
                        ifr.ifr_hwaddr.sa_data[1] << 16 |
                        ifr.ifr_hwaddr.sa_data[2] << 8  |
                        ifr.ifr_hwaddr.sa_data[3];
            mac_low =   ifr.ifr_hwaddr.sa_data[4] << 8  |
                        ifr.ifr_hwaddr.sa_data[5];

            if ((mac_high_prev == 0) && (mac_low_prev == 0)) {
                mac_high_prev = mac_high;
                mac_low_prev = mac_low;
                strcpy(&if_name[0], br.ports[i].port_name);
            } else {
                if (mac_high == mac_high_prev) {
                    if (mac_low <= mac_low_prev) {
                        mac_low_prev = mac_low;
                        strcpy(&if_name[0], br.ports[i].port_name);
                    }
                } else {
                    if (mac_high <= mac_high_prev) {
                        mac_high_prev = mac_high;
                        strcpy(&if_name[0], br.ports[i].port_name);
                    }
                }
            }
        } /* Find the numerically smallest MAC address */
    }

    strncpy(ifr.ifr_name, &if_name[0], sizeof(ifr.ifr_name));

    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
        return memset(minor_mac, 0x00, ETH_ALEN);
    } else {
        return memcpy(minor_mac, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
    }
}
