
#ifndef __FIQ_AT91SAM92_H__
#define __FIQ_AT91SAM92_H__
/* defines for atmel 926x */
#include <mach/gpio.h>
#include <mach/at91_tc.h>


/*
 * We have different setups for each of the three processors.
 * This is mainly because of different names, but also because the 9263 differs
 */

#ifdef CONFIG_ARCH_AT91SAM9G45  /* one interrupt for tc block; use TC0 */
   /* Limits we support */
#  define FIQ_IRQ_MIN AT91SAM9G45_ID_TCB
#  define FIQ_IRQ_MAX AT91SAM9G45_ID_TCB
   /* Choices we make */
#  define FIQ_IRQNR AT91SAM9G45_ID_TCB
#  define FIQ_BASE  AT91SAM9G45_BASE_TC0
#endif

#ifdef CONFIG_ARCH_AT91SAM9263  /* one interrupt for tc block; use TC0 */
   /* Limits we support */
#  define FIQ_IRQ_MIN AT91SAM9263_ID_TCB
#  define FIQ_IRQ_MAX AT91SAM9263_ID_TCB
   /* Choices we make */
#  define FIQ_IRQNR AT91SAM9263_ID_TCB
#  define FIQ_BASE  AT91SAM9263_BASE_TC0
#  define FIQ_BITNR AT91_PIN_PB20
#endif

#ifdef CONFIG_ARCH_AT91SAM9260 /* Use TC0 but any of TC0-TC2 can work */
   /* Limits we support */
#  define FIQ_IRQ_MIN AT91SAM9260_ID_TC0
#  define FIQ_IRQ_MAX AT91SAM9260_ID_TC2
   /* Choices we make */
#  define FIQ_IRQNR AT91SAM9260_ID_TC0
#  define FIQ_BASE  AT91SAM9260_BASE_TC0
#  define FIQ_BITNR AT91_PIN_PB20
#endif

#ifdef CONFIG_ARCH_AT91SAM9261 /* Use TC0 but any of TC0-TC2 can work */
   /* Limits we support */
#  define FIQ_IRQ_MIN AT91SAM9261_ID_TC0
#  define FIQ_IRQ_MAX AT91SAM9261_ID_TC2
   /* Choices we make */
#  define FIQ_IRQNR AT91SAM9261_ID_TC0
#  define FIQ_BASE  AT91SAM9261_BASE_TC0
#  define FIQ_BITNR AT91_PIN_PA20 /* very few pins are available */
#endif





#define FIQ_MHZ 3 /* timer clock runs at 99.328MHz/32 = 3.104MHz */
#define FIQ_TASK_CONVERT  1000, 3104 /* sysctl-stamp, used in fiq-task.c */

extern void *__fiq_timer_base; /* for register access to the timer */

/* how to get a time stamp (counts, not usec). This cpu counts up */
#define __GETSTAMP() __raw_readl(__fiq_timer_base + AT91_TC_CV)

static inline void __fiq_ack(void)
{
    /* read status register */
    static int foo;
    foo = __raw_readl( __fiq_timer_base + AT91_TC_SR);
}

static inline void __fiq_sched_next_irq(int usec)
{
    __raw_writel(usec * FIQ_MHZ, __fiq_timer_base + AT91_TC_RC);
}

/* This is not needed for the fiq engine, but our example task wants it */
#define __fiq_set_gpio_value(a,b) at91_set_gpio_value(a, b);

#endif /* __FIQ_AT91SAM92_H__ */
