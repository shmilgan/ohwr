/**
 * Copyright CERN (c) 2016
 * Federico Vaga <federico.vaga@cern.ch>
 * License GPL v2
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>

#include <mach/at91sam9g45.h>
/*
 * Ugly trick to be able to use headers that have been moved out
 * from mach/ directory
 */
#include <mach/../../at91_aic.h>
#define WR_IS_NODE 0
#define WR_IS_SWITCH 1
#include "../wr_nic/nic-hardware.h" /* Magic numbers: please fix them as needed */


#define FPGA_BASE_WRVIC	0x10050000
#define FPGA_SIZE_WRVIC	0x00001000

static struct resource wrs_vic_resources[] = {
	[0] = {
		.start	= FPGA_BASE_WRVIC,
		.end	= FPGA_BASE_WRVIC + FPGA_SIZE_WRVIC - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= NR_IRQS_LEGACY + AT91SAM9G45_ID_IRQ0,
		.end	= NR_IRQS_LEGACY + AT91SAM9G45_ID_IRQ0,
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
		/* FIXME is it really HIGHLEVEL ? */
	},
};

static struct platform_device wrs_vic_device = {
	.name		= "htvic-wr-swi",
	.id		= 0,
	.resource	= wrs_vic_resources,
	.num_resources	= ARRAY_SIZE(wrs_vic_resources),
};

#if 0
static struct resource wrs_nic_recources[] = {
	/* Memory */
	{
		.name = "wrs-mem-nic",
		.start = FPGA_BASE_NIC,
		.end = FPGA_BASE_NIC + FPGA_SIZE_NIC - 1,
		.flags = IORESOURCE_MEM,
	}, {
		.name = "wrs-mem-ep",
		.start = FPGA_BASE_EP,
		.end = FPGA_BASE_EP + FPGA_SIZE_EP - 1,
		.flags = IORESOURCE_MEM,
	}, {
		.name = "wrs-mem-ts",
		.start = FPGA_BASE_TS,
		.end = FPGA_BASE_TS + FPGA_SIZE_TS - 1,
		.flags = IORESOURCE_MEM,
	}, {
		.name = "wrs-mem-ppsg",
		.start = FPGA_BASE_PPSG,
		.end = FPGA_BASE_PPSG + FPGA_SIZE_PPSG - 1,
		.flags = IORESOURCE_MEM,
	},
	/* IRQ from HTVIC */
	{
		.name = "wr-nic-irq",
		.start = 0,
		.end = 0,
		.flags = IORESOURCE_IRQ,
	}, {
		.name = "wr-tstamp",
		.start = 1,
		.end = 1,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device wrs_nic_device = {
	.name = "wrs-nic",
	.id = 0,
	.resource = wrs_nic_recources,
	.num_resources = ARRAY_SIZE(wrs_nic_recources),
};
#endif

static int wrs_devices_init(void)
{
	int err;

	err = platform_device_register(&wrs_vic_device);
	if (err)
		return err;
#if 0 /* perhaps we enable it in the future */
	err = platform_device_register(&wrs_nic_device);
	if (err)
		return err;
#endif
	return 0;
}

static void wrs_devices_exit(void)
{
#if 0
	platform_device_unregister(&wrs_nic_device);
#endif
	platform_device_unregister(&wrs_vic_device);
}

module_init(wrs_devices_init);
module_exit(wrs_devices_exit);

MODULE_LICENSE("GPL");
