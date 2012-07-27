/* Software PWM emulation for AT91SAM9xxx MCUs. Relies on Alessandro's fiq-engine. 

   Licensed under GPL v2 (c) T.W. 2012 

*/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>

#include <mach/gpio.h>
#include <mach/at91_tc.h>
#include <mach/at91sam9g45.h>

#include "fiq-engine.h"
#include "at91_softpwm.h"

struct at91_softpwm_dev {
	int in_use;
	int pin;
	int setpoint;
	int state;
	void *timer_base;
};

#define PERIOD 3000

static struct at91_softpwm_dev dev;

static void at91_softpwm_fiq_handler(void)
{
	static int status;
   status = __raw_readl(dev.timer_base + AT91_TC_SR);
	if(!dev.in_use)
		return;

	if(dev.state)
	{			
		__raw_writel(dev.setpoint,  dev.timer_base + AT91_TC_RC); /* FIQ Rate = 1 kHz */
		at91_set_gpio_value(dev.pin, 0);	
		dev.state = 0;
	} else {
		__raw_writel(PERIOD-dev.setpoint,  dev.timer_base + AT91_TC_RC); /* FIQ Rate = 1 kHz */
		at91_set_gpio_value(dev.pin, 1);	
		dev.state = 1;
	}
}

static long at91_softpwm_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{

	/* Check cmd type */
	if (_IOC_TYPE(cmd) != __AT91_SOFTPWM_IOC_MAGIC)
		return -ENOIOCTLCMD;

	switch(cmd) {
	case AT91_SOFTPWM_ENABLE: 
		dev.pin = arg;
		dev.in_use = 1;
		dev.state = 0;
		return 0;
		
	case AT91_SOFTPWM_DISABLE:
		dev.in_use = 0;
		dev.state = 0;
		return 0;

	case AT91_SOFTPWM_SETPOINT:
  	dev.setpoint = PERIOD - (arg * PERIOD / 1000);

		return 0;

	default:
		return -ENOIOCTLCMD;
	}
}

static struct file_operations at91_softpwm_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = at91_softpwm_ioctl
};

// TODO check available minor numbers
static struct miscdevice at91_softpwm_misc = {
	.minor = 78,
	.name  = "at91_softpwm",
	.fops  = &at91_softpwm_fops
};

int __init at91_softpwm_init(void)
{
	int err;

	printk("at91_softpwm: initializing\n");

	// register misc device
	err = misc_register(&at91_softpwm_misc);
	if (err < 0) {
		printk(KERN_ERR "%s: Can't register misc device\n",
		       KBUILD_MODNAME);
		return err;
	}


	dev.in_use = 0;
	dev.setpoint = 0;
  dev.timer_base = ioremap_nocache(AT91SAM9G45_BASE_TC2, 0x40);

	__fiq_register(at91_softpwm_fiq_handler, AT91SAM9G45_ID_TCB);

  __raw_writel(AT91_TC_TIMER_CLOCK3
    | AT91_TC_WAVE
    | AT91_TC_WAVESEL_UP_AUTO,
  dev.timer_base + AT91_TC_CMR); /* Clock = 3 MHz */

	__raw_writel(PERIOD,  dev.timer_base + AT91_TC_RC); /* FIQ Rate = 1 kHz */
  __raw_writel((1<<4) /* rc/rb compare */, dev.timer_base + AT91_TC_IER);
	__raw_writel(AT91_TC_CLKEN | AT91_TC_SWTRG, dev.timer_base + AT91_TC_CCR);

	return 0;
}

void __exit at91_softpwm_exit(void)
{

	__fiq_unregister(at91_softpwm_fiq_handler, AT91SAM9G45_ID_TCB);

  __raw_writel((1<<4)/* rc compare */, dev.timer_base + AT91_TC_IDR);
	__raw_writel(AT91_TC_CLKDIS, dev.timer_base + AT91_TC_CCR);

	if(dev.in_use)
		at91_set_gpio_value(dev.pin, 0);

	dev.in_use = 0;
	
  misc_deregister(&at91_softpwm_misc);
	iounmap(dev.timer_base);
}

module_init(at91_softpwm_init);
module_exit(at91_softpwm_exit);

MODULE_DESCRIPTION("Atmel AT91SAM9G45 software PWM");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");
