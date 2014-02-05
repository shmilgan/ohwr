#ifndef __WR_PSTATS_H__
#define __WR_PSTATS_H__

/*General VIC IRQ stuff*/
#define FPGA_BASE_PSTATS 0x10058000
#define WR_PSTATS_IRQ	3
#define WRVIC_BASE_IRQ  (NR_AIC_IRQS + (5 * 32))
/*****/


#define PSTATS_NPORTS 8	/* how many eth ports are in the switch */
#define PSTATS_CNT_PP 38	/* how many counters per port */
#define PSTATS_ADR_PP ((PSTATS_CNT_PP+3)/4)	/* how many address words are
						 * there per counter (each of
						 * them stores the state of 4
						 * counters) */
#define PSTATS_MSB_SHIFT	16	/*how many bits are stored in hw*/
#define PSTATS_LSB_MSK 0x0000ffff
#define PSTATS_MSB_MSK 0xffff0000
#define PSTATS_ALL_MSK 0xffffffff

#define PSTATS_IRQBUFSZ  16


#endif
