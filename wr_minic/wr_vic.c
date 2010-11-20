/*
 * whiterabbit_vic.c
 *
 *  Copyright (c) 2009 Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/slab.h>

#include <asm/mach-types.h>
#include <asm/setup.h>
#include <asm/irq.h>
#include <asm/atomic.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>


#include "wr_vic.h"

#define DRV_MODULE_VERSION "0.1"
#define DRV_NAME "wr_vic"
#define PFX DRV_NAME ": "


/* [0x0]: REG VIC Control Register */
#define VIC_REG_CTL 0x00000000
/* [0x4]: REG Raw Interrupt Status Register */
#define VIC_REG_RISR 0x00000004
/* [0x8]: REG Interrupt Enable Register */
#define VIC_REG_IER 0x00000008
/* [0xc]: REG Interrupt Disable Register */
#define VIC_REG_IDR 0x0000000c
/* [0x10]: REG Interrupt Mask Register */
#define VIC_REG_IMR 0x00000010
/* [0x14]: REG Vector Address Register */
#define VIC_REG_VAR 0x00000014
/* [0x1c]: REG End Of Interrupt Acknowledge Register */
#define VIC_REG_EOIR 0x0000001c

#define VIC_CTL_ENABLE                        (1<<0)
#define VIC_CTL_POL                           (1<<1)

#define VIC_IVT_BASE 0x00000080

#define VIC_SPURIOUS_IRQ 0x12345678

#define vic_readl(vic, offs)			\
	__raw_readl((vic)->regs+ (offs))

#define vic_writel(vic, offs, value)			\
	__raw_writel((value), (vic)->regs + (offs))



struct wrmch_vic {
	void __iomem *regs;
	wrvic_irq_t handlers[WRMCH_VIC_MAX_IRQS];
	void *dev_ids[WRMCH_VIC_MAX_IRQS];
};

static struct wrmch_vic *VIC;


static irqreturn_t wrmch_vic_interrupt(int irq, void *dev_id)
{
	u32 reg_var;
	wrvic_irq_t handler;

	reg_var = vic_readl(VIC, VIC_REG_VAR); // determine the interrupt source

	if (reg_var == VIC_SPURIOUS_IRQ) {
		printk(KERN_ERR PFX "spurious interrupt");
	} else {
		//			printk("Got irq: var %d\n", reg_var);
		handler = VIC->handlers[reg_var];
		handler(VIC->dev_ids[reg_var]);
	}

	vic_writel(VIC, VIC_REG_EOIR, 0); // clear the interrupt pending flag

	return IRQ_HANDLED;
}


int wrmch_vic_request_irq(int irq, wrvic_irq_t handler, void *dev_id)
{
	if (irq < 0 || irq >= WRMCH_VIC_MAX_IRQS)
		return -EINVAL;

	if (VIC->handlers[irq])
		return -EADDRINUSE;

	VIC->handlers[irq] = handler;
	VIC->dev_ids[irq] = dev_id;

	vic_writel(VIC, VIC_IVT_BASE + (irq << 2), irq); //(u32)handler);
	wrmch_vic_enable_irq(irq);

	return 0;
}

void wrmch_vic_free_irq(int irq, void *dev_id)
{
	if (irq < 0 || irq >= WRMCH_VIC_MAX_IRQS)
		return;

	if (VIC->dev_ids[irq] != dev_id)
		return;

	wrmch_vic_disable_irq(irq);
	vic_writel(VIC, VIC_IVT_BASE + (irq << 2), VIC_SPURIOUS_IRQ);
	VIC->handlers[irq] = NULL;
	VIC->dev_ids[irq] = NULL;
}

void wrmch_vic_enable_irq(int irq)
{
	if (irq < 0 || irq >= WRMCH_VIC_MAX_IRQS)
		return;

	vic_writel(VIC, VIC_REG_IER, (1<<irq));
//printk("vic_enable_irq: irq %d imr %x\n", irq, vic_readl(VIC, VIC_REG_IMR));
}

void wrmch_vic_disable_irq(int irq)
{
	if (irq < 0 || irq >= WRMCH_VIC_MAX_IRQS)
		return;

	vic_writel(VIC, VIC_REG_IDR, (1<<irq));
//printk("vic_disable_irq: irq %d imr %x\n", irq, vic_readl(VIC, VIC_REG_IMR));
}

static int __devinit wrmch_vic_init_module(void)
{
	int err, i;

	VIC = kzalloc(sizeof(struct wrmch_vic), GFP_KERNEL);

	VIC->regs = ioremap(FPGA_BASE_VIC, 0x1000);

	vic_writel(VIC, VIC_REG_CTL, 0); // output is active LO
	vic_writel(VIC, VIC_REG_IDR, 0xffffffff); // disable all interrupts

	err = request_irq(AT91SAM9263_ID_IRQ0, wrmch_vic_interrupt,
			  IRQF_TRIGGER_LOW | IRQF_SHARED, "whiterabbit_vic", 
			  VIC);

	if (unlikely(err)) {
		printk(KERN_ERR PFX "request IRQ for WR VIC failed");
		return err;
	}

	/* clear the vector table */
	for (i = 0; i < WRMCH_VIC_MAX_IRQS; i++)
		vic_writel(VIC, VIC_IVT_BASE + (i<<2), VIC_SPURIOUS_IRQ);

	vic_writel(VIC, VIC_REG_CTL, VIC_CTL_ENABLE); // enable the VIC

	printk(KERN_INFO PFX "module initialized");

	return 0;
}


static void __devexit wrmch_vic_cleanup_module(void)
{
	free_irq(AT91SAM9263_ID_IRQ0,  VIC);

	if (VIC) {
		if (VIC->regs) {
			iounmap(VIC->regs);
			VIC->regs = NULL;
		}
		kfree(VIC);
		VIC = NULL;
	}

	printk(KERN_INFO PFX "module cleanup");
}

EXPORT_SYMBOL_GPL(wrmch_vic_request_irq);
EXPORT_SYMBOL_GPL(wrmch_vic_free_irq);
EXPORT_SYMBOL_GPL(wrmch_vic_enable_irq);
EXPORT_SYMBOL_GPL(wrmch_vic_disable_irq);

module_init(wrmch_vic_init_module);
module_exit(wrmch_vic_cleanup_module);

MODULE_AUTHOR("Tomasz Wlostowski");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("White Rabbit MCH Vectored Interrupt Controller driver");
MODULE_VERSION(DRV_MODULE_VERSION);


