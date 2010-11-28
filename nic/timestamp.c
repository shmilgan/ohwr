/*
 * Timestamping routines for WR Switch
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
#include <linux/sockios.h>
#include <linux/net_tstamp.h>

#include "wr-nic.h"

int wrn_tstamp_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct wrn_ep *ep = netdev_priv(dev);
	struct hwtstamp_config config;

	if (copy_from_user(&config, rq->ifr_data, sizeof(config)))
		return -EFAULT;
	netdev_dbg(dev, "%s: tx type %i, rx filter %i\n", __func__,
		   config.tx_type, config.rx_filter);

	switch (config.tx_type) {
	case HWTSTAMP_TX_ON:
		set_bit(WRN_EP_STAMPING_TX, &ep->ep_flags);
		/* FIXME memset(nic->tx_tstable, 0, sizeof(nic->tx_tstable));*/
		break;

	case HWTSTAMP_TX_OFF:
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
		clear_bit(WRN_EP_STAMPING_RX, &ep->ep_flags);
		break;

	case HWTSTAMP_FILTER_PTP_V2_L2_EVENT:
		set_bit(WRN_EP_STAMPING_RX, &ep->ep_flags);

		config.rx_filter = HWTSTAMP_FILTER_ALL;
		break;
	default:
		return -ERANGE;
	}

	/* FIXME: minic_update_ts_config(nic); */

	if (copy_to_user(rq->ifr_data, &config, sizeof(config)))
		return -EFAULT;
	return 0;
}
