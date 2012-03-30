/*
 * White Rabbit Switch Management
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_snmpd v1.0
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: Provides access to endpoint RFCR and ECR registers, to read and
 *              and write the VID_VAL, QMODE and PORTID fields.
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

#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/if.h>

#include <string.h>

#include "wr_ipc.h"
#include "wrsw_hal.h"
#include "hal_exports.h"

#include "endpoint_hw.h"

#define WRN_NR_ENDPOINTS 10

static hexp_port_list_t port_list;

static int ep_hw_ioctl(int cmd, struct wrn_register_req *req, int port_idx)
{
    int sockfd, err;
    struct ifreq ifr;

    if ((port_idx < 0) || (port_idx > WRN_NR_ENDPOINTS))
        return -EINVAL;

    sockfd = socket(AF_PACKET, SOCK_RAW, 0);
    if (sockfd < 0)
        return -EIO;

    // Fill ioctl request
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_addr.sa_family = AF_PACKET;
    strncpy(ifr.ifr_name,
            port_list.port_names[port_idx],
            sizeof(ifr.ifr_name));
    ifr.ifr_data = req;

    // IOCTL call
    err = ioctl(sockfd, cmd, &ifr);
    if (err < 0) {
        close(sockfd);
        return -EIO;
    }
    close(sockfd);
    return 0;
}

int ep_hw_init(void)
{
    wripc_handle_t  hal_ipc;

    /* Connect to HAL to get information of the ports */
    hal_ipc = wripc_connect("wrsw_hal");
    if (hal_ipc < 0)
        return -EIO;

    /* Get port list */
    if (wripc_call(hal_ipc, "halexp_query_ports", &port_list, 0) < 0) {
        wripc_close(hal_ipc);
        return -EIO;
    }

    wripc_close(hal_ipc);
    return 0;
}

int ep_hw_set_qmode(int port_idx, int qmode)
{
    struct wrn_register_req req;

    req.cmd = WRN_RFCR_SET_QMODE;
    req.val = qmode;
    if (ep_hw_ioctl(PRIV_IOCSSETRFCR, &req, port_idx) < 0)
        return -EIO;
    return 0;
}

int ep_hw_get_qmode(int port_idx)
{
    struct wrn_register_req req;

    req.cmd = WRN_RFCR_GET_QMODE;
    if (ep_hw_ioctl(PRIV_IOCGGETRFCR, &req, port_idx) < 0)
        return -EIO;
    return req.val;
}

int ep_hw_set_pvid(int port_idx, int pvid)
{
    struct wrn_register_req req;

    req.cmd = WRN_RFCR_SET_VID_VAL;
    req.val = pvid;
    if (ep_hw_ioctl(PRIV_IOCSSETRFCR, &req, port_idx) < 0)
        return -EIO;
    return 0;
}

int ep_hw_get_pvid(int port_idx)
{
    struct wrn_register_req req;

    req.cmd = WRN_RFCR_GET_VID_VAL;
    if (ep_hw_ioctl(PRIV_IOCGGETRFCR, &req, port_idx) < 0)
        return -EIO;
    return req.val;
}

int ep_hw_get_port_id(int port_idx)
{
    struct wrn_register_req req;

    req.cmd = WRN_ECR_GET_PORTID;
    if (ep_hw_ioctl(PRIV_IOCGGETECR, &req, port_idx) < 0)
        return -EIO;
    return req.val;
}
