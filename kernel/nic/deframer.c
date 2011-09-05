/*
 * RX Deframer Control Register procedures
 *
 * Copyright (C) 2010 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>,
 *         Miguel Baizan <miguel.baizan@integrasys-sa.com>
 *         Juan Luis Manas <juan.manas@integrasys-sa.com>
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

int wrn_get_deframer_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
    struct wrn_register_req deframer_req;
    struct wrn_ep *ep = netdev_priv(dev);
    u32 rfcr;

    if (copy_from_user(&deframer_req, rq->ifr_data, sizeof(deframer_req)))
        return -EFAULT;

    rfcr = readl(&ep->ep_regs->RFCR);

    switch(deframer_req.cmd) {
    case WRN_RFCR_GET_PRIO_VAL:
        deframer_req.val = EP_RFCR_PRIO_VAL_R(rfcr);
        break;
    case WRN_RFCR_GET_QMODE:
        deframer_req.val = EP_RFCR_QMODE_R(rfcr);
        break;
    case WRN_RFCR_GET_VID_VAL:
        deframer_req.val = EP_RFCR_VID_VAL_R(rfcr);
        break;
    default:
        // do nothing.....
        return -ENOIOCTLCMD;
    }

    if (copy_to_user(rq->ifr_data, &deframer_req, sizeof(deframer_req)))
        return -EFAULT;

    return 0;
}


int wrn_set_deframer_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
    struct wrn_register_req deframer_req;
    struct wrn_ep *ep = netdev_priv(dev);
    u32 rfcr;


    if (copy_from_user(&deframer_req, rq->ifr_data, sizeof(deframer_req)))
        return -EFAULT;

    rfcr = readl(&ep->ep_regs->RFCR);

    switch(deframer_req.cmd) {
    case WRN_RFCR_SET_PRIO_VAL:
        if (deframer_req.val >= 8)
            return -EINVAL;
        rfcr &= (~EP_RFCR_PRIO_VAL_MASK);
        writel((rfcr | EP_RFCR_PRIO_VAL_W(deframer_req.val)),
            &ep->ep_regs->RFCR);
        break;
    case WRN_RFCR_SET_VID_VAL:
        if ((deframer_req.val == 0x000) ||
            (deframer_req.val == 0xFFF))
            return -EINVAL;
        rfcr &= (~EP_RFCR_VID_VAL_MASK);
        writel((rfcr | EP_RFCR_VID_VAL_W(deframer_req.val)),
            &ep->ep_regs->RFCR);
        break;
    case WRN_RFCR_SET_QMODE:
        rfcr &= (~EP_RFCR_QMODE_MASK);
        writel((rfcr | EP_RFCR_QMODE_W(deframer_req.val & 0x11)),
            &ep->ep_regs->RFCR);
        break;
    default:
        // do nothing.....
        return -ENOIOCTLCMD;
    }

    return 0;
}
