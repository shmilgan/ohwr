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

#include "wr-nic.h"

/* These are the standard netword device operations */
static int wrn_open(struct net_device *netdev)
{
	struct wrn_devpriv *priv = netdev_priv(netdev);
	struct wrn_dev *wrn = priv->wrn;

	netdev_dbg(netdev, "%s\n", __func__);

	if (!is_valid_ether_addr(netdev->dev_addr))
		return -EADDRNOTAVAIL;

#if 0 /* FIXME: the whole open method is missing */
	minic_writel(nic, MINIC_REG_MCR, 0);
	minic_disable_irq(nic, 0xffffffff);
	ep_enable(netdev);

	netif_carrier_off(netdev);

	init_timer(&nic->link_timer);
	nic->link_timer.data = (unsigned long)netdev;
	nic->link_timer.function = minic_check_link;
	mod_timer(&nic->link_timer, jiffies + LINK_POLL_INTERVAL);

	nic->synced = false;
	nic->syncing_counters = false;

	nic->rx_base = MINIC_PBUF_SIZE >> 1;
	nic->rx_size = MINIC_PBUF_SIZE >> 1;

	nic->tx_base = 0;
	nic->tx_size = MINIC_PBUF_SIZE >> 1;

	nic->tx_hwtstamp_enable = 0;
	nic->rx_hwtstamp_enable = 0;
	nic->tx_hwtstamp_oob = 1;

	minic_new_rx_buffer(nic);

	minic_enable_irq(nic, MINIC_EIC_IER_RX); // enable RX irq
	minic_writel(nic, MINIC_REG_MCR, MINIC_MCR_RX_EN); // enable RX

	if (netif_queue_stopped(nic->netdev)) {
		netif_wake_queue(netdev);
	} else {
		netif_start_queue(netdev);
	}

	nic->iface_up = 0;
#endif
	return 0;
}

static int wrn_close(struct net_device *netdev)
{
	/* FIXME: the close method is missing */
	return 0;
}

static int wrn_set_mac_address(struct net_device *netdev, void* addr)
{
	/* FIXME: set_mac_address is missing */
	return 0;
}

static int wrn_start_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	/* FIXME: start_xmit is missing */
	return -ENODEV;
}

struct net_device_stats *wrn_get_stats(struct net_device *netdev)
{
	/* FIXME: getstats is missing */
	return NULL;
}

static int wrn_ioctl(struct net_device *netdev, struct ifreq *rq, int cmd)
{
	/* FIXME: ioctl is missing */
	return -ENOIOCTLCMD;
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
	/* Missing ops, possibly to add later (FIXME?) */
	.ndo_set_multicast_list	= wrn_set_multicast_list,
	.ndo_change_mtu		= wrn_change_mtu,
	/* There are several more, but not really useful for us */
#endif
};


int wrn_netops_init(struct net_device *netdev)
{
	netdev->netdev_ops = &wrn_netdev_ops;
	return 0;
}

irqreturn_t wrn_interrupt(int irq, void *dev_id)
{
	/* FIXME */
	return IRQ_HANDLED;
}
