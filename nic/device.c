/*
 * Device initialization and cleanup for White-Rabbit switch network interface
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
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/io.h>

#include "wr-nic.h"

/* The remove function is used by probe, so it's not __devexit */
static int __devexit wrn_remove(struct platform_device *pdev)
{
	struct wrn_dev *wrn = pdev->dev.platform_data;
	struct resource *res;
	int i;

	for (i = 0; i < WRN_NR_ENDPOINTS; i++) {
		if (wrn->dev[i]) {
			wrn_endpoint_remove(wrn->dev[i]);
			free_netdev(wrn->dev[i]);
			wrn->dev[i] = NULL;
		}
	}

	if (wrn->base_nic)	iounmap(wrn->base_nic);
	if (wrn->base_ppsg)	iounmap(wrn->base_ppsg);
	if (wrn->base_calib)	iounmap(wrn->base_calib);
	if (wrn->base_stamp)	iounmap(wrn->base_stamp);

	if (wrn->irq_registered) {
		res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
		free_irq(res->start, wrn);
	}
	return 0;
}

/* This helper is used by probe below, to avoid code replication */
static int __devinit __wrn_ioremap(struct platform_device *pdev, int n,
				    void __iomem **ptr)
{
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, n);
	if (!res) {
		dev_err(&pdev->dev, "No resource %i defined\n", n);
		return -ENOMEM;
	}
	*ptr = ioremap(res->start, res->end + 1 - res->start);
	if (!*ptr) {
		dev_err(&pdev->dev, "Ioremap for resource %i failed\n", n);
		return -ENOMEM;
	}
	return 0;
}

static int __devinit wrn_probe(struct platform_device *pdev)
{
	struct net_device *netdev;
	struct wrn_devpriv *priv;
	struct wrn_dev *wrn = pdev->dev.platform_data;
	struct resource *res;
	int i, err = 0;

	/* no need to lock_irq: we only protect count and continue unlocked */
	spin_lock(&wrn->lock);
	if (++wrn->use_count != 1) {
		--wrn->use_count;
		spin_unlock(&wrn->lock);
		return -EBUSY;
	}
	spin_unlock(&wrn->lock);

	/* These memory regions are mapped once for all endpoints */
	if ( (err = __wrn_ioremap(pdev, WRN_RES_MEM_NIC, &wrn->base_nic)) )
		goto out;
	if ( (err = __wrn_ioremap(pdev, WRN_RES_MEM_PPSG, &wrn->base_ppsg)) )
		goto out;
	if ( (err = __wrn_ioremap(pdev, WRN_RES_MEM_CALIB, &wrn->base_calib)) )
		goto out;
	if ( (err = __wrn_ioremap(pdev, WRN_RES_MEM_STAMP, &wrn->base_stamp)) )
		goto out;

	/* get the interrupt number from the resource */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	err = request_irq(res->start, wrn_interrupt,
			  IRQF_TRIGGER_LOW | IRQF_SHARED,
			  DRV_NAME,
			  wrn);
	if (err) goto out;
	wrn->irq_registered = 1;

	/* Finally, register one interface per endpoint */
	memset(wrn->dev, 0, sizeof(wrn->dev));
	for (i = 0; i < WRN_NR_ENDPOINTS; i++) {
		netdev = alloc_etherdev(sizeof(struct wrn_devpriv));
		if (!netdev) {
			dev_err(&pdev->dev, "Etherdev alloc failed.\n");
			err = -ENOMEM;
			goto out;
		}
		priv = netdev_priv(netdev);
		priv->wrn = wrn;
		priv->ep_number = i;

		/* The netdevice thing is registered from the endpoint */
		err = wrn_endpoint_probe(netdev);
		if (err == -ENODEV)
			break;
		if (err)
			goto out;
		/* This endpoint went in properly */
		wrn->dev[i] = netdev;
	}
	err = 0;
out:
	if (err) {
		/* Call the remove function to avoid duplicating code */
		wrn_remove(pdev);
	}
	return err;
}

/* This is not static as ./module.c is going to register it */
struct platform_driver wrn_driver = {
	.probe		= wrn_probe,
	.remove		= wrn_remove, /* not __exit_p as probe calls it */
	/* No suspend or resume by now */
	.driver		= {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
	},
};
