/*
 * Timestamping routines for WR Switch
 *
 * Copyright (C) 2010 CERN (www.cern.ch)
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

static int record_tstamp(struct wrn_dev *wrn, u32 ts, u8 port_id, u16 frame_id)
{
	int i; /* FIXME: use list for faster access */
	pr_debug("%s: Got TS: %x pid %d fid %d\n", __func__,
		 ts, port_id, frame_id);

	for(i=0;i<WRN_TS_BUF_SIZE;i++)
		if(!wrn->ts_buf[i].valid) {
			wrn->ts_buf[i].ts = ts;
			wrn->ts_buf[i].port_id = port_id;
			wrn->ts_buf[i].frame_id = frame_id;
			wrn->ts_buf[i].valid = 1;
			return 0;
		}

	/* no space in TS buffer? */
	return -ENOMEM;
}

void wrn_tstamp_init(struct wrn_dev *wrn)
{
	memset(wrn->ts_buf, 0, sizeof(wrn->ts_buf));
	/* enable TXTSU irq */
	writel(TXTSU_EIC_IER_NEMPTY, &wrn->txtsu_regs->EIC_IER);
}

irqreturn_t wrn_tstamp_interrupt(int irq, void *dev_id)
{
	struct wrn_dev *wrn = dev_id;
	struct TXTSU_WB *regs = wrn->txtsu_regs;

	/* FIXME: locking */
	u32 r0, r1;

	r0 = readl(&regs->TSF_R0);
	r1 = readl(&regs->TSF_R1);

	if(record_tstamp(wrn, r0,
			 TXTSU_TSF_R1_PID_R(r1),
			 TXTSU_TSF_R1_FID_R(r1)) < 0) {
		printk("%s: ENOMEM in the TS buffer. Disabling TX stamping.\n",
		       __func__);
		writel(TXTSU_EIC_IER_NEMPTY, &wrn->txtsu_regs->EIC_IDR);
	}
	writel(TXTSU_EIC_IER_NEMPTY, &wrn->txtsu_regs->EIC_ISR); /* ack irq */
	return IRQ_HANDLED;
}

int wrn_tstamp_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct wrn_ep *ep = netdev_priv(dev);
	struct hwtstamp_config config;

	if (copy_from_user(&config, rq->ifr_data, sizeof(config)))
		return -EFAULT;
	if (0) netdev_dbg(dev, "%s: tx type %i, rx filter %i\n", __func__,
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
