/*
 * Fast-interrupt infrastructure for a real-time task.
 * Alessandro Rubini, 2007,2008 GPL 2 or later.
 * This code is generic, you must supplement it with cpu-specific code
 * in another source file.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <asm/io.h>

#include "fiq-engine.h"

int fiq_use_fiq; 
EXPORT_SYMBOL(fiq_use_fiq); 

int fiq_verbose;

/*
 * High-level interface (cpu-independent)
 */

/* An irq handler, faking fiq, to test with fiq=0 */
irqreturn_t fake_fiq_handler(int irq, void *dev)
{
    (*fiq_handler)();
    return IRQ_HANDLED;
}

