/*
 * Driver for the HT-VIC IRQ controller
 *
 * Copyright (c) 2016 CERN
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/irqdomain.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
#include <linux/irqchip/chained_irq.h>
#else
static inline void chained_irq_enter(struct irq_chip *chip,
                                     struct irq_desc *desc)
{
        /* FastEOI controllers require no action on entry. */
        if (chip->irq_eoi)
                return;

        if (chip->irq_mask_ack) {
                chip->irq_mask_ack(&desc->irq_data);
        } else {
                chip->irq_mask(&desc->irq_data);
                if (chip->irq_ack)
                        chip->irq_ack(&desc->irq_data);
        }
}

static inline void chained_irq_exit(struct irq_chip *chip,
                                    struct irq_desc *desc)
{
        if (chip->irq_eoi)
                chip->irq_eoi(&desc->irq_data);
        else
                chip->irq_unmask(&desc->irq_data);
}
#endif

#include "htvic.h"

struct memory_ops memop = {
	.read = NULL,
	.write = NULL,
};

/**
 * End of interrupt for the VIC. In case of level interrupt,
 * there is no way to delegate this to the kernel
 */
static void htvic_eoi(struct irq_data *d)
{
	struct htvic_device *vic = irq_data_get_irq_chip_data(d);
	/*
	 * Any write operation acknowledges the pending interrupt.
	 * Then, VIC advances to another pending interrupt(s) or
	 * releases the master interrupt output.
	 */
	htvic_iowrite(vic, 1, vic->kernel_va + VIC_REG_EOIR);
}


static void htvic_mask_disable_reg(struct irq_data *d)
{
	struct htvic_device *htvic = irq_data_get_irq_chip_data(d);

	htvic_iowrite(htvic, 1 << d->hwirq,
		      htvic->kernel_va + VIC_REG_IDR);
}


static void htvic_unmask_enable_reg(struct irq_data *d)
{
	struct htvic_device *htvic = irq_data_get_irq_chip_data(d);

	htvic_iowrite(htvic, 1 << d->hwirq,
		      htvic->kernel_va + VIC_REG_IER);
}

static void htvic_irq_ack(struct irq_data *d)
{
}


/**
 *
 */
static unsigned int htvic_irq_startup(struct irq_data *d)
{
	struct htvic_device *vic = irq_data_get_irq_chip_data(d);
	int ret;

	ret = try_module_get(vic->pdev->dev.driver->owner);
	if (ret == 0) { /* 0 fail, 1 success */
		dev_err(&vic->pdev->dev,
			"Cannot pin the \"%s\" driver. Something really wrong is going on\n",
			vic->pdev->dev.driver->name);
		return 1;
	}
	htvic_unmask_enable_reg(d);
	return 0;
}


/**
 * Executed when a driver does `free_irq()`.
 */
static void htvic_irq_shutdown(struct irq_data *d)
{
	struct htvic_device *vic = irq_data_get_irq_chip_data(d);

	htvic_mask_disable_reg(d);
	module_put(vic->pdev->dev.driver->owner);
}

static struct irq_chip htvic_chip = {
	.name = "HT-VIC",
	.irq_startup = htvic_irq_startup,
	.irq_shutdown = htvic_irq_shutdown,
	.irq_ack  = htvic_irq_ack,
	.irq_eoi = htvic_eoi,
	.irq_mask_ack = htvic_mask_disable_reg,
	.irq_mask = htvic_mask_disable_reg,
	.irq_unmask = htvic_unmask_enable_reg,
};


/**
 * It match a given device with the irq_domain. `struct device_node *` is just
 * a convention. actually it can be anything (I do not understand why kernel
 * people did not use `void *`)
 *
 * In our case here we expect a string because we identify this domain by
 * name
 */
static int htvic_irq_domain_match(struct irq_domain *d, struct device_node *node)
{
	char *name = (char *)node;

	if (strcmp(d->name, name) == 0)
		return 1;
	return 0;
}


/**
 * Given the hardware IRQ and the Linux IRQ number (virtirq), configure the
 * Linux IRQ number in order to handle properly the incoming interrupts
 * on the hardware IRQ line.
 */
static int htvic_irq_domain_map(struct irq_domain *h,
				unsigned int virtirq,
				irq_hw_number_t hwirq)
{
	struct htvic_device *htvic = h->host_data;
	struct irq_desc *desc = irq_to_desc(virtirq);
#if 0
	struct resource *r = platform_get_resource(htvic->pdev,
						   IORESOURCE_IRQ, 0);
#endif
	irq_set_chip(virtirq, &htvic_chip);
	irq_set_handler(virtirq, handle_simple_irq);

	/*
	 * It MUST be no-thread because the VIC EOI must occur AFTER
	 * the device handler ack its signal. Any whay the interrupt from
	 * the carrier is already threaded
	 */
	desc->status_use_accessors |= IRQ_NOTHREAD;
	/* The VIC is only level, it emulates edge */
#if 0
	if (r->flags & (IORESOURCE_IRQ_HIGHEDGE | IORESOURCE_IRQ_LOWEDGE)) {
		__irq_set_handler(virtirq, handle_edge_irq, 0, NULL);
	} else {
		dev_err(&htvic->pdev->dev,
			"Invalid IRQ resource, it must be explicit if it is edge or level\n");
		return -EINVAL;
	}
#endif
	irq_set_chip_data(virtirq, htvic);
	irq_set_handler_data(virtirq, htvic);

	return 0;
}


static struct irq_domain_ops htvic_irq_domain_ops = {
	.match = htvic_irq_domain_match,
	.map = htvic_irq_domain_map,
};


/**
 * Handle cascade IRQ coming from the platform and re-route it properly.
 * When the platform receives an interrupt it will call than this function
 * which then will call the proper handler
 */
static void htvic_handle_cascade_irq(unsigned int irq, struct irq_desc *desc)
{
	struct htvic_device *htvic = irq_get_handler_data(irq);
	struct irq_chip *chip = irq_get_chip(irq);
	unsigned int cascade_irq, /* i = 0, */ vect;
	/* unsigned long status; */

	chained_irq_enter(chip, desc);

	do {
		vect = htvic_ioread(htvic, htvic->kernel_va + VIC_REG_VAR) & 0xFF;
		if (vect >= VIC_MAX_VECTORS)
			goto out;

		cascade_irq = irq_find_mapping(htvic->domain, vect);
		generic_handle_irq(cascade_irq);

		/**
		 * ATTENTION here the ack is actually an EOI.The kernel
		 * does not export the handle_edge_eoi_irq() handler which
		 * is the one we need here. The kernel offers us the
		 * handle_edge_irq() which use only the ack() function.
		 * So what actually we need to to is to call the ack
		 * function but not the eoi function.
		 */
		htvic_eoi(irq_get_irq_data(cascade_irq));
	} while(htvic_ioread(htvic, htvic->kernel_va + VIC_REG_RISR));

out:
	chained_irq_exit(chip, desc);
}


/**
 * Mapping of HTVIC irqs to Linux irqs using linear IRQ domain
 */
static int htvic_irq_mapping(struct htvic_device *htvic)
{
	struct irq_desc *desc;
	int i, irq;

	htvic->domain = irq_domain_add_linear(NULL, VIC_MAX_VECTORS,
					      &htvic_irq_domain_ops, htvic);
	if (!htvic->domain)
		return -ENOMEM;

	htvic->domain->name = kasprintf(GFP_KERNEL, "%s",
					dev_name(&htvic->pdev->dev));

	/* Create the mapping between HW irq and virtual IRQ number */
	for (i = 0; i < VIC_MAX_VECTORS; ++i) {
		htvic->hwid[i] = htvic_ioread(htvic, htvic->kernel_va +
					      VIC_IVT_RAM_BASE + 4 * i);
		htvic_iowrite(htvic, i,
			      htvic->kernel_va + VIC_IVT_RAM_BASE + 4 * i);

		irq = irq_create_mapping(htvic->domain, i);
		if (irq <= 0)
			goto out;
	}


	irq = platform_get_irq(htvic->pdev, 0);
	desc = irq_to_desc(irq);
	htvic->platform_handle_irq = desc->handle_irq;
	htvic->platform_handler_data = desc->irq_data.handler_data;

	if (irq_set_handler_data(irq, htvic) != 0)
		BUG();
	irq_set_chained_handler(irq, htvic_handle_cascade_irq);

	return 0;

out:
	irq_domain_remove(htvic->domain);
	return -EPERM;
}

/**
 * Check if the platform is providing all the necessary information
 * for the HTVIC to work properly.
 *
 * The HTVIC needs the following informations:
 * - a Linux IRQ number where it should attach itself
 * - a virtual address where to find the component
 */
static int htvic_validation(struct platform_device *pdev)
{
	struct resource *r;

	r = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!r) {
		dev_err(&pdev->dev, "Carrier IRQ number is missing\n");
		return -EINVAL;
	}

	if (!(r->flags & (IORESOURCE_IRQ_HIGHEDGE |
			  IORESOURCE_IRQ_LOWEDGE |
			  IORESOURCE_IRQ_HIGHLEVEL |
			  IORESOURCE_IRQ_LOWLEVEL))) {
		dev_err(&pdev->dev,
			"Edge/Level High/Low information missing\n");
		return -EINVAL;
	}

	r = platform_get_resource(pdev, IORESOURCE_MEM, HTVIC_MEM_BASE);
	if (!r) {
		dev_err(&pdev->dev, "VIC base address is missing\n");
		return -EINVAL;
	}

	return 0;
}


/**
 * It acks any pending interrupt in order to avoid to bring the HTVIC
 * to a stable status (possibly)
 */
static inline void htvic_ack_pending(struct htvic_device *htvic)
{
	while (htvic_ioread(htvic, htvic->kernel_va + VIC_REG_RISR))
		htvic_iowrite(htvic, 1, htvic->kernel_va + VIC_REG_EOIR);
}


/**
 * Create a new instance for this driver.
 */
static int htvic_probe(struct platform_device *pdev)
{
	struct htvic_device *htvic;
	const struct resource *r;
	uint32_t ctl;
	int ret;


	/*
	 * TODO theoretically speaking all the confguration should come
	 * from a platform_data structure. Since we do not have it yet,
	 * we proceed this way
	 */
	switch(pdev->id_entry->driver_data) {
	case HTVIC_VER_SPEC:
	case HTVIC_VER_WRSWI:
		memop.read = __htvic_ioread32;
		memop.write = __htvic_iowrite32;
		break;
	case HTVIC_VER_SVEC:
		memop.read = __htvic_ioread32be;
		memop.write = __htvic_iowrite32be;
		break;
	default:
		ret = -EINVAL;
		goto out_memop;
	}

	ret = htvic_validation(pdev);
	if (ret)
		return ret;

	htvic = kzalloc(sizeof(struct htvic_device), GFP_KERNEL);
	if (!htvic)
		return -ENOMEM;
	dev_set_drvdata(&pdev->dev, htvic);
	htvic->pdev = pdev;

	r = platform_get_resource(pdev, IORESOURCE_MEM, HTVIC_MEM_BASE);
	htvic->kernel_va = ioremap(r->start, resource_size(r));

	/* Disable the VIC during the configuration */
	htvic_iowrite(htvic, 0, htvic->kernel_va + VIC_REG_CTL);
	/* Disable also all interrupt lines */
	htvic_iowrite(htvic, ~0, htvic->kernel_va + VIC_REG_IDR);
	/* Ack any pending interrupt */
	htvic_ack_pending(htvic);

	ret = htvic_irq_mapping(htvic);
	if (ret)
		goto out_map;

	/* VIC configuration */
	ctl = 0;
	ctl |= VIC_CTL_ENABLE;
	switch (pdev->id_entry->driver_data) {
	case HTVIC_VER_SPEC:
		ctl |= VIC_CTL_POL;
		ctl |= VIC_CTL_EMU_EDGE;
		ctl |= VIC_CTL_EMU_LEN_W(250);
		break;
	case HTVIC_VER_SVEC:
		ctl |= VIC_CTL_POL;
		ctl |= VIC_CTL_EMU_EDGE;
		ctl |= VIC_CTL_EMU_LEN_W(1000);
		break;
	case HTVIC_VER_WRSWI:
		break;
	default:
		goto out_ctl;
	}
	htvic_iowrite(htvic, ctl, htvic->kernel_va + VIC_REG_CTL);

	return 0;
out_ctl:
out_map:
	kfree(htvic);
out_memop:
	return ret;
}


/**
 * Copied from kernel 3.6 since it's not exported
 * And modified a bit because other helpers are not exported
 */
void __htvic_irq_shutdown(struct irq_desc *desc)
{
	/* irq_state_set_disabled(desc); */
	desc->irq_data.state_use_accessors |= IRQD_IRQ_DISABLED;
	desc->depth = 1;
	if (desc->irq_data.chip->irq_shutdown)
		desc->irq_data.chip->irq_shutdown(&desc->irq_data);
	else if (desc->irq_data.chip->irq_disable)
		desc->irq_data.chip->irq_disable(&desc->irq_data);
	else
		desc->irq_data.chip->irq_mask(&desc->irq_data);
	/* irq_state_set_masked(desc); */
	desc->irq_data.state_use_accessors |= IRQD_IRQ_MASKED;
}

/**
 * Unload the htvic driver from the platform
 */
static int htvic_remove(struct platform_device *pdev)
{
	struct htvic_device *htvic = dev_get_drvdata(&pdev->dev);
	struct irq_desc *desc = irq_to_desc(platform_get_irq(htvic->pdev, 0));
	int i;

	/*
	 * Disable all interrupts to prevent spurious interrupt
	 * Disable also the HTVIC component for the very same reason,
	 * but this way on next instance even if we enable the VIC
	 * no interrupt will come unless configured.
	 */
	htvic_iowrite(htvic, ~0, htvic->kernel_va + VIC_REG_IDR);
	htvic_iowrite(htvic, 0, htvic->kernel_va + VIC_REG_CTL);

	/*
	 * Restore HTVIC vector table with it's original content
	 * Release Linux IRQ number
	 */
	for (i = 0; i < VIC_MAX_VECTORS; i++) {
		htvic_iowrite(htvic, htvic->hwid[i], htvic->kernel_va + VIC_IVT_RAM_BASE + 4 * i);
		irq_dispose_mapping(irq_find_mapping(htvic->domain, i));
	}


	/*
	 * Restore the platform IRQ status by undoing what
	 * irq_set_chained_handler() does in the kernel. I have to do it
	 * manually here because it was not thought that an IRQ controller
	 * may appear later in time.
	 */
	__htvic_irq_shutdown(desc); /* free_irq() */
	irq_set_handler(desc->irq_data.irq, htvic->platform_handle_irq);
	irq_set_handler_data(desc->irq_data.irq, htvic->platform_handler_data);
	irq_modify_status(desc->irq_data.irq,
			  IRQ_NOPROBE | IRQ_NOREQUEST | IRQ_NOTHREAD,
			  0);
	/*
	 * Clear the memory and restore flags when needed
	 */
	kfree(htvic->domain->name);
	irq_domain_remove(htvic->domain);
	kfree(htvic);

	return 0;
}


/**
 * List of supported platform
 */
static const struct platform_device_id htvic_id_table[] = {
	{	/* SPEC compatible */
		.name = "htvic-spec",
		.driver_data = HTVIC_VER_SPEC,
	}, {	/* SVEC compatible */
		.name = "htvic-svec",
		.driver_data = HTVIC_VER_SVEC,
	}, {
		.name = "htvic-wr-swi",
		.driver_data = HTVIC_VER_WRSWI,
	},
	{},
};


static struct platform_driver htvic_driver = {
	.driver = {
		.name = "htvic",
		.owner = THIS_MODULE,
	},
	.id_table = htvic_id_table,
	.probe = htvic_probe,
	.remove = htvic_remove,
};
module_platform_driver(htvic_driver);

MODULE_AUTHOR("Federico Vaga <federico.vaga@cern.ch>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CERN BECOHT VHDL Vector Interrupt Controller - HTVIC");
