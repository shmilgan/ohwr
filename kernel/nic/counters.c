/*
 * Event counters memory
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

int wrn_event_counters_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
    struct wrn_register_req counter_req;
    struct wrn_ep *ep = netdev_priv(dev);
    u32 offset;
    u32 counter;

    if (copy_from_user(&counter_req, rq->ifr_data, sizeof(counter_req)))
        return -EFAULT;

    offset = counter_req.val;

    /* Test that an invalid offset (i.e. non existing counter) is not being
       requested */
    if (offset < 0 || offset > 10)
        return -EINVAL;

    counter = readl(&ep->ep_regs->RMON_RAM[offset]);
    counter_req.val = counter;

    if (copy_to_user(rq->ifr_data, &counter_req, sizeof(counter_req)))
        return -EFAULT;

    return 0;
}
