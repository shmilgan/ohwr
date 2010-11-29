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
 * so sometimes a call to something within ./endpoint.c is performed.
 */
static int wrn_open(struct net_device *dev)
{
	struct wrn_ep *ep = netdev_priv(dev);

	/* This is "open" just for an endpoint. The nic hw is already on */
	netdev_dbg(dev, "%s\n", __func__);

	if (!is_valid_ether_addr(dev->dev_addr))
		return -EADDRNOTAVAIL;

	/* Mark it as down, and start the ep-specific polling timer */
	clear_bit(WRN_EP_UP, &ep->ep_flags);
	wrn_ep_open(dev);

	/* Software-only management is in this file*/
	if (netif_queue_stopped(dev)) {
		netif_wake_queue(dev);
	} else {
		netif_start_queue(dev);
	}

	/* Most drivers call platform_set_drvdata() but we don't need it */
	return 0;
}

static int wrn_close(struct net_device *dev)
{
	struct wrn_ep *ep = netdev_priv(dev);
	int ret;

	if ( (ret = wrn_ep_close(dev)) )
		return ret;

	/* FIXME: software-only fixing at close time */
	netif_stop_queue(dev);
	netif_carrier_off(dev);
	clear_bit(WRN_EP_UP, &ep->ep_flags);
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

/* This is called with the lock taken */
static int __wrn_alloc_tx_desc(struct wrn_dev *wrn)
{
	int ret, i = 0;

	do {
		/* First increment the position */
		wrn->next_txdesc++;
		if (unlikely(wrn->next_txdesc >= WRN_NR_TXDESC))
			wrn->next_txdesc = 0;
		/* then check if it's available */
		ret = test_and_set_bit(wrn->next_txdesc, &wrn->tx_mask);
		if (!ret)
			return wrn->next_txdesc;
	} while (++i < WRN_NR_TXDESC);
	return -ENOMEM;
}

static void __wrn_tx_desc(struct wrn_dev *wrn, int desc,
			  void *data, int len)
{
	/* FIXME: tx desc */
}


static int wrn_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct wrn_ep *ep = netdev_priv(dev);
	struct wrn_dev *wrn = ep->wrn;
	union skb_shared_tx *shtx = skb_tx(skb);
	unsigned long flags;
	int desc;
	u16 tx_oob = 0;
	void *data;
	unsigned len;

	if (unlikely(skb->len > WRN_MTU)) {
		/* FIXME: check this WRN_MTU is needed and used properly */
		ep->stats.tx_errors++;
		return -EMSGSIZE;
	}

	/* Allocate a descriptor (start from last allocated) */
	spin_lock_irqsave(&wrn->lock, flags);
	desc = __wrn_alloc_tx_desc(wrn);
	spin_unlock_irqrestore(&wrn->lock, flags);

	data = skb->data;
	len = skb->len;

	spin_lock_irqsave(&ep->lock, flags);

	wrn->skb_desc[desc] = skb; /* Save for tx irq and stamping */
	netif_stop_queue(dev); /* Queue is stopped until tx is over */

	if(test_bit(WRN_EP_STAMPING_TX, &ep->ep_flags)) {
		/* FIXME: the hw tx stamping */
#if 0
		struct skb_shared_hwtstamps *hwts = skb_hwtstamps(skb);
		shtx->in_progress = 1;
		*(u16 *) hwts = tx_oob = nic->tx_hwtstamp_oob;

		nic->tx_hwtstamp_oob ++;
		if(nic->tx_hwtstamp_oob == 60000)
			nic->tx_hwtstamp_oob = 1;
	} else {
		tx_oob = 0;
#endif
	}

	/* This both copies the data to the descriptr and fires tx */
	__wrn_tx_desc(wrn, desc, data, len);

	/* We are done, this is trivial maiintainance*/
	ep->stats.tx_packets++;
	ep->stats.tx_bytes += len;
	dev->trans_start = jiffies;

	spin_unlock_irqrestore(&ep->lock, flags);
	return 0;
}

struct net_device_stats *wrn_get_stats(struct net_device *dev)
{
	struct wrn_ep *ep = netdev_priv(dev);

	/* FIXME: we should get the RMON information from endpoint */
	return &ep->stats;
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
