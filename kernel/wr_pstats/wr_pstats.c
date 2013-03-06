/*
 * White Rabbit Per-port statistics
 * Copyright (C) 2013, CERN.
 *
 * Version:     wr_pstats v1.0
 *
 * Author:      Grzegorz Daniluk <grzegorz.daniluk@cern.ch>
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/sysctl.h>
#include<linux/io.h>
#include<linux/interrupt.h>
#include<linux/spinlock.h>
#include<linux/moduleparam.h>

#include"../wbgen-regs/pstats-regs.h"
#include"wr_pstats.h"

#define pstats_readl(r)		__raw_readl(&pstats_dev.regs->r)
#define pstats_writel(val, r)	__raw_writel(val, &pstats_dev.regs->r)

static uint8_t param_nports = 18;
static uint32_t portmsk;
module_param(param_nports, byte, S_IRUGO);

struct cntrs_dev {
	uint32_t cntrs[NPORTS*CNT_PP];
	struct PSTATS_WB __iomem *regs;

	/* prevents from simultaneous access to cntrs array from tasklet and 
	 * sysfs handler */
	spinlock_t port_mutex[NPORTS];
	/* circular bufer for passing Port's IRQ mask between irq handler and
	 * the tasklet */
	uint32_t port_irqs[IRQS_BUFSZ];
	volatile uint32_t irqs_head, irqs_tail;
};

static struct cntrs_dev pstats_dev;

static uint32_t* cntr_idx(uint32_t *tab, uint8_t port, uint32_t idx)
{
	return &tab[port*CNT_PP+idx];
}

/* function converting Layer1 and Layer2 words read from HW to counter value */
static int hwcnt_to_sw(uint32_t l1_val, uint32_t l2_val, uint8_t n)
{
	uint32_t lsb, msb;

	lsb = ((l1_val & (0xff<<8*n))>>8*n)&0xff;
	msb = ((l2_val & (0xff<<8*n))>>8*n)&0xff;

	return (((msb<<8)&0xff00) + lsb) & CNT_LSB_MSK;
}

/* IRQ stuff */
static void pstats_irq_enable(uint32_t portmask)
{
	pstats_writel(portmask, EIC_IER);
}

static void pstats_irq_disable(uint32_t portmask)
{
	pstats_writel(portmask, EIC_IDR);
}

static void pstats_irq_clear(uint32_t portmask)
{
	pstats_writel(portmask, EIC_ISR);
}

static uint32_t pstats_irq_status(void)
{
	return pstats_readl(EIC_ISR);
}

static uint32_t pstats_irq_cntrs(uint32_t port)
{
	uint32_t val;

	val = (port<<PSTATS_CR_PORT_SHIFT | PSTATS_CR_RD_IRQ);
	pstats_writel(val, CR);
	return pstats_readl(L1_CNT_VAL);
}

/* Tasklet function, takes irq status for all ports, gets counters' overflow
 * flags and increments appropriate counters*/
static void pstats_tlet(unsigned long arg)
{
	uint32_t irqs, cntrs_ov, cntr;
	uint8_t port;
	uint32_t *ptr;
	struct cntrs_dev *device = (struct cntrs_dev*)arg;

	if(device->irqs_head - device->irqs_tail > IRQS_BUFSZ)
		printk(KERN_WARNING "%s: overflow in port_irqs buffer\n", KBUILD_MODNAME);

	irqs=device->port_irqs[device->irqs_tail++ % IRQS_BUFSZ];
	
	for(port=0; port<param_nports; ++port) {
		if( !(irqs>>port & 0x01) )
			continue;	/*there is no irq from this port*/

		/*now do the irq handling for the port*/
		spin_lock(&device->port_mutex[port]);
		cntrs_ov = pstats_irq_cntrs(port);
		for(cntr=0; cntr<CNT_PP; ++cntr) {
			/*decode counters overflow flags to increment coutners*/
			if( cntrs_ov>>cntr & 0x01 ) {
				ptr = cntr_idx(device->cntrs, port, cntr);
				*ptr += 1<<CNT_LSB;
			}
		}
		spin_unlock(&device->port_mutex[port]);
	}
}
DECLARE_TASKLET(proc_ports, pstats_tlet, (unsigned long)&pstats_dev);

static irqreturn_t pstats_irq_handler(int irq, void *devid)
{
	struct cntrs_dev *device = (struct cntrs_dev*)devid;
	uint32_t irqs;

	irqs = pstats_irq_status();
	
	pstats_irq_disable(MSK_ALLPORTS);
	pstats_irq_clear(irqs);
	device->port_irqs[device->irqs_head++ % IRQS_BUFSZ] = irqs;
	tasklet_schedule(&proc_ports);
	pstats_irq_enable(portmsk);

	return IRQ_HANDLED;
}


/* SYSFS handler, reads counters from hw and passes to sysfs */
static int pstats_handler(ctl_table *ctl, int write, void *buffer, 
		size_t *lenp, loff_t *ppos)
{
	int ret;
	uint8_t* port;
	
	port = (uint8_t*)ctl->extra1;
	if(!write)
		pstats_rd_cntrs(*port);

	ret = proc_dointvec(ctl, 0, buffer, lenp, ppos);

	return ret;
}

static ctl_table pstats_proc[19];	/* initialized in _init function */

static ctl_table proc_table[] = {
	{
		.procname = "pstats",
		.mode = 0555,
		.child	= pstats_proc,
	},
	{0,}
};


static int rd_cnt_word(uint8_t port, int adr)
{
	uint32_t val[2];
	uint32_t *ptr;
	uint8_t i;

	val[0] = ( adr<<PSTATS_CR_ADDR_SHIFT | 
			port<<PSTATS_CR_PORT_SHIFT | PSTATS_CR_RD_EN );

	pstats_writel(val[0], CR);

	val[0] = pstats_readl(L1_CNT_VAL);
	val[1] = pstats_readl(L2_CNT_VAL);

	for(i=0; i<=3; ++i) {
		if(4*adr+i>=CNT_PP) break;
		spin_lock(&pstats_dev.port_mutex[port]);
		ptr = cntr_idx(pstats_dev.cntrs, port, 4*adr+i);
		*ptr &= CNT_MSB_MSK;
		*ptr |= hwcnt_to_sw(val[0], val[1], i);
		spin_unlock(&pstats_dev.port_mutex[port]);
	}

	return 0;
}

static int pstats_rd_cntrs(uint8_t port)
{
	int adr;

	for(adr=0; adr<ADR_PP; ++adr) {
		rd_cnt_word(port, adr);
	}

	return 0;
}

static struct ctl_table_header *pstats_header;

static int __init pstats_init(void)
{
	int err = 0;
	uint32_t i;

	/*convert nports to one-hot port mask (for enabling IRQs*/
	printk(KERN_INFO "nports=%u\n", param_nports);
	portmsk = 1;
	for(i=0; i<param_nports; ++i)
		portmsk *= 2;
	portmsk -= 1;

	printk(KERN_INFO "PORTMSK=%x\n", portmsk);

	for(i=0; i<param_nports; ++i) {
		pstats_proc[i].procname = portnames[i];
		pstats_proc[i].data	= &pstats_dev.cntrs[i*CNT_PP];
		pstats_proc[i].maxlen	= CNT_PP*sizeof(uint32_t);
		pstats_proc[i].mode	= 0444;
		pstats_proc[i].proc_handler = pstats_handler;
		pstats_proc[i].extra1	= (void*)&portnums[i];
	}
	pstats_proc[param_nports].procname = NULL;

	for(i=0; i<(NPORTS*CNT_PP); ++i)
		pstats_dev.cntrs[i] = 0;
	pstats_dev.irqs_head = 0;
	pstats_dev.irqs_tail = 0;

	pstats_header = register_sysctl_table(proc_table);

	if(!pstats_header) {
		err=-EBUSY;
		goto err_exit;
	}

	/*map PSTATS Wishbone registers*/
	pstats_dev.regs = ioremap(FPGA_BASE_PSTATS, sizeof(struct PSTATS_WB));
	if(!pstats_dev.regs) {
		printk(KERN_ERR "%s: could not map PSTATS registers\n", KBUILD_MODNAME);
		unregister_sysctl_table(pstats_header);
		err = -ENOMEM;
		goto err_exit;
	}

	/*request pstats IRQ*/
	pstats_irq_disable(MSK_ALLPORTS);
	err = request_irq( WRVIC_BASE_IRQ+WR_PSTATS_IRQ, pstats_irq_handler, 
			IRQF_SHARED, "wr_pstats", (void*)&pstats_dev);
	if(err) {
		printk(KERN_ERR "%s: cannot request interrupt\n", KBUILD_MODNAME);
		iounmap(pstats_dev.regs);
		unregister_sysctl_table(pstats_header);
		goto err_exit;
	}

	for(i=0; i<NPORTS; ++i)
		spin_lock_init(&pstats_dev.port_mutex[i]);
	pstats_irq_enable(portmsk);
	
	printk(KERN_INFO "%s: initialized\n", KBUILD_MODNAME);
	return 0;

err_exit:
	printk(KERN_ERR "%s: could not initialize\n", KBUILD_MODNAME);
	return err;
}

static void __exit pstats_exit(void)
{
	pstats_irq_disable(MSK_ALLPORTS);
	free_irq(WRVIC_BASE_IRQ+WR_PSTATS_IRQ, (void*)&pstats_dev);

	iounmap(pstats_dev.regs);
	unregister_sysctl_table(pstats_header);

	printk(KERN_INFO "%s: removed\n", KBUILD_MODNAME);
}

module_init(pstats_init);
module_exit(pstats_exit);

MODULE_DESCRIPTION("WRSW PSTATS counters reading module");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Grzegorz Daniluk");
