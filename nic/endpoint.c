/*
 * Endoint-specific operations in the White-Rabbit switch network interface
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
#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/io.h>

#include "wr-nic.h"

/* Called for each endpoint, with a valid priv structure in place */
int wrn_endpoint_probe(struct net_device *netdev)
{
	struct wrn_devpriv *priv = netdev_priv(netdev);
	int err;

	/* FIXME: check if the endpoint does exist or not */

	if (0 /* not existent -- means no more exist after this one */)
		return -ENODEV;

	/* Errors different from -ENODEV are fatal to insmod */
	priv->base = ioremap(FPGA_BASE_EP(priv->ep_number), WRN_EP_MEM_SIZE);
	if (!priv->base) {
		printk(KERN_ERR DRV_NAME ": ioremap failed for EP %i\n",
		       priv->ep_number);
		return -ENOMEM;
	}

	/* build the device name (FIXME: up or downlink?) */
	dev_alloc_name(netdev, "wru%d");
	wrn_netops_init(netdev); /* function in ./nic-core.c */
	wrn_ethtool_init(netdev); /* function in ./ethtool.c */
	/* Napi is not supported on this device */

	/* FIXME: mii -- copy from minic */

	/* randomize a MAC address, so lazy users can avoid ifconfig */
	random_ether_addr(netdev->dev_addr);

	err = register_netdev(netdev);
	if (err) {
		printk(KERN_ERR DRV_NAME "Can't register dev %s\n",
		       netdev->name);
		iounmap(priv->base);
		/* ENODEV means "no more" for the caller */
		return err == -ENODEV ? -EIO : err;
	}
	return 0;
}

/* Called for each endpoint, with a valid priv structure. The caller frees */
void wrn_endpoint_remove(struct net_device *netdev)
{
	struct wrn_devpriv *priv = netdev_priv(netdev);

	unregister_netdev(netdev);
	iounmap(priv->base);
}
