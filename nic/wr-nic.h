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

#include "nic-hardware.h" /* Magic numbers: please fix them as needed */

#define DRV_NAME "wr-nic" /* Used in messages and device/driver names */
#define DRV_VERSION "0.1" /* For ethtool->get_drvinfo -- FIXME: auto-vers */

/* Structures we are using but may remain opaque */
struct net_device;

/*
 * This is the main data structure for our NIC device
 */
#define WRN_NR_ENDPOINTS 24

struct wrn_dev {
	/* Base addresses for the various areas */
	void __iomem *base_nic; /* nic includes packet RAM */
	void __iomem *base_ppsg;
	void __iomem *base_calib;
	void __iomem *base_stamp;
	/* The base for each endpoint is ioremapped by the EP itself */

	spinlock_t lock;

	struct net_device *dev[WRN_NR_ENDPOINTS];

	/* FIXME: all dev fields must be verified */
	//int tx_hwtstamp_enable;
	//int rx_hwtstamp_enable;

	//int tx_hwtstamp_oob;

	//struct platform_device	*pdev;
	//struct device		*dev;
	//struct net_device	*netdev;

	//struct net_device_stats	stats;
	//struct napi_struct	napi;
	//struct sk_buff		*current_skb;
	//struct timer_list	link_timer;

	//unsigned int rx_head, rx_avail, rx_base, rx_size;
	//unsigned int tx_head, tx_avail, tx_base, tx_size;

	//bool synced;
	//bool syncing_counters;
	//int iface_up;

	//u32 cur_rx_desc;

	struct mii_if_info mii; /* for ethtool operations */

	int use_count; /* only used at probe time */
	int irq_registered;
};

/* Each network device has one such priv structure */
struct wrn_devpriv {
	struct wrn_dev *wrn;
	void __iomem *base; /* each EP has its own memory area */
	int ep_number;
	int flags; /* uplink/downlink and possibly more */
	/* FIXME: other fields needed for endpoint-specific struct? */
};
#define WRN_EP_MEM_SIZE 0x10000 /* 64k like other areas */

/* Our resources. The names are used as indexes in the resource array */
enum wrn_resnames {
	WRN_RES_MEM_NIC = 0,
	WRN_RES_MEM_PPSG,
	WRN_RES_MEM_CALIB,
	WRN_RES_MEM_STAMP,
	/* Irq is last, so platform_get_resource() can use previous enums */
	WRN_RES_IRQ,
};

/* Following functions in in nic-core.c */
extern irqreturn_t wrn_interrupt(int irq, void *dev_id);
extern int wrn_netops_init(struct net_device *netdev);

/* Following data in device.c */
struct platform_driver;
extern struct platform_driver wrn_driver;

/* Following functions in ethtool.c */
extern int wrn_ethtool_init(struct net_device *netdev);

/* Following functions in endpoint.c */
extern int  wrn_endpoint_probe(struct net_device *netdev);
extern void wrn_endpoint_remove(struct net_device *netdev);

#endif /* __WR_NIC_H__ */
