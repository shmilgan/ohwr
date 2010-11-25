/*
 * wr-nic definitions, structures and prototypes
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
#ifndef __WR_NIC_H__
#define __WR_NIC_H__
#include <linux/mii.h>
#include <linux/irqreturn.h>
#include <linux/spinlock.h>
#include <linux/netdevice.h>
#include <linux/timer.h>

#include "nic-hardware.h" /* Magic numbers: please fix them as needed */

#define DRV_NAME "wr-nic" /* Used in messages and device/driver names */
#define DRV_VERSION "0.1" /* For ethtool->get_drvinfo -- FIXME: auto-vers */

/* Structures we are using but may remain opaque */
struct net_device;

/*
 * This is the main data structure for our NIC device
 */
struct wrn_dev {
	/* Base addresses. It's easier with an array, but not all are used */
	void __iomem		*bases[WRN_NBLOCKS];
	struct NIC_WB __iomem	*regs; /* shorthand for NIC-block registers */
	struct wrn_d_tx __iomem	*txd;
	unsigned long		tx_mask; /* descriptors in use */
	struct wrn_d_rx __iomem	*rxd;
	unsigned long		rx_mask; /* descriptors in use */
	spinlock_t		lock;

	struct net_device	*dev[WRN_NR_ENDPOINTS];

	/* FIXME: all dev fields must be verified */

	//unsigned int rx_head, rx_avail, rx_base, rx_size;
	//unsigned int tx_head, tx_avail, tx_base, tx_size;


	//u32 cur_rx_desc;

	int use_count; /* only used at probe time */
	int irq_registered;
};

/* Each network device (endpoint) has one such priv structure */
struct wrn_ep {
	struct wrn_dev		*wrn;
	struct EP_WB __iomem	*ep_regs; /* each EP has its own memory */
	spinlock_t		lock;
	struct timer_list	ep_link_timer;
	volatile unsigned long	ep_flags;
	struct mii_if_info	mii; /* for ethtool operations */
	int			ep_number;
	int			pkt_count; /* used for tx stamping ID */

	struct net_device_stats	stats;
	//struct sk_buff		*current_skb;

	//bool synced;
	//bool syncing_counters;

	//u32 cur_rx_desc;
};
#define WRN_LINK_POLL_INTERVAL (HZ/5)

enum ep_flags { /* only used in the ep_flags register */
	WRN_EP_UP		= 0,
	WRN_EP_IS_UPLINK	= 1,
	WRN_EP_STAMPING_TX	= 2,
	WRN_EP_STAMPING_RX	= 3,
};

/* Our resources. */
enum wrn_resnames {
	/*
	 * The names are used as indexes in the resource array. Note that
	 * they are unrelated with the memory addresses: we can't have
	 * holes in the memory list, so these are _different_ values
	 */
	WRN_RES_MEM_EP_UP0,
	WRN_RES_MEM_EP_UP1,
	WRN_RES_MEM_EP_DP0,
	WRN_RES_MEM_EP_DP1,
	WRN_RES_MEM_EP_DP2,
	WRN_RES_MEM_EP_DP3,
	WRN_RES_MEM_EP_DP4,
	WRN_RES_MEM_EP_DP5,
	WRN_RES_MEM_EP_DP6,
	WRN_RES_MEM_EP_DP7,
	WRN_RES_MEM_PPSG,
	WRN_RES_MEM_CALIBRATOR,
	WRN_RES_MEM_NIC,
	WRN_RES_MEM_TSTAMP,
	/* Irq is last, so platform_get_resource() can use previous enums */
	WRN_RES_IRQ,
};

/*
 * Register access may be needed outside of specific files.
 * Please note that this takes a register *name*, uppercase with no prefix.
 */
#define wrn_ep_read(ep, reg) __raw_readl(&(ep)->ep_regs->reg)
#define wrn_ep_write(ep, reg, val) __raw_writel((val), &(ep)->ep_regs->reg)



/* Following functions in in nic-core.c */
extern irqreturn_t wrn_interrupt(int irq, void *dev_id);
extern int wrn_netops_init(struct net_device *netdev);

/* Following data in device.c */
struct platform_driver;
extern struct platform_driver wrn_driver;

/* Following functions in ethtool.c */
extern int wrn_ethtool_init(struct net_device *netdev);

/* Following functions in endpoint.c */
extern int wrn_phy_read(struct net_device *dev, int phy_id, int location);
extern void wrn_phy_write(struct net_device *dev, int phy_id, int loc, int v);

extern int wrn_ep_open(struct net_device *dev);
extern int wrn_ep_close(struct net_device *dev);

extern int  wrn_endpoint_probe(struct net_device *netdev);
extern void wrn_endpoint_remove(struct net_device *netdev);

#endif /* __WR_NIC_H__ */
