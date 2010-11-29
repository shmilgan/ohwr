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
#include <linux/delay.h>
#include <linux/io.h>

#include "wr-nic.h"

/* The remove function is used by probe, so it's not __devexit */
static int __devexit wrn_remove(struct platform_device *pdev)
{
	struct wrn_dev *wrn = pdev->dev.platform_data;
	int i;

	spin_lock(&wrn->lock);
	--wrn->use_count; /* Hmmm... looks like overkill... */
	spin_unlock(&wrn->lock);

	/* First of all, stop any transmission */
	writel(0, &wrn->regs->CR);

	/* Then remove devices, memory maps, interrupts */
	for (i = 0; i < WRN_NR_ENDPOINTS; i++) {
		if (wrn->dev[i]) {
			wrn_endpoint_remove(wrn->dev[i]);
			free_netdev(wrn->dev[i]);
			wrn->dev[i] = NULL;
		}
	}

	for (i = 0; i < ARRAY_SIZE(wrn->bases); i++) {
		if (wrn->bases[i])
			iounmap(wrn->bases[i]);
	}

	/* Unregister all interrupts that were registered */
	for (i = 0; wrn->irq_registered; i++) {
		static int irqs[] = WRN_IRQ_NUMBERS;
		if (wrn->irq_registered & (1 << i))
			free_irq(irqs[i], wrn);
		wrn->irq_registered &= ~(1 << i);
	}
	return 0;
}

/* This helper is used by probe below */
static int __devinit __wrn_map_resources(struct platform_device *pdev)
{
	int n, i = 0;
	struct resource *res;
	void __iomem *ptr;
	struct wrn_dev *wrn = pdev->dev.platform_data;

	/*
	 * The memory regions are mapped once for all endpoints.
	 * We don't populate the whole array, but use the resource list
	 */
	while ( (res =platform_get_resource(pdev, IORESOURCE_MEM, i)) ) {
		ptr = ioremap(res->start, res->end + 1 - res->start);
		if (!ptr) {
			dev_err(&pdev->dev, "Remap for res %i (%08x) failed\n",
				i, res->start);
			return -ENOMEM;
		}
		/* Hack: find the block number and fill the array */
		n = __FPGA_BASE_TO_NR(res->start);
		pr_debug("Remapped %08x (block %i) to %p\n",
			 res->start, n, ptr);
		wrn->bases[n] = ptr;

		i++; /* next please */
	}
	return 0;
}

static int __devinit wrn_probe(struct platform_device *pdev)
{
	struct net_device *netdev;
	struct wrn_ep *ep;
	struct wrn_dev *wrn = pdev->dev.platform_data;
	int i, err = 0;

	/* Lazily: irqs are not in the resource list */
	static int irqs[] = WRN_IRQ_NUMBERS;
	static char *irq_names[] = WRN_IRQ_NAMES;
	static irq_handler_t irq_handlers[] = WRN_IRQ_HANDLERS;

	/* No need to lock_irq: we only protect count and continue unlocked */
	spin_lock(&wrn->lock);
	if (++wrn->use_count != 1) {
		--wrn->use_count;
		spin_unlock(&wrn->lock);
		return -EBUSY;
	}
	spin_unlock(&wrn->lock);

	/* Map our resource list and instantiate the shortcut pointers */
	if ( (err = __wrn_map_resources(pdev)) )
		goto out;
	wrn->regs = wrn->bases[WRN_BLOCK_NIC];
	wrn->txd = (void *)&wrn->regs->TX1_D1;
	wrn->rxd = (void *)&wrn->regs->RX1_D1;

	/* Register the interrupt handlers (not shared) */
	for (i = 0; i < ARRAY_SIZE(irq_names); i++) {
		err = request_irq(irqs[i], irq_handlers[i],
			      IRQF_TRIGGER_LOW, irq_names[i], wrn);
		if (err) goto out;
		wrn->irq_registered |= 1 << i;
	}
	/* Reset the device, just to be sure, before making anything */
	writel(0, &wrn->regs->CR);
	mdelay(10);

	/* Finally, register one interface per endpoint */
	memset(wrn->dev, 0, sizeof(wrn->dev));
	for (i = 0; i < WRN_NR_ENDPOINTS; i++) {
		netdev = alloc_etherdev(sizeof(struct wrn_ep));
		if (!netdev) {
			dev_err(&pdev->dev, "Etherdev alloc failed.\n");
			err = -ENOMEM;
			goto out;
		}
		/* The ep structure is filled before calling ep_probe */
		ep = netdev_priv(netdev);
		ep->wrn = wrn;
		ep->ep_regs = wrn->bases[WRN_FIRST_EP + i];
		ep->ep_number = i;
		if (i < WRN_NR_UPLINK)
			set_bit(WRN_EP_IS_UPLINK, &ep->ep_flags);

		/* The netdevice thing is registered from the endpoint */
		err = wrn_endpoint_probe(netdev);
		if (err == -ENODEV)
			break;
		if (err)
			goto out;
		/* This endpoint went in properly */
		wrn->dev[i] = netdev;
	}
	err = 0; /* no more endpoints, we succeeded. Enable the device */
	writel(NIC_CR_RX_EN | NIC_CR_TX_EN, &wrn->regs->CR);

out:
	if (err) {
		/* Call the remove function to avoid duplicating code */
		wrn_remove(pdev);
	} else {
		dev_info(&pdev->dev, "White Rabbit NIC driver\n");
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
