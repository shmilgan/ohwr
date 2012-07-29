#ifndef __FIQ_ENGINE_H__
#define __FIQ_ENGINE_H__

#include <linux/irqreturn.h>

/* include cpu-specific defines to customize use */
#if defined(CONFIG_ARCH_AT91)
#  include "fiq-at91sam92.h"
#elif defined(CONFIG_ARCH_PXA)
#  include "fiq-pxa.h"
#elif defined(CONFIG_ARCH_OMAP3)
#  include "fiq-omap3.h"
#else
#  error "This package has no support for your architecture"
#endif

/* exported by the kernel (our patch) */
extern void (*fiq_userptr)(void);

/* declared in fiq-asm.S */
extern void fiq_entry(void);
extern unsigned long fiqcount;
extern void (*fiq_handler)(void);

/* declared in fiq-module.c */
extern irqreturn_t fake_fiq_handler(int irq, void *dev);
extern int fiq_verbose;
extern int fiq_use_fiq; /* false by default */
extern int fiq_register(void (*handler)(void), int irq);
extern int fiq_unregister(void (*handler)(void), int irq);


/* declared in the cpu-specific source file */
extern int __fiq_register(void (*handler)(void), int irq);
extern void __fiq_unregister(void (*handler)(void), int irq);

#endif /* __FIQ_ENGINE_H__ */
