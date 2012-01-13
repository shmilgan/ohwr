/*
  Register definitions for slave core: WR switch endpoint controller

  * File           : ../../../software/include/hw/endpoint_regs.h
  * Author         : auto-generated by wbgen2 from ep_wishbone_controller.wb
  * Created        : Wed Nov  3 19:00:12 2010
  * Standard       : ANSI C

    THIS FILE WAS GENERATED BY wbgen2 FROM SOURCE FILE ep_wishbone_controller.wb
    DO NOT HAND-EDIT UNLESS IT'S ABSOLUTELY NECESSARY!

*/

#ifndef __WBGEN2_REGDEFS_EP_WISHBONE_CONTROLLER_WB
#define __WBGEN2_REGDEFS_EP_WISHBONE_CONTROLLER_WB


#ifndef __WBGEN2_MACROS_DEFINED__
#define __WBGEN2_MACROS_DEFINED__
#define WBGEN2_GEN_MASK(offset, size) (((1<<(size))-1) << (offset))
#define WBGEN2_GEN_WRITE(value, offset, size) (((value) & ((1<<(size))-1)) << (offset))
#define WBGEN2_GEN_READ(reg, offset, size) (((reg) >> (offset)) & ((1<<(size))-1))
#define WBGEN2_SIGN_EXTEND(value, bits) (((value) & (1<<bits) ? ~((1<<(bits))-1): 0 ) | (value))
#endif


/* definitions for register: Endpoint Control Register */

/* definitions for field: Port identifier in reg: Endpoint Control Register */
#define EP_ECR_PORTID_MASK                    WBGEN2_GEN_MASK(0, 5)
#define EP_ECR_PORTID_SHIFT                   0
#define EP_ECR_PORTID_W(value)                WBGEN2_GEN_WRITE(value, 0, 5)
#define EP_ECR_PORTID_R(reg)                  WBGEN2_GEN_READ(reg, 0, 5)

/* definitions for field: Reset event counters in reg: Endpoint Control Register */
#define EP_ECR_RST_CNT                        WBGEN2_GEN_MASK(5, 1)

/* definitions for field: Transmit framer enable in reg: Endpoint Control Register */
#define EP_ECR_TX_EN_FRA                      WBGEN2_GEN_MASK(6, 1)

/* definitions for field: Receive deframer enable in reg: Endpoint Control Register */
#define EP_ECR_RX_EN_FRA                      WBGEN2_GEN_MASK(7, 1)

/* definitions for register: Timestamping Control Register */

/* definitions for field: Transmit timestamping enable in reg: Timestamping Control Register */
#define EP_TSCR_EN_TXTS                       WBGEN2_GEN_MASK(0, 1)

/* definitions for field: Receive timestamping enable in reg: Timestamping Control Register */
#define EP_TSCR_EN_RXTS                       WBGEN2_GEN_MASK(1, 1)

/* definitions for field: Timestamping counter synchronization start in reg: Timestamping Control Register */
#define EP_TSCR_CS_START                      WBGEN2_GEN_MASK(2, 1)

/* definitions for field: Timestamping counter synchronization done in reg: Timestamping Control Register */
#define EP_TSCR_CS_DONE                       WBGEN2_GEN_MASK(3, 1)

/* definitions for register: RX Deframer Control Register */

/* definitions for field: RX accept runts in reg: RX Deframer Control Register */
#define EP_RFCR_A_RUNT                        WBGEN2_GEN_MASK(0, 1)

/* definitions for field: RX accept giants in reg: RX Deframer Control Register */
#define EP_RFCR_A_GIANT                       WBGEN2_GEN_MASK(1, 1)

/* definitions for field: RX accept HP in reg: RX Deframer Control Register */
#define EP_RFCR_A_HP                          WBGEN2_GEN_MASK(2, 1)

/* definitions for field: RX accept fragments in reg: RX Deframer Control Register */
#define EP_RFCR_A_FRAG                        WBGEN2_GEN_MASK(3, 1)

/* definitions for field: RX 802.1q port mode in reg: RX Deframer Control Register */
#define EP_RFCR_QMODE_MASK                    WBGEN2_GEN_MASK(4, 2)
#define EP_RFCR_QMODE_SHIFT                   4
#define EP_RFCR_QMODE_W(value)                WBGEN2_GEN_WRITE(value, 4, 2)
#define EP_RFCR_QMODE_R(reg)                  WBGEN2_GEN_READ(reg, 4, 2)

/* definitions for field: Force 802.1q priority in reg: RX Deframer Control Register */
#define EP_RFCR_FIX_PRIO                      WBGEN2_GEN_MASK(6, 1)

/* definitions for field: Port-assigned 802.1x priority in reg: RX Deframer Control Register */
#define EP_RFCR_PRIO_VAL_MASK                 WBGEN2_GEN_MASK(8, 3)
#define EP_RFCR_PRIO_VAL_SHIFT                8
#define EP_RFCR_PRIO_VAL_W(value)             WBGEN2_GEN_WRITE(value, 8, 3)
#define EP_RFCR_PRIO_VAL_R(reg)               WBGEN2_GEN_READ(reg, 8, 3)

/* definitions for field: Port-assigned VID in reg: RX Deframer Control Register */
#define EP_RFCR_VID_VAL_MASK                  WBGEN2_GEN_MASK(16, 12)
#define EP_RFCR_VID_VAL_SHIFT                 16
#define EP_RFCR_VID_VAL_W(value)              WBGEN2_GEN_WRITE(value, 16, 12)
#define EP_RFCR_VID_VAL_R(reg)                WBGEN2_GEN_READ(reg, 16, 12)

/* definitions for register: Flow Control Register */

/* definitions for field: RX Pause enable in reg: Flow Control Register */
#define EP_FCR_RXPAUSE                        WBGEN2_GEN_MASK(0, 1)

/* definitions for field: TX Pause enable in reg: Flow Control Register */
#define EP_FCR_TXPAUSE                        WBGEN2_GEN_MASK(1, 1)

/* definitions for field: TX pause threshold in reg: Flow Control Register */
#define EP_FCR_TX_THR_MASK                    WBGEN2_GEN_MASK(8, 8)
#define EP_FCR_TX_THR_SHIFT                   8
#define EP_FCR_TX_THR_W(value)                WBGEN2_GEN_WRITE(value, 8, 8)
#define EP_FCR_TX_THR_R(reg)                  WBGEN2_GEN_READ(reg, 8, 8)

/* definitions for field: TX pause quanta in reg: Flow Control Register */
#define EP_FCR_TX_QUANTA_MASK                 WBGEN2_GEN_MASK(16, 16)
#define EP_FCR_TX_QUANTA_SHIFT                16
#define EP_FCR_TX_QUANTA_W(value)             WBGEN2_GEN_WRITE(value, 16, 16)
#define EP_FCR_TX_QUANTA_R(reg)               WBGEN2_GEN_READ(reg, 16, 16)

/* definitions for register: Endpoint MAC address high part register */

/* definitions for register: Endpoint MAC address low part register */

/* definitions for register: DMTD Control Register */

/* definitions for field: DMTD Phase measurement enable in reg: DMTD Control Register */
#define EP_DMCR_EN                            WBGEN2_GEN_MASK(0, 1)

/* definitions for field: DMTD averaging samples in reg: DMTD Control Register */
#define EP_DMCR_N_AVG_MASK                    WBGEN2_GEN_MASK(16, 12)
#define EP_DMCR_N_AVG_SHIFT                   16
#define EP_DMCR_N_AVG_W(value)                WBGEN2_GEN_WRITE(value, 16, 12)
#define EP_DMCR_N_AVG_R(reg)                  WBGEN2_GEN_READ(reg, 16, 12)

/* definitions for register: DMTD Status register */

/* definitions for field: DMTD Phase shift value in reg: DMTD Status register */
#define EP_DMSR_PS_VAL_MASK                   WBGEN2_GEN_MASK(0, 24)
#define EP_DMSR_PS_VAL_SHIFT                  0
#define EP_DMSR_PS_VAL_W(value)               WBGEN2_GEN_WRITE(value, 0, 24)
#define EP_DMSR_PS_VAL_R(reg)                 WBGEN2_GEN_READ(reg, 0, 24)

/* definitions for field: DMTD Phase shift value ready in reg: DMTD Status register */
#define EP_DMSR_PS_RDY                        WBGEN2_GEN_MASK(24, 1)

/* definitions for register: MDIO Control Register */

/* definitions for field: MDIO Register Value in reg: MDIO Control Register */
#define EP_MDIO_CR_DATA_MASK                  WBGEN2_GEN_MASK(0, 16)
#define EP_MDIO_CR_DATA_SHIFT                 0
#define EP_MDIO_CR_DATA_W(value)              WBGEN2_GEN_WRITE(value, 0, 16)
#define EP_MDIO_CR_DATA_R(reg)                WBGEN2_GEN_READ(reg, 0, 16)

/* definitions for field: MDIO Register Address in reg: MDIO Control Register */
#define EP_MDIO_CR_ADDR_MASK                  WBGEN2_GEN_MASK(16, 8)
#define EP_MDIO_CR_ADDR_SHIFT                 16
#define EP_MDIO_CR_ADDR_W(value)              WBGEN2_GEN_WRITE(value, 16, 8)
#define EP_MDIO_CR_ADDR_R(reg)                WBGEN2_GEN_READ(reg, 16, 8)

/* definitions for field: MDIO Read/Write select in reg: MDIO Control Register */
#define EP_MDIO_CR_RW                         WBGEN2_GEN_MASK(31, 1)

/* definitions for register: MDIO Status Register */

/* definitions for field: MDIO Read Value in reg: MDIO Status Register */
#define EP_MDIO_SR_RDATA_MASK                 WBGEN2_GEN_MASK(0, 16)
#define EP_MDIO_SR_RDATA_SHIFT                0
#define EP_MDIO_SR_RDATA_W(value)             WBGEN2_GEN_WRITE(value, 0, 16)
#define EP_MDIO_SR_RDATA_R(reg)               WBGEN2_GEN_READ(reg, 0, 16)

/* definitions for field: MDIO Ready in reg: MDIO Status Register */
#define EP_MDIO_SR_READY                      WBGEN2_GEN_MASK(31, 1)
/* definitions for RAM: Event counters memory */
#define EP_RMON_RAM_BYTES 0x00000080 /* size in bytes */                               
#define EP_RMON_RAM_WORDS 0x00000020 /* size in 32-bit words, 32-bit aligned */        
/* [0x0]: REG Endpoint Control Register */
#define EP_REG_ECR 0x00000000
/* [0x4]: REG Timestamping Control Register */
#define EP_REG_TSCR 0x00000004
/* [0x8]: REG RX Deframer Control Register */
#define EP_REG_RFCR 0x00000008
/* [0xc]: REG Flow Control Register */
#define EP_REG_FCR 0x0000000c
/* [0x10]: REG Endpoint MAC address high part register */
#define EP_REG_MACH 0x00000010
/* [0x14]: REG Endpoint MAC address low part register */
#define EP_REG_MACL 0x00000014
/* [0x18]: REG DMTD Control Register */
#define EP_REG_DMCR 0x00000018
/* [0x1c]: REG DMTD Status register */
#define EP_REG_DMSR 0x0000001c
/* [0x20]: REG MDIO Control Register */
#define EP_REG_MDIO_CR 0x00000020
/* [0x24]: REG MDIO Status Register */
#define EP_REG_MDIO_SR 0x00000024

#define EP_REG_IDCODE 0x00000028

/* definitions for register: WhiteRabbit-specific Configuration Register */

/* definitions for field: TX Calibration Pattern in reg: WhiteRabbit-specific Configuration Register */
#define MDIO_WR_SPEC_TX_CAL                   WBGEN2_GEN_MASK(0, 1)

/* definitions for field: Calibration Pattern RX Status in reg: WhiteRabbit-specific Configuration Register */
#define MDIO_WR_SPEC_RX_CAL_STAT              WBGEN2_GEN_MASK(1, 1)

/* definitions for field: Reset calibration counter in reg: WhiteRabbit-specific Configuration Register */
#define MDIO_WR_SPEC_CAL_CRST                 WBGEN2_GEN_MASK(2, 1)

/* [0x10]: REG WhiteRabbit-specific Configuration Register */
#define MDIO_REG_WR_SPEC 0x00000010
#endif