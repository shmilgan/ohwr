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
#include <linux/netdevice.h>

#include "../wbgen-regs/pstats-regs.h"
#include "wr_pstats.h"

#define pstats_readl(device, r)		__raw_readl(&device.regs->r)
#define pstats_writel(val, device, r)	__raw_writel(val, &device.regs->r)

static int pstats_nports = PSTATS_DEFAULT_NPORTS;
static uint32_t portmsk;
static unsigned int firmware_version; /* FPGA firmware version */
static unsigned int firmware_counters; /* number of counters */
static unsigned int firmware_adr_pp; /* number of words with counters */
static unsigned int firmware_cpw; /* number of counters per word */
module_param(pstats_nports, int, S_IRUGO);

const char *portnames[]  = {"port0", "port1", "port2", "port3", "port4",
	"port5", "port6", "port7", "port8", "port9", "port10", "port11",
	"port12", "port13", "port14", "port15", "port16", "port17"};

static struct pstats_version_description pstats_desc[] = {
	[0] = {
		.cnt_names = "Inv pstats ver reported by FPGA",
		.rx_packets = 0,
		.tx_packets = 0,
		.rx_errors = 0,
		.tx_carrier_errors = 0,
		.rx_length_errors = 0,
		.rx_crc_errors = 0,
		.rx_fifo_errors = 0,
		.tx_fifo_errors = 0
	},

	[1] = {
		.cnt_names = "TX Underrun\n"		/* 0 */
			"RX Overrun\n"			/* 1 */
			"RX Invalid Code\n"		/* 2 */
			"RX Sync Lost\n"		/* 3 */
			"RX Pause Frames\n"		/* 4 */
			"RX Pfilter Dropped\n"		/* 5 */
			"RX PCS Errors\n"		/* 6 */
			"RX Giant Frames\n"		/* 7 */
			"RX Runt Frames\n"		/* 8 */
			"RX CRC Errors\n"		/* 9 */
			"RX Pclass 0\n"			/* 10 */
			"RX Pclass 1\n"			/* 11 */
			"RX Pclass 2\n"			/* 12 */
			"RX Pclass 3\n"			/* 13 */
			"RX Pclass 4\n"			/* 14 */
			"RX Pclass 5\n"			/* 15 */
			"RX Pclass 6\n"			/* 16 */
			"RX Pclass 7\n"			/* 17 */
			"TX Frames\n"			/* 18 */
			"RX Frames\n"			/* 19 */
			"RX Drop RTU Full\n"		/* 20 */
			"RX PRIO 0\n"			/* 21 */
			"RX PRIO 1\n"			/* 22 */
			"RX PRIO 2\n"			/* 23 */
			"RX PRIO 3\n"			/* 24 */
			"RX PRIO 4\n"			/* 25 */
			"RX PRIO 5\n"			/* 26 */
			"RX PRIO 6\n"			/* 27 */
			"RX PRIO 7\n"			/* 28 */
			"RTU Valid\n"			/* 29 */
			"RTU Responses\n"		/* 30 */
			"RTU Dropped\n"			/* 31 */
			"FastMatch: Priority\n"		/* 32 */
			"FastMatch: FastForward\n"	/* 33 */
			"FastMatch: NonForward\n"	/* 34 */
			"FastMatch: Resp Valid\n"	/* 35 */
			"FullMatch: Resp Valid\n"	/* 36 */
			"Forwarded\n"			/* 37 */
			"TRU Resp Valid",		/* 38 */
		.rx_packets = 19, /* RX Frames */
		.tx_packets = 18, /* TX Frames */
		.rx_errors = 6, /* RX PCS Errors */
		.tx_carrier_errors = 3, /* RX Sync Lost */
		.rx_length_errors = 7, /* RX Giant Frames */
		.rx_crc_errors = 9, /* RX CRC Errors */
		.rx_fifo_errors = 1, /* RX Overrun */
		.tx_fifo_errors = 0 /* TX Underrun */
	}
};

struct cntrs_dev {
	uint64_t cntrs[PSTATS_MAX_NPORTS][PSTATS_MAX_NUM_OF_COUNTERS];
	/* there is no need to keep 64bits for zero values,
	 * part which is read from FPGA is enough */
	uint16_t zeros[PSTATS_MAX_NPORTS][PSTATS_MAX_NUM_OF_COUNTERS];
	struct PSTATS_WB __iomem *regs;

	/* prevents from simultaneous access to cntrs array from tasklet and
	 * sysfs handler */
	spinlock_t port_mutex[PSTATS_MAX_NPORTS];
	/* circular bufer for passing Port's IRQ mask between irq handler and
	 * the tasklet */
	uint32_t port_irqs[PSTATS_IRQBUFSZ];
	uint64_t overflows[PSTATS_IRQBUFSZ][PSTATS_MAX_NPORTS];
	volatile int irqs_head;
	int irqs_tail;
};

static struct cntrs_dev pstats_dev;	/*static data cleared at build time*/

/* pstats info for sysfs */
static int pstats_info[PINFO_SIZE];

/* function converting Layer1 and Layer2 words read from HW to counter value */
static unsigned int hwcnt_to_sw(uint32_t l1_val, uint32_t l2_val, int n)
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

static uint64_t pstats_irq_cntrs(int port)
{
	uint32_t val;
	uint64_t mask = 0;

	val = (port<<PSTATS_CR_PORT_SHIFT | PSTATS_CR_RD_IRQ);
	pstats_writel(val, pstats_dev, CR);
	/* read lower half of cntrs overflow mask */
	mask = (((uint64_t)pstats_readl(pstats_dev, L2_CNT_VAL)) << 32);
	mask |= (uint64_t) pstats_readl(pstats_dev, L1_CNT_VAL);
	return mask;
}

/* Tasklet function, takes irq status for all ports, gets counters' overflow
 * flags and increments appropriate counters*/
static void pstats_tlet_fn(unsigned long arg)
{
	uint32_t irqs;
	uint64_t *cntrs_ov;
	int port, cntr;
	uint64_t *ptr;
	struct cntrs_dev *device = (struct cntrs_dev *)arg;

	if (device->irqs_head - device->irqs_tail > PSTATS_IRQBUFSZ) {
		printk(KERN_WARNING "%s: overflow in port_irqs buffer\n",
				KBUILD_MODNAME);
		/*recover from error situation*/
		device->irqs_tail += PSTATS_IRQBUFSZ;
	}

	while (device->irqs_tail != device->irqs_head) {
		if (0)
			printk(KERN_WARNING "tlet head=%d, tail=%d\n",
			       device->irqs_head, device->irqs_tail);
		irqs = device->port_irqs[device->irqs_tail];

		/* level 1 of IRQs is device->port_irqs where each bit
		 * set to 1 says there is at least one overflowed
		 * counter on the port correcponding to the bit
		 */

		if (0)
			printk(KERN_WARNING "irqs %5x\n", irqs);
		for (port = 0; port < pstats_nports; ++port) {
			if (!(irqs>>port & 0x01))
				continue; /* there is no irq from this port*/

			/* Level 2 of IRQs is reading additional IRQ
			 * flags register for each port that has bit
			 * in device->port_irqs set to 1. This
			 * register is a set of IRQ flags per-counter
			 * for a given port.  That means, when i-th
			 * bit in cntrs_ov is set, i-th counter for
			 * this port has to be incremented by
			 * 1<<PSTATS_MSB_SHIFT
			 */
			spin_lock(&device->port_mutex[port]);
			cntrs_ov = &(device->overflows[device->irqs_tail][port]);
			//if(port==0)
			//	printk(KERN_WARNING "cntrs_ov: %08x %08x\n", (uint32_t)((*cntrs_ov)>>32 & 0xffffffff),
			//      (uint32_t)((*cntrs_ov) & 0x00ffffffffLL));
			for (cntr = 0; cntr < firmware_counters; ++cntr) {
				/*decode counters overflow flags to increment coutners*/
				if ((*cntrs_ov)>>cntr & 0x01) {
					ptr = &(device->cntrs[port][cntr]);
					*ptr += 1<<PSTATS_MSB_SHIFT;
				}
			}
			spin_unlock(&device->port_mutex[port]);
		}
		device->irqs_tail = (device->irqs_tail+1) % PSTATS_IRQBUFSZ;
	}
}
DECLARE_TASKLET(proc_ports, pstats_tlet_fn, (unsigned long)&pstats_dev);

static irqreturn_t pstats_irq_handler(int irq, void *devid)
{
	struct cntrs_dev *device = (struct cntrs_dev *)devid;
	uint32_t irqs, i;

	irqs = pstats_irq_status();

	pstats_irq_disable(PSTATS_ALL_MSK);
	pstats_irq_clear(irqs);
	device->port_irqs[device->irqs_head] = irqs;
	/* dump all overflow information so that we don't lose any if
	 * tasklet is delayed */
	for (i = 0; i < pstats_nports; ++i)
		device->overflows[device->irqs_head][i] = pstats_irq_cntrs(i);

	device->irqs_head = (device->irqs_head + 1) % PSTATS_IRQBUFSZ;
	//device->port_irqs[device->irqs_head++ % PSTATS_IRQBUFSZ] = irqs;
	tasklet_schedule(&proc_ports);
	pstats_irq_enable(portmsk);

	return IRQ_HANDLED;
}

static int rd_cnt_word(int port, int adr)
{
	uint32_t val[2];
	uint64_t *ptr;
	int i;

	val[0] = (adr<<PSTATS_CR_ADDR_SHIFT |
			port<<PSTATS_CR_PORT_SHIFT | PSTATS_CR_RD_EN);

	pstats_writel(val[0], pstats_dev, CR);

	val[0] = pstats_readl(pstats_dev, L1_CNT_VAL);
	val[1] = pstats_readl(pstats_dev, L2_CNT_VAL);

	for (i = 0; i <= 3; ++i) {
		if (4*adr+i >= firmware_counters)
			break;
		spin_lock(&pstats_dev.port_mutex[port]);
		ptr = &(pstats_dev.cntrs[port][4 * adr + i]);
		*ptr &= PSTATS_MSB_MSK;
		*ptr |= hwcnt_to_sw(val[0], val[1], i);
		/* apply zero bias,
		 * NOTE: since cntrs are monotonic, they're always bigger than
		 * zeros */
		*ptr -= pstats_dev.zeros[port][4 * adr + i];
		spin_unlock(&pstats_dev.port_mutex[port]);
	}

	return 0;
}

static int pstats_rd_cntrs(int port)
{
	int adr;

	for (adr = 0; adr < firmware_adr_pp; ++adr)
		rd_cnt_word(port, adr);

	return 0;
}

static void pstats_zero(int port)
{
	int i;
	pstats_rd_cntrs(port);
	spin_lock(&pstats_dev.port_mutex[port]);
	for (i = 0; i < PSTATS_MAX_NUM_OF_COUNTERS; i++) {
		/* since cntrs is monotonic it will never happen that zero will
		 * be bigger than cntrs */
		/* add zeros since it was substracted before in
		 * pstats_rd_cntrs (rd_cnt_word) */
		pstats_dev.cntrs[port][i] += pstats_dev.zeros[port][i];
		/* clear 48 MSBits */
		pstats_dev.cntrs[port][i] &= PSTATS_LSB_MSK;
		/* Copy 16 LSBits to zero */
		pstats_dev.zeros[port][i] =
			(uint16_t) pstats_dev.cntrs[port][i];
	}
	spin_unlock(&pstats_dev.port_mutex[port]);
}

/* SYSCTL handler, pass description of counters */
static int pstats_desc_handler(ctl_table *ctl, int write, void *buffer,
		size_t *lenp, loff_t *ppos)
{
	int port;

	if (write) { /* write description: zero all ports */
		for (port = 0; port < pstats_nports; port++)
			pstats_zero(port);
		return *lenp;
	}

	/* version number is always valid due to check at module load */
	ctl->data = (void *)pstats_desc[firmware_version].cnt_names;
	ctl->maxlen = strlen(pstats_desc[firmware_version].cnt_names);
	return proc_dostring(ctl, 0, buffer, lenp, ppos);
}


/* SYSCTL handler, reads counters from hw and passes to sysfs */
static int pstats_handler(ctl_table *ctl, int write, void *buffer,
		size_t *lenp, loff_t *ppos)
{
	int port;
	int ret;

	port = (int)ctl->extra1;

	if (write) {
		/* on write, we zero the counters */
		if (port < pstats_nports) {
			pstats_zero(port);
		} else { /* write to info: zero them all */
			for (port = 0; port < pstats_nports; port++)
				pstats_zero(port);
		}
		return *lenp;
	}

	if (port < pstats_nports) {
		/* read counters including correction of zeros offset */
		pstats_rd_cntrs(port);
	} else {
		/* stuff for info file, read at module load time */
		pstats_info[PINFO_VER] = firmware_version;
		pstats_info[PINFO_CNTPW] = firmware_cpw;
		pstats_info[PINFO_CNTPP] = firmware_counters;
	}
	/* It might happen that irq comes between reading MSB 32 bits and
	 * LSB 32 bits of particualr counter.
	 * In that case counter's value would be lower than previous one */
	spin_lock(&pstats_dev.port_mutex[port]);
	/* each value will be split into two unsigned longs,
	 * each counter has to be assembled in software reading pstats */
	ret = proc_dointvec(ctl, 0, buffer, lenp, ppos);
	spin_unlock(&pstats_dev.port_mutex[port]);
	return ret;
}

/* one per port, then info and description, and terminator, filled at init time */
static ctl_table pstats_ctl_table[PSTATS_MAX_NPORTS + 3];

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
int pstats_callback(int epnum, struct net_device_stats *stats)
{
	unsigned int index;

	pstats_rd_cntrs(epnum);
	/* version number is always valid due to check at module load */
	index = pstats_desc[firmware_version].rx_packets;
	stats->rx_packets = (unsigned long) pstats_dev.cntrs[epnum][index];
	index = pstats_desc[firmware_version].tx_packets;
	stats->tx_packets = (unsigned long) pstats_dev.cntrs[epnum][index];
	index = pstats_desc[firmware_version].rx_errors;
	stats->rx_errors = (unsigned long) pstats_dev.cntrs[epnum][index];
	index = pstats_desc[firmware_version].tx_carrier_errors;
	stats->tx_carrier_errors =
				(unsigned long) pstats_dev.cntrs[epnum][index];
	index = pstats_desc[firmware_version].rx_length_errors;
	stats->rx_length_errors = (unsigned long)pstats_dev.cntrs[epnum][index];
	index = pstats_desc[firmware_version].rx_crc_errors;
	stats->rx_crc_errors = (unsigned long)pstats_dev.cntrs[epnum][index];
	index = pstats_desc[firmware_version].rx_fifo_errors;
	stats->rx_fifo_errors = (unsigned long)pstats_dev.cntrs[epnum][index];
	index = pstats_desc[firmware_version].tx_fifo_errors;
	stats->tx_fifo_errors = (unsigned long)pstats_dev.cntrs[epnum][index];

	return 0;
}

static struct ctl_table_header *pstats_header;

static int __init pstats_init(void)
{
	int i, err = 0;
	unsigned int data;

	if (pstats_nports > PSTATS_MAX_NPORTS) {
		printk(KERN_ERR "%s: Too many ports for pstats %u,"
		       "only %d supported\n", KBUILD_MODNAME, pstats_nports,
		       PSTATS_MAX_NPORTS);
		err = -EFBIG; /* "File too large", not exact */
		goto err_exit;
	}
	/*convert nports to one-hot port mask (for enabling IRQs*/
	printk(KERN_INFO "nports=%u\n", pstats_nports);
	portmsk = (1 << pstats_nports) - 1;

	printk(KERN_INFO "PORTMSK=%x\n", portmsk);

	/*map PSTATS Wishbone registers*/
	pstats_dev.regs = ioremap(FPGA_BASE_PSTATS, sizeof(struct PSTATS_WB));
	if (!pstats_dev.regs) {
		printk(KERN_ERR "%s: could not map PSTATS registers\n",
				KBUILD_MODNAME);
		unregister_sysctl_table(pstats_header);
		err = -ENOMEM;
		goto err_exit;
	}

	/* get version number */
	data = pstats_readl(pstats_dev, INFO);
	firmware_version = PSTATS_INFO_VER_R(data);
	firmware_counters = PSTATS_INFO_CPP_R(data);
	firmware_cpw = PSTATS_INFO_CPW_R(data);
	/* assume 4 counters per word */
	firmware_adr_pp = (firmware_counters+3)/4;

	if (firmware_version >= ARRAY_SIZE(pstats_desc)) {
		printk(KERN_ERR "%s: pstats version %d not supported\n",
		       KBUILD_MODNAME, firmware_version);
		err = -EFBIG; /* "File too large", not exact */
		goto err_exit;
	}

	if (firmware_counters > PSTATS_MAX_NUM_OF_COUNTERS) {
		printk(KERN_ERR "%s: too many counters %d, "
				"maximum supported %d\n",
		       KBUILD_MODNAME, firmware_counters,
		       PSTATS_MAX_NUM_OF_COUNTERS);
		err = -EFBIG; /* "File too large", not exact */
		goto err_exit;
	}

	for (i = 0; i < pstats_nports; ++i) {
		pstats_ctl_table[i].procname = portnames[i];
		pstats_ctl_table[i].data = &pstats_dev.cntrs[i];
		/* each value will be split into two unsigned longs,
		 * each counter has to be assembled in software reading pstats
		 */
		pstats_ctl_table[i].maxlen =
			firmware_counters*sizeof(uint64_t);
		pstats_ctl_table[i].mode = 0644;
		pstats_ctl_table[i].proc_handler = pstats_handler;
		pstats_ctl_table[i].extra1 = (void *)i;
	}
	/* the last-but-one with info about pstats */
	pstats_ctl_table[i].procname = "info";
	pstats_ctl_table[i].data = pstats_info;
	pstats_ctl_table[i].maxlen = PINFO_SIZE * sizeof(int);
	pstats_ctl_table[i].mode = 0644;
	pstats_ctl_table[i].proc_handler = pstats_handler;
	pstats_ctl_table[i].extra1 = (void *)i;

	i++;
	/* fill data and maxlen at open time, so we can replace FPGA
	 * without reloading kernel module */
	pstats_ctl_table[i].procname = "description";
	pstats_ctl_table[i].data = NULL;
	pstats_ctl_table[i].maxlen = 0;
	pstats_ctl_table[i].mode = 0644;
	pstats_ctl_table[i].proc_handler = pstats_desc_handler;
	pstats_ctl_table[i].extra1 = (void *)i;

	pstats_header = register_sysctl_table(proc_table);

	if (!pstats_header) {
		err = -EBUSY;
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

	for (i = 0; i < PSTATS_MAX_NPORTS; ++i)
		spin_lock_init(&pstats_dev.port_mutex[i]);
	pstats_irq_enable(portmsk);

	wr_nic_pstats_callback = pstats_callback;

	printk(KERN_INFO "%s: initialized\n", KBUILD_MODNAME);
	return 0;

err_exit:
	if (pstats_dev.regs)
		iounmap(pstats_dev.regs);
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
