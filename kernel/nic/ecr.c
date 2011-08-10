/*
 * Endpoint Control Register procedures
 *
 * Copyright (C) 2010 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>, 
 *         Miguel Baizan <miguel.baizan@integrasys-sa.com>
 * Partly from previous work by Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Partly from previous work by  Emilio G. Cota <cota@braap.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/netdevice.h>

#include "wr-nic.h"

int wrn_get_ecr_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
    struct wrn_register_req ecr_req;
    struct wrn_ep *ep = netdev_priv(dev);
    u32 ecr;

    if (copy_from_user(&ecr_req, rq->ifr_data, sizeof(ecr_req)))
        return -EFAULT;

    ecr = readl(&ep->ep_regs->ECR);

    switch(ecr_req.cmd) {
    case WRN_ECR_GET_PORTID:
        ecr_req.val = EP_ECR_PORTID_R(ecr);
        break;
    default:
        // do nothing.....
        return -ENOIOCTLCMD;
    }

    if (copy_to_user(rq->ifr_data, &ecr_req, sizeof(ecr_req)))
        return -EFAULT;

    return 0;
}

