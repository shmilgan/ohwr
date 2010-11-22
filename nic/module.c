/*
 * Module-related material for wr-nic: load and unload
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/platform_device.h>

#include "wr-nic.h"

/* Our platform data is actually the device itself, and we have 1 only */
static struct wrn_dev wrn_dev;

/* The WRN_RES_ names are defined in the header file. Each is 64kB */
#define __RES(x) .start = x, .end = x + 0x10000 - 1, .flags = IORESOURCE_MEM

static struct resource wrn_resources[] = {
	[WRN_RES_MEM_NIC]	= { __RES( FPGA_BASE_NIC ) },
	[WRN_RES_MEM_PPSG]	= { __RES( FPGA_BASE_PPSG ) },
	[WRN_RES_MEM_CALIB]	= { __RES( FPGA_BASE_CALIB ) },
	[WRN_RES_MEM_STAMP]	= { __RES( FPGA_BASE_STAMP ) },

	/* Last is the interrupt */
	[WRN_RES_IRQ] = {
		.start =	WRN_INTERRUPT,
		.end =		WRN_INTERRUPT,
		.flags =	IORESOURCE_IRQ,
	}
};
#undef __RES

static struct platform_device wrn_device = {
	.name = DRV_NAME,
	.id = 0,
	.resource = wrn_resources,
	.num_resources = ARRAY_SIZE(wrn_resources),
	.dev = {
		.platform_data = &wrn_dev,
		/* dma_mask not used, as we make no DMA */
	},
};

/*
 * Module init and exit stuff. Here we register the platform data
 * as well, but the driver itself is in device.c
 */
int __init wrn_init(void)
{
	/* A few fields must be initialized at run time */
	spin_lock_init(&wrn_dev.lock);

	platform_device_register(&wrn_device);
	platform_driver_register(&wrn_driver);
	return -EAGAIN;
}

void __exit wrn_exit(void)
{
	platform_driver_unregister(&wrn_driver);
	platform_device_unregister(&wrn_device);
	return;
}

module_init(wrn_init);
module_exit(wrn_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:wr-nic");
