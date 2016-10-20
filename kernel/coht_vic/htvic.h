/*
 * Copyright (c) 2016 CERN
 * Author: Federico Vaga <federico.vaga@cern.ch>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __HTVIC_H__
#define __HTVIC_H__

#include "htvic_regs.h"

#define VIC_MAX_VECTORS 32

#define VIC_SDB_VENDOR 0xce42
#define VIC_SDB_DEVICE 0x0013
#define VIC_IRQ_BASE_NUMBER 0

enum htvic_versions {
	HTVIC_VER_SPEC = 0,
	HTVIC_VER_SVEC,
	HTVIC_VER_WRSWI,
};

enum htvic_mem_resources {
	HTVIC_MEM_BASE = 0,
};

struct htvic_data {
	uint32_t is_edge; /* 1 edge, 0 level */
	uint32_t is_raising; /* 1 raising, 0 falling */
	uint32_t pulse_len;
};

struct htvic_device {
	struct platform_device *pdev;
	struct irq_domain *domain;
	unsigned int hwid[VIC_MAX_VECTORS]; /**> original ID from FPGA */
	struct htvic_data *data;
	void __iomem *kernel_va;

	irq_flow_handler_t platform_handle_irq;
	void *platform_handler_data;
};


struct memory_ops {
	u32 (*read)(void *addr);
	void (*write)(u32 value, void *addr);
};

extern struct memory_ops memop;

static inline u32 htvic_ioread(struct htvic_device *htvic, void __iomem *addr)
{
	return memop.read(addr);
}

static inline void htvic_iowrite(struct htvic_device *htvic,
				u32 value, void __iomem *addr)
{
	return memop.write(value, addr);
}


static inline u32 __htvic_ioread32(void *addr)
{
	return ioread32(addr);
}

static inline u32 __htvic_ioread32be(void *addr)
{
	return ioread32be(addr);
}

static inline void __htvic_iowrite32(u32 value,void __iomem *addr)
{
	iowrite32(value, addr);
}

static inline void  __htvic_iowrite32be(u32 value, void __iomem *addr)
{
	iowrite32be(value, addr);
}

#endif
