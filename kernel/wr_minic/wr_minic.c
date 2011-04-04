/* 
 * Copyright (c) 2010 Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Copyright (c) 2009 Emilio G. Cota <cota@braap.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/etherdevice.h>
#include <linux/net_tstamp.h>
#include <linux/interrupt.h>
#include <linux/netdevice.h>
#include <linux/ethtool.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/mii.h>
#include <linux/swab.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/if.h>

#include <asm/atomic.h>

#include "minic_regs.h"
#include "endpoint_regs.h"
#include "pps_gen_regs.h"

#ifndef WRVIC_BASE_IRQ
#define WRVIC_BASE_IRQ (NR_AIC_IRQS + (5 * 32)) /* top of GPIO interrupts */
#endif


#define DMTD_AVG_SAMPLES 256
#define DMTD_MAX_PHASE 16384

#define F_COUNTER_BITS 4
#define F_COUNTER_MASK ((1<<F_COUNTER_BITS)-1)

#define LINK_POLL_INTERVAL (HZ/5)

#define DRV_NAME		"wr_minic"
#define DRV_VERSION             "0.5.1"

#define MINIC_NAPI_WEIGHT	10
#define MINIC_MTU 1540

#define MINIC_TX_MAX_TS 64


#define RX_DESC_VALID(d) ((d) & (1<<31) ? 1 : 0)
#define RX_DESC_ERROR(d) ((d) & (1<<30) ? 1 : 0)
#define RX_DESC_HAS_OOB(d)  ((d) & (1<<29) ? 1 : 0)
#define RX_DESC_SIZE(d)  (((d) & (1<<0) ? -1 : 0) + (d & 0xfffe))

#define TX_DESC_VALID (1<<31)
#define TX_DESC_WITH_OOB (1<<30)
#define TX_DESC_HAS_OWN_MAC (1<<28)

#define RX_OOB_SIZE 6
#define REFCLK_FREQ 125000000

/*
 * Extracts the values of TS rising and falling edge counters
 * from the descriptor header
 */
#define EXPLODE_WR_TIMESTAMP(raw, rc, fc) \
  rc = (raw) & 0xfffffff;		  \
  fc = (raw >> 28) & 0xf;

/* UGLY HACK WARNING:
   remove the PPS subsystem and put it in a separate driver */

struct tx_timestamp {
	u16 fid;
	u16 port;
	struct skb_shared_hwtstamps ts_val;
	int valid;
};

struct wr_minic {
	void __iomem *base;         // address of the Minic+RAM+Endpoint combo
	void __iomem *minic_regs;   // address of the miNIC registers
	void __iomem *ep_regs;      // address of the Endpoint registers
	void __iomem *pbuf;         // address of the Packet RAM
	void __iomem *ppsg;         // address of the PPS generator

	spinlock_t		lock;

	int tx_hwtstamp_enable;
	int rx_hwtstamp_enable;

	u16 tx_hwtstamp_oob;

	struct tx_timestamp tx_tstable[MINIC_TX_MAX_TS];

	struct platform_device	*pdev;
	struct device		*dev;
	struct net_device	*netdev;
	struct net_device_stats	stats;
	struct napi_struct	napi;
	struct sk_buff		*current_skb;
	struct timer_list	link_timer;
 
	unsigned int rx_head, rx_avail, rx_base, rx_size;
	unsigned int tx_head, tx_avail, tx_base, tx_size;

	bool synced;
	bool syncing_counters;
	int iface_up;

	u32 cur_rx_desc;

	struct mii_if_info mii;
};

MODULE_DESCRIPTION("White Rabbit miNIC driver");
MODULE_AUTHOR("Tomasz Wlostowski <tomasz.wlostowsk@cern.ch>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:wr-minic");

#define minic_readl(m, offs)			\
  __raw_readl((m)->minic_regs + (offs))

#define minic_writel(m, offs, value)			\
  __raw_writel((value), (m)->minic_regs + (offs))

#define endpoint_readl(m, offs)			\
  __raw_readl((m)->ep_regs + (offs))

#define endpoint_writel(m, offs, value)			\
  __raw_writel((value), (m)->ep_regs + (offs))

#define ppsg_readl(m, offs)			\
  __raw_readl((m)->ppsg + (offs))

#define ppsg_writel(m, offs, value)			\
  __raw_writel((value), (m)->ppsg + (offs))


// reads an MDIO register
static int phy_read(struct net_device *dev, int phy_id, int location)
{
	struct wr_minic *nic = netdev_priv(dev);

	endpoint_writel(nic, EP_REG_MDIO_CR, EP_MDIO_CR_ADDR_W(location));
	while( (endpoint_readl(nic, EP_REG_MDIO_SR) & EP_MDIO_SR_READY) == 0)
		;
	return EP_MDIO_SR_RDATA_R(endpoint_readl(nic, EP_REG_MDIO_SR));
}

static void phy_write(struct net_device *dev, int phy_id, int location,
		      int value)
{
	struct wr_minic *nic = netdev_priv(dev);

	endpoint_writel(nic, EP_REG_MDIO_CR,
			EP_MDIO_CR_ADDR_W(location)
			| EP_MDIO_CR_DATA_W(value)
			| EP_MDIO_CR_RW);
	while( (endpoint_readl(nic, EP_REG_MDIO_SR) & EP_MDIO_SR_READY) == 0)
		;
}


static int minic_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	struct wr_minic *lp = netdev_priv(dev);
	int ret;

	spin_lock_irq(&lp->lock);
	ret = mii_ethtool_gset(&lp->mii, cmd);
	spin_unlock_irq(&lp->lock);

	cmd->supported= SUPPORTED_FIBRE | SUPPORTED_Autoneg
		| SUPPORTED_1000baseKX_Full;
	cmd->advertising = ADVERTISED_1000baseKX_Full | ADVERTISED_Autoneg;
	cmd->port = PORT_FIBRE;
	cmd->speed = SPEED_1000;
	cmd->duplex = DUPLEX_FULL;
	cmd->autoneg = AUTONEG_ENABLE;

	return ret;
}

static int minic_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	struct wr_minic *lp = netdev_priv(dev);
	int ret;

	spin_lock_irq(&lp->lock);
	ret = mii_ethtool_sset(&lp->mii, cmd);
	spin_unlock_irq(&lp->lock);

	return ret;
}


static int minic_nwayreset(struct net_device *dev)
{
	struct wr_minic *lp = netdev_priv(dev);
	int ret;

	spin_lock_irq(&lp->lock);
	ret = mii_nway_restart(&lp->mii);
	spin_unlock_irq(&lp->lock);

	return ret;
}

static void minic_get_drvinfo(struct net_device *dev,
			      struct ethtool_drvinfo *info)
{
	strlcpy(info->driver, DRV_NAME, sizeof(info->driver));
	strlcpy(info->version, DRV_VERSION, sizeof(info->version));
	strlcpy(info->bus_info, dev_name(dev->dev.parent),
		sizeof(info->bus_info));
}

static const struct ethtool_ops minic_ethtool_ops = {
	.get_settings	= minic_get_settings,
	.set_settings	= minic_set_settings,
	.get_drvinfo	= minic_get_drvinfo,
	.nway_reset	= minic_nwayreset,
	.get_link	= ethtool_op_get_link,
};


// use in locked context, please
static void minic_ppsg_read_time(struct wr_minic *nic, u32 *cntr, u64 *utc)
{
	uint32_t cyc_before, cyc_after;
	uint32_t utc_lo, utc_hi;

	for(;;) {
		cyc_before =ppsg_readl(nic, PPSG_REG_CNTR_NSEC) & 0xfffffff;
		utc_lo = ppsg_readl(nic, PPSG_REG_CNTR_UTCLO) ;
		utc_hi = ppsg_readl(nic, PPSG_REG_CNTR_UTCHI) & 0xff;
		cyc_after = ppsg_readl(nic, PPSG_REG_CNTR_NSEC) & 0xfffffff;

		// there was an UTC transition. (nanosecond counter overflow),
		// read the value again.
		if(cyc_after < REFCLK_FREQ/4
		   && cyc_before > (REFCLK_FREQ - REFCLK_FREQ/4)) {
			continue;
		} else {
			if(utc) *utc = (u64)utc_lo | (u64)utc_hi  << 32;
			if(cntr) *cntr = cyc_after;
			return;
		}
	}
}


static inline u32 minic_ppsg_get_nsecs(struct wr_minic *nic)
{
	return ppsg_readl(nic, PPSG_REG_CNTR_NSEC) & 0xfffffff;
}


static void minic_disable_irq(struct wr_minic *nic, u32 mask)
{
	//  dev_dbg(nic->dev, "wr_disable_irq() - mask %x\n", mask);
	minic_writel(nic, MINIC_REG_EIC_IDR, mask);
}

static void minic_enable_irq(struct wr_minic *nic, u32 mask)
{
	//  dev_dbg(nic->dev, "wr_enable_irq() - mask %x\n", mask);
	minic_writel(nic, MINIC_REG_EIC_IER, mask);
}

static void minic_clear_irq(struct wr_minic *nic, u32 mask)
{
	minic_writel(nic, MINIC_REG_EIC_ISR, mask);
}

static struct net_device_stats *minic_get_stats(struct net_device *netdev)
{
	struct wr_minic *nic = netdev_priv(netdev);

	//	wr_update_tx_stats(nic);
	//	wr_update_rx_stats(nic);
	return &nic->stats;
}

static void minic_new_rx_buffer(struct wr_minic *nic)
{
	nic->rx_head = nic->rx_base;

	minic_writel(nic,  MINIC_REG_MCR, 0);
	minic_writel(nic,  MINIC_REG_RX_ADDR, nic->rx_base);
	minic_writel(nic,  MINIC_REG_RX_AVAIL, (nic->rx_size - MINIC_MTU) >> 2);
	minic_writel(nic,  MINIC_REG_MCR, MINIC_MCR_RX_EN);
}


static void minic_new_tx_buffer(struct wr_minic *nic)
{
	nic->tx_head = nic->tx_base;
	nic->tx_avail = (nic->tx_size - MINIC_MTU) >> 2;
	minic_writel(nic, MINIC_REG_TX_ADDR, nic->tx_base);
}

static int minic_rx_frame(struct wr_minic *nic)
{
	struct net_device	*netdev = nic->netdev;
	struct sk_buff		*skb;
	u32 __iomem *rx_head;
	u32 *tmp_ptr;
	u32 payload_size, num_words;
	u32 desc_hdr;
	u32 rx_addr_cur;
	u32 rx_oob;
	u32 tmp_buf[(MINIC_MTU >> 2) + 8];
	int i;

	/* get the address of the latest RX descriptor */
	rx_addr_cur = minic_readl(nic, MINIC_REG_RX_ADDR) & 0xffffff;

	if(rx_addr_cur < nic->rx_head)  /* nothing new in the buffer? */
		return 1;

	rx_head = nic->pbuf + nic->rx_head;
	desc_hdr = __raw_readl(rx_head++); /* read 32-bit descriptor header */


	if(!RX_DESC_VALID(desc_hdr)) {
		/*
		 * invalid descriptor? Weird, the RX_ADDR seems
		 * to be saying something different. Ignore the packet
		 * and purge the RX buffer.
		 */
		dev_info(nic->dev, "%s: weird, invalid RX descriptor "
			 "(%x, head %x)", __func__, desc_hdr,
			 (unsigned int) rx_head-1);
		minic_new_rx_buffer(nic);
		return 0;
	}

//  if(RX_DESC_HAS_OOB(desc_hdr))

	payload_size = RX_DESC_SIZE(desc_hdr);

	num_words = (payload_size + 3) >> 2;

	/* valid packet */
	if(!RX_DESC_ERROR(desc_hdr)) {
		skb = netdev_alloc_skb(netdev, payload_size + 9);
		if (unlikely(skb == NULL)) {
			if (net_ratelimit())
				dev_warn(nic->dev, "-ENOMEM - pckt dropped\n");
			return 0;
		}

		/* Make the IP header aligned (the eth header is 14 bytes) */
		skb_reserve(skb, 2);

		for(i=num_words, tmp_ptr = tmp_buf;i >=0 ; i--)
			*tmp_ptr ++ = __raw_readl(rx_head++);

		if(RX_DESC_HAS_OOB(desc_hdr)) // RX timestamping
		{
			struct skb_shared_hwtstamps *hwts = skb_hwtstamps(skb);
			u32 counter_r, counter_f; // timestamp counter values
			u32 counter_ppsg; // PPS generator nanosecond counter
			u64 utc;
			s32 cntr_diff;

			payload_size -= RX_OOB_SIZE;

			memcpy(&rx_oob, ((void *)tmp_buf) + payload_size+2, 4);
			rx_oob = swab32(rx_oob);

			EXPLODE_WR_TIMESTAMP(rx_oob, counter_r, counter_f);

			minic_ppsg_read_time(nic, &counter_ppsg, &utc);

			if(counter_r > (3*REFCLK_FREQ/4)
			   && counter_ppsg < REFCLK_FREQ/4)
				utc--;

			/* fixme: we need to pass the phase value somehow
			 * for RX timestamps. For the time being, we pass
			 * the R-F counter difference on the MSB of UTC
			 * (instead of sign value), so the PTP can detect
			 * the valid counter
			 */

			hwts->hwtstamp.tv.sec = (s32)utc & 0x7fffffff;

			cntr_diff = (counter_r & F_COUNTER_MASK) - counter_f;

			/* the bit says the rising edge cnter is 1tick ahead */
			if(cntr_diff == 1 || cntr_diff == (-F_COUNTER_MASK))
				hwts->hwtstamp.tv.sec |= 0x80000000;

			hwts->hwtstamp.tv.nsec = counter_r * 8;
		}

		memcpy(skb_put(skb, payload_size), tmp_buf, payload_size);

		/* determine protocol id */
		skb->protocol = eth_type_trans(skb, netdev);

		/* @fixme ignore the checksum for the time being */
		skb->ip_summed = CHECKSUM_UNNECESSARY;

		/* update the rx buffer head to point to the next descriptor */
		nic->rx_head += ( num_words + 1 ) << 2;

		netdev->last_rx = jiffies;
		nic->stats.rx_packets++;
		nic->stats.rx_bytes += payload_size;
		netif_receive_skb(skb);
	} else { // RX_DESC_ERROR
		nic->stats.rx_errors ++;
		nic->rx_head += ( num_words + 1 ) << 2;
	}
	wmb();
	return 0;
}

static int minic_poll_txts_fifo(struct wr_minic *nic, u16 oob_fid,
				struct skb_shared_hwtstamps *hwts)
{
	int i;

	u32 dmsr;
	int dmtd_phase;

	dmsr = endpoint_readl(nic, EP_REG_DMSR);

	if(dmsr & EP_DMSR_PS_RDY)
		dmtd_phase = EP_DMSR_PS_VAL_R(dmsr);
	else
		dmtd_phase = 0;

	// sign-extend, fix the average if its out of range due to jitter
	if(dmtd_phase & 0x800000) dmtd_phase |= 0xff000000;

	// calculate the average
	dmtd_phase /= DMTD_AVG_SAMPLES;

	if(dmtd_phase > DMTD_MAX_PHASE) dmtd_phase -= DMTD_MAX_PHASE;
	if(dmtd_phase < 0) dmtd_phase += DMTD_MAX_PHASE;


	while(!(minic_readl(nic, MINIC_REG_TSFIFO_CSR)
		& MINIC_TSFIFO_CSR_EMPTY)) {
		u32 tsval_raw = minic_readl(nic, MINIC_REG_TSFIFO_R0);
		u32 fid = (minic_readl(nic, MINIC_REG_TSFIFO_R1) >> 5) & 0xffff;

		u32 counter_r, counter_f; // timestamp counter values
		u32 counter_ppsg; // PPS generator nanosecond counter
		u64 utc;
		struct skb_shared_hwtstamps tsval;

		EXPLODE_WR_TIMESTAMP(tsval_raw, counter_r, counter_f);

		//   printk("About to read time\n");

		minic_ppsg_read_time(nic, &counter_ppsg, &utc);

		//    printk("After read time\n");

		// the timestamp was taken at the end of previous second
		// of UTC time, and now we are at the beg. of the next second
		if(counter_r > (3*REFCLK_FREQ/4)
		   && counter_ppsg < REFCLK_FREQ/4)
			utc--;

		// fixme
		tsval.hwtstamp.tv.sec = ((s32)utc & 0x7fffffff);
		tsval.hwtstamp.tv.nsec = counter_r * 8;

		for(i = 0; i<MINIC_TX_MAX_TS;i++)
			if(!nic->tx_tstable[i].valid) {
				// printk("Addts: fid %d tsval %x\n",
				// fid, tsval);
				nic->tx_tstable[i].valid = 1;
				nic->tx_tstable[i].ts_val = tsval;
				nic->tx_tstable[i].fid = fid;
				break;
			}
	}

	//  printk("queryts: fid %d\n", oob_fid);

	for(i = 0; i<MINIC_TX_MAX_TS;i++) {
		if(nic->tx_tstable[i].valid && oob_fid
		   == nic->tx_tstable[i].fid) {
			// printk("GotTS: fid %d\n", oob_fid);
			if(hwts)
				memcpy(hwts, &nic->tx_tstable[i].ts_val,
				       sizeof(struct skb_shared_hwtstamps));
			nic->tx_tstable[i].valid = 0;
			return 0;
		}
	}
	//  printk("Missed timestamp...");
	return -1;
}

static inline void minic_tx_handle_irq(struct wr_minic *nic)
{
	struct net_device *netdev = nic->netdev;

	struct skb_shared_hwtstamps hwts;
	struct skb_shared_hwtstamps *hwoob = skb_hwtstamps(nic->current_skb);
	union skb_shared_tx *shtx = skb_tx(nic->current_skb);

	unsigned long flags;
	u16 oob_tag;

	spin_lock_irqsave(&nic->lock, flags);

	/*
	 * this will only work for the NIC directly connected to the endpoint.
	 * In case of a switch, the packet will reach the output port after
	 * being completely transmitted by the NIC (i.e. after TX interrupt)
	 */

	if(shtx->in_progress) {
		oob_tag = *(u16 *) hwoob;
		if(!minic_poll_txts_fifo(nic, oob_tag, &hwts))
			skb_tstamp_tx(nic->current_skb, &hwts);
	}

	dev_kfree_skb_irq(nic->current_skb);

	if (netif_queue_stopped(nic->netdev))
		netif_wake_queue(netdev);

	spin_unlock_irqrestore(&nic->lock, flags);

	minic_clear_irq(nic, MINIC_EIC_ISR_TX); // clear the TX interrupt
}

static inline void minic_rx_handle_irq(struct wr_minic *nic)
{
	int buf_full;

	buf_full = minic_readl(nic, MINIC_REG_MCR) & MINIC_MCR_RX_FULL ? 1 : 0;

	while(!minic_rx_frame(nic));

	if(buf_full)
		minic_new_rx_buffer(nic);

	minic_clear_irq(nic, MINIC_EIC_ISR_RX);
}

/* called by WRVIC driver */
static irqreturn_t minic_interrupt(int irq, void *dev_id)
{
	struct net_device *netdev = dev_id;
	struct wr_minic *nic = netdev_priv(netdev);
	u32 isr;

	isr = minic_readl(nic, MINIC_REG_EIC_ISR);

	if (isr & MINIC_EIC_ISR_TX)
		minic_tx_handle_irq(nic);

	if (isr & MINIC_EIC_ISR_RX)
		minic_rx_handle_irq(nic);
	return IRQ_HANDLED;
}

static int minic_hw_tx(struct wr_minic *nic, char *data, unsigned size,
		       u16 tx_oob_val)
{
	u32 __iomem *dst, *dptr;
	u32 nwords;
	u32 mcr;
	int i;
	u32 pkt_buf[(MINIC_MTU >> 2) + 4];

	nwords = ((size + 1) >> 1) - 1;

	memset(pkt_buf, 0x0, size + 16);
	memcpy(pkt_buf + 1, data, size);

	if(nwords < 30) nwords = 30; // min length = 60 bytes (CRC excluded)

	if(tx_oob_val) { // do the TX timestamping?
		tx_oob_val = swab16(tx_oob_val);
		memcpy((void*)pkt_buf + 4 + size, &tx_oob_val, sizeof(u16));
		nwords++;
		pkt_buf[0] = TX_DESC_WITH_OOB;
		//	 printk("add oob\n");
	} else {
		pkt_buf[0] = 0;
	}

	pkt_buf[0] |= TX_DESC_VALID | TX_DESC_HAS_OWN_MAC | nwords;

	dptr = (u32*) pkt_buf;
	dst = nic->pbuf + nic->tx_head;
	for(i = ((nwords + 1) >> 1) + 3; i; i--)
		*dst++ = *dptr++;

	minic_enable_irq(nic, MINIC_EIC_IER_TX);

	mcr = minic_readl(nic, MINIC_REG_MCR);
	minic_writel(nic, MINIC_REG_MCR, mcr | MINIC_MCR_TX_START);

	return 0;
}

static int minic_start_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	struct wr_minic *nic = netdev_priv(netdev);
	union skb_shared_tx *shtx = skb_tx(skb);
	u16 tx_oob = 0;

	char *data;
	unsigned len;

	if (unlikely(skb->len > MINIC_MTU)) {
		nic->stats.tx_errors++;
		return -EMSGSIZE;
	}

	data = skb->data;
	len = skb->len;

	spin_lock_irq(&nic->lock);

	nic->current_skb = skb;
	netif_stop_queue(netdev); // queue is stopped until packet is tx'ed

	if(nic -> tx_hwtstamp_enable && shtx->hardware) {
		struct skb_shared_hwtstamps *hwts = skb_hwtstamps(skb);
		shtx->in_progress = 1;
		*(u16 *) hwts = tx_oob = nic->tx_hwtstamp_oob;

		nic->tx_hwtstamp_oob ++;
		if(nic->tx_hwtstamp_oob == 60000)
			nic->tx_hwtstamp_oob = 1;
	} else {
		tx_oob = 0;
	}

	minic_new_tx_buffer(nic);
	minic_hw_tx(nic, data, len, tx_oob);

	nic->stats.tx_packets++;
	nic->stats.tx_bytes += len;

	netdev->trans_start = jiffies;

	spin_unlock_irq(&nic->lock);

	return 0;
}

static void minic_update_ts_config(struct wr_minic *nic)
{
	endpoint_writel(nic, EP_REG_TSCR,
			(nic->tx_hwtstamp_enable ? EP_TSCR_EN_TXTS : 0)
			| (nic->rx_hwtstamp_enable ? EP_TSCR_EN_RXTS : 0)
		);

	//  printk("update_ts_config: TSCR %x\n",
	// endpoint_readl(nic, EP_REG_TSCR));
}

static int minic_tstamp_ioctl(struct net_device *netdev, struct ifreq *rq,
			      int cmd)
{
	struct wr_minic *nic = netdev_priv(netdev);
	struct hwtstamp_config config;

	if (copy_from_user(&config, rq->ifr_data, sizeof(config)))
		return -EFAULT;

	//	printk("hwtstamp_ioctl()\n" );

	switch (config.tx_type) {
	case HWTSTAMP_TX_ON:
	  //	printk("%s: hw tx timestamping ON\n", __func__);
		nic->tx_hwtstamp_enable = 1;

		memset(nic->tx_tstable, 0, sizeof(nic->tx_tstable));

		break;
	case HWTSTAMP_TX_OFF:
	  //	printk("%s: hw tx timestamping OFF\n", __func__);

		nic->tx_hwtstamp_enable = 0;
		break;

	default:
		return -ERANGE;
	}

	/*
	 * For the time being, make this really simple and stupid: either
	 * time-tag _all_ the incoming packets or none of them.
	 */
	switch (config.rx_filter) {
	case HWTSTAMP_FILTER_NONE:
	  //dev_dbg(nic->dev, "%s - hw rx timestamping OFF\n", __func__);
		nic->rx_hwtstamp_enable = 0;
		break;

	case HWTSTAMP_FILTER_PTP_V2_L2_EVENT:
	  //	printk( "%s - hw rx timestamping ON for PTP L2 events\n",
		// __func__);
		nic->rx_hwtstamp_enable = 1; // Only PTPv2 supported by now
		config.rx_filter = HWTSTAMP_FILTER_ALL;
		break;
	default:
		return -ERANGE;
	}

	minic_update_ts_config(nic);

	return copy_to_user(rq->ifr_data, &config, sizeof(config)) ?
		-EFAULT : 0;
}

#define PRIV_IOCGCALIBRATE (SIOCDEVPRIVATE+1)
#define PRIV_IOCGGETPHASE (SIOCDEVPRIVATE+2)

#define CAL_CMD_TX_ON 1
#define CAL_CMD_TX_OFF 2
#define CAL_CMD_RX_ON 3
#define CAL_CMD_RX_OFF 4
#define CAL_CMD_RX_CHECK 5


struct wrmch_calibration_req {
	int cmd;
	//  int tx_rx;
	int cal_present;
};

struct wrmch_phase_req {
	int ready;
	u32 phase;
};

static int phase_ioctl(struct net_device *netdev, struct ifreq *rq, int cmd)
{
	struct wrmch_phase_req phase_req;
	struct wr_minic *nic = netdev_priv(netdev);

	u32 dmsr = endpoint_readl(nic, EP_REG_DMSR);

	if(dmsr & EP_DMSR_PS_RDY) {
		s32 dmtd_phase = EP_DMSR_PS_VAL_R(dmsr);

		// sign-extend, fix the average if out of range due to jitter
		if(dmtd_phase & 0x800000) dmtd_phase |= 0xff000000;

		// calculate the average
		dmtd_phase /= DMTD_AVG_SAMPLES;

		if(dmtd_phase > DMTD_MAX_PHASE) dmtd_phase -= DMTD_MAX_PHASE;
		if(dmtd_phase < 0) dmtd_phase += DMTD_MAX_PHASE;

		phase_req.phase = dmtd_phase;
		phase_req.ready = 1;
	} else {
		phase_req.phase = 0;
		phase_req.ready = 0;
	}

	return copy_to_user(rq->ifr_data, &phase_req, sizeof(phase_req)) ?
		-EFAULT : 0;
}

static int calibration_ioctl(struct net_device *netdev, struct ifreq *rq,
			     int cmd)
{
	struct wrmch_calibration_req cal_req;
	struct wr_minic *nic = netdev_priv(netdev);
	u32 tmp;

	if (copy_from_user(&cal_req, rq->ifr_data, sizeof(cal_req)))
		return -EFAULT;

	switch(cal_req.cmd) {
	case CAL_CMD_TX_ON:
			tmp = phy_read(netdev, 0, MDIO_REG_WR_SPEC);
			phy_write(netdev, 0, MDIO_REG_WR_SPEC,
				  tmp | MDIO_WR_SPEC_TX_CAL);
		break;

	case CAL_CMD_TX_OFF:
			tmp = phy_read(netdev, 0, MDIO_REG_WR_SPEC);
			phy_write(netdev, 0, MDIO_REG_WR_SPEC,
				  tmp & (~MDIO_WR_SPEC_TX_CAL));
		break;

	case CAL_CMD_RX_ON:
		if(nic->iface_up) {
			tmp = phy_read(netdev, 0, MDIO_REG_WR_SPEC);
			phy_write(netdev, 0, MDIO_REG_WR_SPEC,
				  tmp | MDIO_WR_SPEC_CAL_CRST);
		} else {
			return -EFAULT;
		}
		break;

	case CAL_CMD_RX_OFF:
		if(nic->iface_up) {
			// do nothing.....
		} else {
			return -EFAULT;
		}
		break;

	case CAL_CMD_RX_CHECK:
		if(nic->iface_up) {
			tmp = phy_read(netdev, 0, MDIO_REG_WR_SPEC);

			cal_req.cal_present = tmp & MDIO_WR_SPEC_RX_CAL_STAT
				? 1 : 0;

			if (copy_to_user(rq->ifr_data,&cal_req,
					 sizeof(cal_req)))
				return -EFAULT;
			return 0;
		} else {
			return -EFAULT;
		}
		break;
	}

	return 0;
}

static int minic_ioctl(struct net_device *netdev, struct ifreq *rq, int cmd)
{
	struct wr_minic *nic = netdev_priv(netdev);
	int res;
	switch (cmd) {
	case SIOCSHWTSTAMP:
		return minic_tstamp_ioctl(netdev, rq, cmd);
	case PRIV_IOCGCALIBRATE:
		return calibration_ioctl(netdev, rq, cmd);
	case PRIV_IOCGGETPHASE:
		return phase_ioctl(netdev, rq, cmd);
	default:
		spin_lock_irq(&nic->lock);
		res = generic_mii_ioctl(&nic->mii, if_mii(rq), cmd, NULL);
		spin_unlock_irq(&nic->lock);
		return res;
	}
}

static void ep_enable(struct net_device *netdev)
{
	struct wr_minic *nic = netdev_priv(netdev);

	endpoint_writel(nic, EP_REG_DMCR,
			EP_DMCR_EN | EP_DMCR_N_AVG_W(DMTD_AVG_SAMPLES));
	endpoint_writel(nic, EP_REG_ECR,
			EP_ECR_TX_EN_FRA | EP_ECR_RX_EN_FRA | EP_ECR_RST_CNT);
	endpoint_writel(nic, EP_REG_RFCR,
			3 << EP_RFCR_QMODE_SHIFT); // QMODE = UNQUALIFIED
	endpoint_writel(nic, EP_REG_TSCR, 0); // disable the timestamping
	endpoint_writel(nic, EP_REG_FCR, 0); // no flow control by now
	phy_write(netdev, 0, MII_ADVERTISE, 0x01a0); // adv. TX+RX flow, full
	phy_write(netdev, 0, MII_BMCR, 0);
	//phy_read(netdev, 0, MII_BMCR) | BMCR_ANENABLE | BMCR_ANRESTART);
}

static void ep_disable(struct net_device *netdev)
{
	struct wr_minic *nic = netdev_priv(netdev);

	endpoint_writel(nic, EP_REG_ECR, 0);
	endpoint_writel(nic, EP_REG_TSCR, 0);
}

static void update_link_status(unsigned long dev_id)
{
	struct net_device *netdev = (struct net_device *) dev_id;
	struct wr_minic *nic = netdev_priv(netdev);
	u32 ecr, bmsr, bmcr, lpa;

	bmsr = phy_read(netdev, 0, MII_BMSR);
	bmcr = phy_read(netdev, 0, MII_BMCR);

	// printk(KERN_INFO "iface %s naddr %x bmsr %x\n",
	// netdev->name, nic->mii.dev, bmsr);

	if (!mii_link_ok(&nic->mii)) {		/* no link */
		if(netif_carrier_ok(netdev)) {
			netif_carrier_off(netdev);
			nic->iface_up = 0;
			printk(KERN_INFO "%s: Link down.\n", netdev->name);
		}
		return;
	}

	if(netif_carrier_ok(netdev))
		return;

	if (bmcr & BMCR_ANENABLE) { /* AutoNegotiation is enabled */
		if (!(bmsr & BMSR_ANEGCOMPLETE)) {
			/*
			 * Do nothing - another interrupt is generated
			 * when negotiation complete
			 */
			return;
		}

		lpa  = phy_read(netdev,0, MII_LPA);
		netif_carrier_on(netdev);
		nic->iface_up = 1;

    //    endpoint_writel(nic, EP_REG_FCR,
		// EP_FCR_TXPAUSE |EP_FCR_RXPAUSE | EP_FCR_TX_THR_W(128)
		// | EP_FCR_TX_QUANTA_W(200));

		printk(KERN_INFO "%s: Link up, lpa 0x%04x.\n",
		       netdev->name, lpa);
	} else {
		netif_carrier_on(netdev);
		printk(KERN_INFO "%s: Link up.\n", netdev->name);
		nic->iface_up = 1;
	}

	ecr = endpoint_readl(nic, EP_REG_ECR);
	/* reset RMON counters */
	endpoint_writel(nic, EP_REG_ECR, ecr | EP_ECR_RST_CNT);
	endpoint_writel(nic, EP_REG_ECR, ecr );
}

static void minic_check_link(unsigned long dev_id)
{
	struct net_device *dev = (struct net_device *) dev_id;
	struct wr_minic *lp = netdev_priv(dev);

	update_link_status(dev_id);
	mod_timer(&lp->link_timer, jiffies + LINK_POLL_INTERVAL);
}

static int minic_open(struct net_device *netdev)
{
	struct wr_minic *nic = netdev_priv(netdev);

	//	printk("Bringing up: %s\n", netdev->name);

	if (!is_valid_ether_addr(netdev->dev_addr))
		return -EADDRNOTAVAIL;

	minic_writel(nic, MINIC_REG_MCR, 0);
	minic_disable_irq(nic, 0xffffffff);
	ep_enable(netdev);

	netif_carrier_off(netdev);

	init_timer(&nic->link_timer);
	nic->link_timer.data = (unsigned long)netdev;
	nic->link_timer.function = minic_check_link;
	mod_timer(&nic->link_timer, jiffies + LINK_POLL_INTERVAL);

	nic->synced = false;
	nic->syncing_counters = false;

	nic->rx_base = MINIC_PBUF_SIZE >> 1;
	nic->rx_size = MINIC_PBUF_SIZE >> 1;

	nic->tx_base = 0;
	nic->tx_size = MINIC_PBUF_SIZE >> 1;

	nic->tx_hwtstamp_enable = 0;
	nic->rx_hwtstamp_enable = 0;
	nic->tx_hwtstamp_oob = 1;

	minic_new_rx_buffer(nic);

	minic_enable_irq(nic, MINIC_EIC_IER_RX); // enable RX irq
	minic_writel(nic, MINIC_REG_MCR, MINIC_MCR_RX_EN); // enable RX

	if (netif_queue_stopped(nic->netdev)) {
		netif_wake_queue(netdev);
	} else {
		netif_start_queue(netdev);
	}

	nic->iface_up = 0;
	return 0;
}

static int minic_close(struct net_device *netdev)
{
	struct wr_minic *nic = netdev_priv(netdev);

	//	napi_disable(&nic->napi);

	del_timer_sync(&nic->link_timer);

	nic->iface_up = 0;
	ep_disable(netdev);

	if (!netif_queue_stopped(netdev))
		netif_stop_queue(netdev);

	minic_writel(nic, MINIC_REG_MCR, 0);
	minic_writel(nic, MINIC_REG_EIC_IDR, 0xffffffff);

	//	dev_info(nic->dev, "wr_close() done\n");

	return 0;
}

static void minic_update_mac(struct net_device *netdev)
{
	struct wr_minic *nic = netdev_priv(netdev);

	endpoint_writel(nic, EP_REG_MACL,
			(netdev->dev_addr[3] << 24)
			| (netdev->dev_addr[2] << 16)
			| (netdev->dev_addr[1] << 8)
			| (netdev->dev_addr[0]));
	endpoint_writel(nic, EP_REG_MACH,
			(netdev->dev_addr[5] << 8)
			| (netdev->dev_addr[4]));
}


static int minic_set_mac_address(struct net_device *netdev, void* addr)
{
	struct sockaddr *address = addr;
	if (!is_valid_ether_addr(address->sa_data))
		return -EADDRNOTAVAIL;

	memcpy(netdev->dev_addr, address->sa_data, netdev->addr_len);
	minic_update_mac(netdev);

	//  printk("%s: Setting MAC address to %pM\n",
	//netdev->name, netdev->dev_addr);

	return 0;
}


static const struct net_device_ops minic_netdev_ops = {
	.ndo_open		= minic_open,
	.ndo_stop		= minic_close,
	.ndo_start_xmit		= minic_start_xmit,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_get_stats		= minic_get_stats,
//	.ndo_set_multicast_list	= wr_set_multicast_list,
	.ndo_set_mac_address	= minic_set_mac_address,
//	.ndo_change_mtu		= wr_change_mtu,
	.ndo_do_ioctl		= minic_ioctl,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller	= NULL;//wr_netpoll;
#endif
};


static int __devinit minic_probe(struct platform_device *pdev)
{
	struct net_device	*netdev;
	struct resource		*base_minic, *base_ppsg;
	struct wr_minic		*nic;

	int			err;

	base_minic = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!base_minic) {
		dev_err(&pdev->dev, "no mmio resource defined\n");
		err = -ENXIO;
		goto err_out;
	}

	base_ppsg = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!base_ppsg) {
		dev_err(&pdev->dev, "no mmio resource defined\n");
		err = -ENXIO;
		goto err_out;
	}

	netdev = alloc_etherdev(sizeof(struct wr_minic));
	if (!netdev) {
		dev_err(&pdev->dev, "etherdev alloc failed, aborting.\n");
		err = -ENOMEM;
		goto err_out;
	}

	//  printk("base-addr 0x%x-0x%x\n", base->start, base->end);

	SET_NETDEV_DEV(netdev, &pdev->dev);

	/* initialise the nic structure */
	nic = netdev_priv(netdev);

	nic->pdev = pdev;
	nic->netdev = netdev;
	nic->dev = &pdev->dev;
	nic->iface_up = 0;

	spin_lock_init(&nic->lock);

	nic->base = ioremap(base_minic->start,
			    base_minic->end - base_minic->start + 1);
	nic->ppsg = ioremap(base_ppsg->start,
			    base_ppsg->end - base_ppsg->start + 1);

	//  dev_info(&pdev->dev, "io at 0x%x, pbuf at 0x%x\n",
	// nic->minic_regs, nic->pbuf);

	if (!nic->base || !nic->ppsg) {
		//		if (netif_msg_probe(nic))
		dev_err(&pdev->dev, "failed to map minic address space\n");
		err = -ENOMEM;
		goto err_out_free_netdev;
	}

	nic->minic_regs = nic->base + MINIC_BASE_IO;
	nic->ep_regs = nic->base + MINIC_BASE_ENDPOINT;
	nic->pbuf = nic->base + MINIC_BASE_PBUF;

	netdev->base_addr = (u32) nic->base;
	strcpy(netdev->name, (char *)pdev->dev.platform_data);

	if(endpoint_readl(nic, EP_REG_IDCODE) != 0xcafebabe) {
		printk(KERN_INFO "Looks like the port %s "
		       "hasn't been synthesized...\n", netdev->name);
		free_netdev(netdev);
		iounmap(nic->base);
		iounmap(nic->ppsg);
		return 0 ;
	}

	memset(netdev->dev_addr, 0, 6);
	//	minic_get_mac_addr(nic);

	netdev->irq = platform_get_irq(pdev, 0);
	if (netdev->irq < 0) {
		err = -ENXIO;
		goto err_out_iounmap;
	}

	err = request_irq(WRVIC_BASE_IRQ+netdev->irq, minic_interrupt,
			  IRQF_SHARED, "wr-minic", netdev);

	if (err) {
		//		if (netif_msg_probe(nic)) {
		dev_err(&netdev->dev, "request IRQ %d failed, err=%d\n",
			netdev->irq, err);
		//		}
		goto err_out_iounmap;
	}

	netdev->netdev_ops = &minic_netdev_ops;
	netdev->ethtool_ops = &minic_ethtool_ops;
	netdev->features |= 0;

	/* setup NAPI */
	memset(&nic->napi, 0, sizeof(nic->napi));
	// netif_napi_add(netdev, &nic->napi, minic_poll, MINIC_NAPI_WEIGHT);

	nic->mii.dev = netdev;		/* Support for ethtool */
	nic->mii.mdio_read = phy_read;
	nic->mii.mdio_write = phy_write;
	nic->mii.phy_id = 0;
	nic->mii.phy_id_mask = 0x1f;
	nic->mii.reg_num_mask = 0x1f;

	nic->mii.force_media = 0;
	nic->mii.advertising = ADVERTISE_1000XFULL;
	nic->mii.full_duplex = 1;

	err = register_netdev(netdev);
	if (err) {
		//		if (netif_msg_probe(nic))
		dev_err(&pdev->dev, "unable to register net device\n");
		goto err_out_freeirq;
	}

	platform_set_drvdata(pdev, netdev);

	/* WR NIC banner */
	//	if (netif_msg_probe(nic)) {
	dev_info(&pdev->dev, "White Rabbit miNIC at 0x%08lx irq %d\n",
		 netdev->base_addr, netdev->irq);
	//	}

	return 0;

err_out_freeirq:
	free_irq(netdev->irq, &pdev->dev);
err_out_iounmap:
	iounmap(nic->base);
err_out_free_netdev:
	free_netdev(netdev);
err_out:
	platform_set_drvdata(pdev, NULL);
	return err;
}

static int __devexit minic_remove(struct platform_device *pdev)
{
	struct net_device *netdev;
	struct wr_minic *nic;

	netdev = platform_get_drvdata(pdev);
	if (!netdev)
		return 0;

	free_irq(WRVIC_BASE_IRQ + netdev->irq, netdev);

	nic = netdev_priv(netdev);
	unregister_netdev(netdev);
	iounmap(nic->base);
	free_netdev(netdev);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

#define minic_suspend	NULL
#define minic_resume	NULL

static struct platform_driver minic_driver = {
	.remove		= __exit_p(minic_remove),
	.suspend	= minic_suspend,
	.resume		= minic_resume,
	.driver		= {
		.name		= DRV_NAME,
		.owner		= THIS_MODULE,
	},
};

static int __devinit minic_init_module(void)
{
	return platform_driver_probe(&minic_driver, minic_probe);
}

static void __devexit minic_cleanup_module(void)
{
	platform_driver_unregister(&minic_driver);
}

module_init(minic_init_module);
module_exit(minic_cleanup_module);
