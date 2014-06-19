/*
 * White Rabbit Per-port statistics
 * Copyright (C) 2013, CERN.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysctl.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/moduleparam.h>

#include "../wbgen-regs/pstats-regs.h"
#include "wr_pstats.h"

#define pstats_readl(device, r)		__raw_readl(&device.regs->r)
#define pstats_writel(val, device, r)	__raw_writel(val, &device.regs->r)

static int pstats_nports = PSTATS_NPORTS;
static uint32_t portmsk;
module_param(pstats_nports, int, S_IRUGO);

const char *portnames[]  = {"port0", "port1", "port2", "port3", "port4",
	"port5", "port6", "port7", "port8", "port9", "port10", "port11",
	"port12", "port13", "port14", "port15", "port16", "port17"};


struct cntrs_dev {
	unsigned int cntrs[PSTATS_NPORTS*PSTATS_CNT_PP];
	struct PSTATS_WB __iomem *regs;

	/* prevents from simultaneous access to cntrs array from tasklet and
	 * sysfs handler */
	spinlock_t port_mutex[PSTATS_NPORTS];
	/* circular bufer for passing Port's IRQ mask between irq handler and
	 * the tasklet */
	uint32_t port_irqs[PSTATS_IRQBUFSZ];
	volatile int irqs_head;
	int irqs_tail;
};

static struct cntrs_dev pstats_dev;	/*static data cleared at build time*/

static unsigned int *cntr_idx(unsigned int *tab, int port, int idx)
{
	return &tab[port*PSTATS_CNT_PP+idx];
}

/* function converting Layer1 and Layer2 words read from HW to counter value */
static int hwcnt_to_sw(uint32_t l1_val, uint32_t l2_val, int n)
{
	uint32_t lsb, msb;

	lsb = ((l1_val & (0xff<<8*n))>>8*n)&0xff;
	msb = ((l2_val & (0xff<<8*n))>>8*n)&0xff;

	return (msb<<8) | lsb;
}

/* IRQ stuff */
static void pstats_irq_enable(uint32_t portmask)
{
	pstats_writel(portmask, pstats_dev, EIC_IER);
}

static void pstats_irq_disable(uint32_t portmask)
{
	pstats_writel(portmask, pstats_dev, EIC_IDR);
}

static void pstats_irq_clear(uint32_t portmask)
{
	pstats_writel(portmask, pstats_dev, EIC_ISR);
}

static uint32_t pstats_irq_status(void)
{
	return pstats_readl(pstats_dev, EIC_ISR);
}

static uint32_t pstats_irq_cntrs(int port)
{
	uint32_t val;

	val = (port<<PSTATS_CR_PORT_SHIFT | PSTATS_CR_RD_IRQ);
	pstats_writel(val, pstats_dev, CR);
	return pstats_readl(pstats_dev, L1_CNT_VAL);
}

/* Tasklet function, takes irq status for all ports, gets counters' overflow
 * flags and increments appropriate counters*/
static void pstats_tlet_fn(unsigned long arg)
{
	uint32_t irqs, cntrs_ov;
	int port, cntr;
	unsigned int *ptr;
	struct cntrs_dev *device = (struct cntrs_dev *)arg;

	if (device->irqs_head - device->irqs_tail > PSTATS_IRQBUFSZ) {
		printk(KERN_WARNING "%s: overflow in port_irqs buffer\n",
				KBUILD_MODNAME);
		/*recover from error situation*/
		device->irqs_tail += PSTATS_IRQBUFSZ;
	}

	irqs = device->port_irqs[device->irqs_tail++ % PSTATS_IRQBUFSZ];

	/* level 1 of IRQs is device->port_irqs where each bit set to 1 says
	 * there is at least one overflowed counter on the port correcponding
	 * to the bit */
	for (port = 0; port < pstats_nports; ++port) {
		if (!(irqs>>port & 0x01))
			continue;	/*there is no irq from this port*/

		/* Level 2 of IRQs is reading additional IRQ flags register for
		 * each port that has bit in device->port_irqs set to 1. This
		 * register is a set of IRQ flags per-counter for a given port.
		 * That means, when i-th bit in cntrs_ov is set, i-th counter
		 * for this port has to be incremented by 1<<PSTATS_MSB_SHIFT */
		spin_lock(&device->port_mutex[port]);
		cntrs_ov = pstats_irq_cntrs(port);
		for (cntr = 0; cntr < PSTATS_CNT_PP; ++cntr) {
			/*decode counters overflow flags to increment coutners*/
			if (cntrs_ov>>cntr & 0x01) {
				ptr = cntr_idx(device->cntrs, port, cntr);
				*ptr += 1<<PSTATS_MSB_SHIFT;
			}
		}
		spin_unlock(&device->port_mutex[port]);
	}
}
DECLARE_TASKLET(proc_ports, pstats_tlet_fn, (unsigned long)&pstats_dev);

static irqreturn_t pstats_irq_handler(int irq, void *devid)
{
	struct cntrs_dev *device = (struct cntrs_dev *)devid;
	uint32_t irqs;

	irqs = pstats_irq_status();

	pstats_irq_disable(PSTATS_ALL_MSK);
	pstats_irq_clear(irqs);
	device->port_irqs[device->irqs_head++ % PSTATS_IRQBUFSZ] = irqs;
	tasklet_schedule(&proc_ports);
	pstats_irq_enable(portmsk);

	return IRQ_HANDLED;
}

static int rd_cnt_word(int port, int adr)
{
	uint32_t val[2];
	unsigned int *ptr;
	int i;

	val[0] = (adr<<PSTATS_CR_ADDR_SHIFT |
			port<<PSTATS_CR_PORT_SHIFT | PSTATS_CR_RD_EN);

	pstats_writel(val[0], pstats_dev, CR);

	val[0] = pstats_readl(pstats_dev, L1_CNT_VAL);
	val[1] = pstats_readl(pstats_dev, L2_CNT_VAL);

	for (i = 0; i <= 3; ++i) {
		if (4*adr+i >= PSTATS_CNT_PP)
			break;
		spin_lock(&pstats_dev.port_mutex[port]);
		ptr = cntr_idx(pstats_dev.cntrs, port, 4*adr+i);
		*ptr &= PSTATS_MSB_MSK;
		*ptr |= hwcnt_to_sw(val[0], val[1], i);
		spin_unlock(&pstats_dev.port_mutex[port]);
	}

	return 0;
}

static int pstats_rd_cntrs(int port)
{
	int adr;

	for (adr = 0; adr < PSTATS_ADR_PP; ++adr)
		rd_cnt_word(port, adr);

	return 0;
}

/* SYSCTL handler, reads counters from hw and passes to sysfs */
static int pstats_handler(ctl_table *ctl, int write, void *buffer,
		size_t *lenp, loff_t *ppos)
{
	int ret;
	int port;

	port = (int)ctl->extra1;
	if (!write)
		pstats_rd_cntrs(port);

	ret = proc_dointvec(ctl, 0, buffer, lenp, ppos);

	return ret;
}

static ctl_table pstats_ctl_table[19];	/* initialized in _init function */

static ctl_table proc_table[] = {
	{
		.procname = "pstats",
		.mode = 0555,
		.child	= pstats_ctl_table,
	},
	{0,}
};

/*
 * This module is optional, in a way, and if there it is loaded after
 * wr_nic. So it must register itself to wr_nic, to export this.
 */
int pstats_callback(int epnum, unsigned int cntr[PSTATS_CNT_PP])
{
	int i;

	pstats_rd_cntrs(epnum);
	for (i = 0; i < PSTATS_CNT_PP; i++)
		cntr[i] = *cntr_idx(pstats_dev.cntrs, epnum, i);
	return 0;
}

static struct ctl_table_header *pstats_header;

static int __init pstats_init(void)
{
	int i, err = 0;

	/*convert nports to one-hot port mask (for enabling IRQs*/
	printk(KERN_INFO "nports=%u\n", pstats_nports);
	portmsk = (1 << pstats_nports) - 1;

	printk(KERN_INFO "PORTMSK=%x\n", portmsk);

	for (i = 0; i < pstats_nports; ++i) {
		pstats_ctl_table[i].procname = portnames[i];
		pstats_ctl_table[i].data = &pstats_dev.cntrs[i*PSTATS_CNT_PP];
		pstats_ctl_table[i].maxlen = PSTATS_CNT_PP*sizeof(unsigned int);
		pstats_ctl_table[i].mode = 0444;
		pstats_ctl_table[i].proc_handler = pstats_handler;
		pstats_ctl_table[i].extra1 = (void *)i;
	}

	pstats_header = register_sysctl_table(proc_table);

	if (!pstats_header) {
		err = -EBUSY;
		goto err_exit;
	}

	/*map PSTATS Wishbone registers*/
	pstats_dev.regs = ioremap(FPGA_BASE_PSTATS, sizeof(struct PSTATS_WB));
	if (!pstats_dev.regs) {
		printk(KERN_ERR "%s: could not map PSTATS registers\n",
				KBUILD_MODNAME);
		unregister_sysctl_table(pstats_header);
		err = -ENOMEM;
		goto err_exit;
	}

	/*request pstats IRQ*/
	pstats_irq_disable(PSTATS_ALL_MSK);
	err = request_irq(WRVIC_BASE_IRQ+WR_PSTATS_IRQ, pstats_irq_handler,
			IRQF_SHARED, "wr_pstats", &pstats_dev);
	if (err) {
		printk(KERN_ERR "%s: cannot request interrupt\n",
				KBUILD_MODNAME);
		iounmap(pstats_dev.regs);
		unregister_sysctl_table(pstats_header);
		goto err_exit;
	}

	for (i = 0; i < PSTATS_NPORTS; ++i)
		spin_lock_init(&pstats_dev.port_mutex[i]);
	pstats_irq_enable(portmsk);

	wr_nic_pstats_callback = pstats_callback;

	printk(KERN_INFO "%s: initialized\n", KBUILD_MODNAME);
	return 0;

err_exit:
	printk(KERN_ERR "%s: could not initialize\n", KBUILD_MODNAME);
	return err;
}

static void __exit pstats_exit(void)
{
	pstats_irq_disable(PSTATS_ALL_MSK);
	free_irq(WRVIC_BASE_IRQ+WR_PSTATS_IRQ, &pstats_dev);

	wr_nic_pstats_callback = NULL;

	iounmap(pstats_dev.regs);
	unregister_sysctl_table(pstats_header);

	printk(KERN_INFO "%s: removed\n", KBUILD_MODNAME);
}

module_init(pstats_init);
module_exit(pstats_exit);

MODULE_DESCRIPTION("WRSW PSTATS counters reading module");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Grzegorz Daniluk");
