/*
 * Core file for White-Rabbit switch network interface
 *
 * Copyright (C) 2010 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>
 * Parts are from previous work by Tomasz Wlostowski <tomasz.wlostowsk@cern.ch>
 * Parts are from previous work by  Emilio G. Cota <cota@braap.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <asm/unaligned.h>

#include "wr-nic.h"

/*
 * The following functions are the standard network device operations.
 * They act on the _endpoint_ (as each Linux interface is one endpoint)
 * so sometimes a call to something withing ./endpoint.c is performed.
 */
static int wrn_open(struct net_device *dev)
{
	struct wrn_ep *ep = netdev_priv(dev);

	/* This is "open" just for an endpoint. The nic hw is already on */
	netdev_dbg(dev, "%s\n", __func__);

	if (!is_valid_ether_addr(dev->dev_addr))
		return -EADDRNOTAVAIL;

	/* Acknowledge all possibly pending interrupt sources */

	/* FIXME */

	/* Enable tx and rx, without timestamping at this point */


	/* Most drivers call platform_set_drvdata() but we don't need it */

	if (netif_queue_stopped(dev)) {
		netif_wake_queue(dev);
	} else {
		netif_start_queue(dev);
	}

	/* Mark it as down, and start the ep-specific polling timer */
	clear_bit(WRN_EP_UP, &ep->ep_flags);
	wrn_ep_open(dev);

	return 0;
}

static int wrn_close(struct net_device *dev)
{
	struct wrn_ep *ep = netdev_priv(dev);

	wrn_ep_close(dev);
	clear_bit(WRN_EP_UP, &ep->ep_flags);

	/* FIXME: other things to cleanup on close? */
	return 0;
}

static int wrn_set_mac_address(struct net_device *dev, void* vaddr)
{
	struct wrn_ep *ep = netdev_priv(dev);
	struct sockaddr *addr = vaddr;
	u32 val;

	netdev_dbg(dev, "%s\n", __func__);

	if (!is_valid_ether_addr(addr->sa_data)) {
		netdev_dbg(dev, "%s: invalid\n", __func__);
		return -EADDRNOTAVAIL;
	}

	memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);

	val = __get_unaligned_le((u32 *)dev->dev_addr+0);
	writel(val, ep->ep_regs->MACL);
	val = __get_unaligned_le((u16 *)dev->dev_addr+4);
	writel(val, ep->ep_regs->MACH);
	return 0;
}

static int wrn_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	/* FIXME: start_xmit is missing */
	return -ENODEV;
}

struct net_device_stats *wrn_get_stats(struct net_device *dev)
{
	/* FIXME: getstats is missing */
	return NULL;
}

static int wrn_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct wrn_ep *ep = netdev_priv(dev);
	int res;

	switch (cmd) {
	case SIOCSHWTSTAMP:
		return wrn_tstamp_ioctl(dev, rq, cmd);
	case PRIV_IOCGCALIBRATE:
		return wrn_calib_ioctl(dev, rq, cmd);
	case PRIV_IOCGGETPHASE:
		return wrn_phase_ioctl(dev, rq, cmd);
	default:
		spin_lock_irq(&ep->lock);
		res = generic_mii_ioctl(&ep->mii, if_mii(rq), cmd, NULL);
		spin_unlock_irq(&ep->lock);
		return res;
	}
}

static const struct net_device_ops wrn_netdev_ops = {
	.ndo_open		= wrn_open,
	.ndo_stop		= wrn_close,
	.ndo_start_xmit		= wrn_start_xmit,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_get_stats		= wrn_get_stats,
	.ndo_set_mac_address	= wrn_set_mac_address,
	.ndo_do_ioctl		= wrn_ioctl,
#if 0
	/* Missing ops, possibly to add later */
	.ndo_set_multicast_list	= wrn_set_multicast_list,
	.ndo_change_mtu		= wrn_change_mtu,
	/* There are several more, but not really useful for us */
#endif
};


int wrn_netops_init(struct net_device *dev)
{
	dev->netdev_ops = &wrn_netdev_ops;
	return 0;
}

irqreturn_t wrn_interrupt(int irq, void *dev_id)
{
	/* FIXME -- interrupt */

	/* FIXME: check status register (BNA, REC) */
	return IRQ_HANDLED;
}
