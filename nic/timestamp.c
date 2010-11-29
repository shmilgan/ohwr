/*
 * Timestamping routines for WR Switch
 *
 * Copyright (C) 2010 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>
 * Partly from previous work by Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Partly from previous work by  Emilio G. Cota <cota@braap.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/sockios.h>
#include <linux/net_tstamp.h>

#include "wr-nic.h"

irqreturn_t wrn_tstamp_interrupt(int irq, void *dev_id)
{
	/* FIXME: the tstamp interrupt */
	return IRQ_NONE;
}

int wrn_tstamp_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct wrn_ep *ep = netdev_priv(dev);
	struct hwtstamp_config config;

	if (copy_from_user(&config, rq->ifr_data, sizeof(config)))
		return -EFAULT;
	netdev_dbg(dev, "%s: tx type %i, rx filter %i\n", __func__,
		   config.tx_type, config.rx_filter);

	switch (config.tx_type) {
		/* Set up time stamping on transmission */
	case HWTSTAMP_TX_ON:
		set_bit(WRN_EP_STAMPING_TX, &ep->ep_flags);
		/* FIXME: enable timestamp on tx in hardware */
		break;

	case HWTSTAMP_TX_OFF:
		/* FIXME: disable timestamp on tx in hardware */
		clear_bit(WRN_EP_STAMPING_TX, &ep->ep_flags);
		break;

	default:
		return -ERANGE;
	}

	/*
	 * For the time being, make this really simple and stupid: either
	 * time-tag _all_ the incoming packets or none of them.
	 */
	switch (config.rx_filter) {
	case HWTSTAMP_FILTER_NONE:
		/* FIXME: disable rx in hardware */
		clear_bit(WRN_EP_STAMPING_RX, &ep->ep_flags);
		break;

	default: /* All other case: activate stamping */
		/* FIXME: enable rx in hardware */
		set_bit(WRN_EP_STAMPING_RX, &ep->ep_flags);

		break;
	}

	/* FIXME: minic_update_ts_config(nic); */

	if (copy_to_user(rq->ifr_data, &config, sizeof(config)))
		return -EFAULT;
	return 0;
}
