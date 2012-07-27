
/* cpu-specific code for Atmel 92xx timers (actually, only 9263 as it is) */

#include <linux/module.h>
#include <linux/interrupt.h>

#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/at91_aic.h>
#include <mach/at91_pmc.h>
#include <mach/at91_tc.h>

#include "fiq-engine.h"
#include "fiq-at91sam92.h"

static int fiq_irqnr;

/* The following are called from the cpu-independent source file */
int __fiq_register(void (*handler)(void), int irq)
{
    int ret;

    if (irq < FIQ_IRQ_MIN || irq > FIQ_IRQ_MAX) {
	printk(KERN_WARNING "at91-fiq: irq %i not in valid range (%i-%i)\n",
	       irq, FIQ_IRQ_MIN, FIQ_IRQ_MAX);
	return -EINVAL;
    }

    fiq_irqnr = irq; /* save it for ack and so on */

    /* clock the peripheral */
    at91_sys_write(AT91_PMC_PCER, 1 << fiq_irqnr);

    fiq_handler = handler; /* used both from fiq and non-fiq */

    /* request the irq in any case, only turn it to fiq as needed */
    ret = request_irq(irq, fake_fiq_handler, IRQF_DISABLED,
		      "fake-fiq", &fiq_handler);
    if (ret) {
	printk(KERN_WARNING "at91-fiq: can't request irq %i\n", irq);
	return ret;
    }

    if (fiq_use_fiq) {
	at91_sys_write(AT91_AIC_FFER, 1 << fiq_irqnr);
    }

    return 0;
}

void __fiq_unregister(void (*handler)(void), int irq)
{
    if (fiq_use_fiq) {
			at91_sys_write(AT91_AIC_FFDR, 1 << fiq_irqnr);
    }
    free_irq(irq, &fiq_handler);
    fiq_handler = NULL;
}
