#ifndef __WR_PSTATS_H__
#define __WR_PSTATS_H__

/*General VIC IRQ stuff*/
#define FPGA_BASE_PSTATS 0x10058000
#define WR_PSTATS_IRQ	3
#define WRVIC_BASE_IRQ  (NR_AIC_IRQS + (5 * 32))
/*****/


#define PSTATS_NPORTS 18	/* how many eth ports are in the switch */
#define PSTATS_CNT_PP 39	/* how many counters per port */
#define PSTATS_ADR_PP ((PSTATS_CNT_PP+3)/4)	/* how many address words are
						 * there per counter (each of
						 * them stores the state of 4
						 * counters) */
#define PSTATS_MSB_SHIFT	16	/*how many bits are stored in hw*/
#define PSTATS_LSB_MSK 0x0000ffff
#define PSTATS_MSB_MSK 0xffff0000
#define PSTATS_ALL_MSK 0xffffffff

#define PSTATS_IRQBUFSZ  16

extern int (*wr_nic_pstats_callback)(int epnum,
				     unsigned int ctr[PSTATS_CNT_PP]);

enum { /* names for values, from page 14 of hw/gw document */
	PSTATS_C_T_UNDERRUN = 0,
	PSTATS_C_R_OVERRUN,
	PSTATS_C_R_INVALID_CODE,
	PSTATS_C_R_SYNC_LOST,
	PSTATS_C_R_PAUSE,
	PSTATS_C_R_PFILTER_DROP,
	PSTATS_C_R_PCS_ERROR,
	PSTATS_C_R_GIANT,
	PSTATS_C_R_RUNT,
	PSTATS_C_R_CRC_ERROR,
	PSTATS_C_R_PCLASS_0,
	PSTATS_C_R_PCLASS_1,
	PSTATS_C_R_PCLASS_2,
	PSTATS_C_R_PCLASS_3,
	PSTATS_C_R_PCLASS_4,
	PSTATS_C_R_PCLASS_5,
	PSTATS_C_R_PCLASS_6,
	PSTATS_C_R_PCLASS_7,
	PSTATS_C_T_FRAME,
	PSTATS_C_R_FRAME,
	PSTATS_C_RTU_REQ_FLAG,
	PSTATS_C_R_PRI_0,
	PSTATS_C_R_PRI_1,
	PSTATS_C_R_PRI_2,
	PSTATS_C_R_PRI_3,
	PSTATS_C_R_PRI_4,
	PSTATS_C_R_PRI_5,
	PSTATS_C_R_PRI_6,
	PSTATS_C_R_PRI_7,
	PSTATS_C_RTU_REQ,
	PSTATS_C_RTU_RESP,
	PSTATS_C_RTU_DROPS,
	PSTATS_C_RTU_HP,
	PSTATS_C_RTU_FF,
	PSTATS_C_RTU_NF,
	PSTATS_C_RTU_FST,
	PSTATS_C_RTU_FULL,
	PSTATS_C_RTU_FWD,
	PSTATS_C_RTU_RSP,
};

#endif
