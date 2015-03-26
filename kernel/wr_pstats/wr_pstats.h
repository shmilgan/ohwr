#ifndef __WR_PSTATS_H__
#define __WR_PSTATS_H__

/*General VIC IRQ stuff*/
#define FPGA_BASE_PSTATS 0x10058000
#define WR_PSTATS_IRQ	3
#define WRVIC_BASE_IRQ  (NR_AIC_IRQS + (5 * 32))
/*****/

#define PSTATS_MAX_NUM_OF_COUNTERS 39		/* Maximum number of counters
						 * supported by the driver */

#define PSTATS_MAX_NPORTS 18			/* Maximum number of ports
						 * supported by the driver */
#define PSTATS_DEFAULT_NPORTS PSTATS_MAX_NPORTS	/* how many eth ports are
						 * in the switch */

#define PSTATS_MSB_SHIFT	16	/*how many bits are stored in hw*/
#define PSTATS_LSB_MSK 0x0000ffff
#define PSTATS_MSB_MSK 0xffff0000
#define PSTATS_ALL_MSK 0xffffffff

#define PSTATS_IRQBUFSZ  16

#define PINFO_SIZE  3
#define PINFO_VER   0
#define PINFO_CNTPW 1
#define PINFO_CNTPP 2

extern int (*wr_nic_pstats_callback)(int epnum,
				     struct net_device_stats *stats);

struct pstats_version_description {
	unsigned int rx_packets;
	unsigned int tx_packets;
	unsigned int rx_errors;
	unsigned int tx_carrier_errors;
	unsigned int rx_length_errors;
	unsigned int rx_crc_errors;
	unsigned int rx_fifo_errors;
	unsigned int tx_fifo_errors;
	char *cnt_names;
};

#endif
