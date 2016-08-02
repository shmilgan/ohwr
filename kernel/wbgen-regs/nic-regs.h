/*
  Register definitions for slave core: White Rabbit Switch NIC's spec

  * File           : nic-regs.h
  * Author         : auto-generated by wbgen2 from nic-regs.wb
  * Standard       : ANSI C

    THIS FILE WAS GENERATED BY wbgen2 FROM SOURCE FILE nic-regs.wb
    DO NOT HAND-EDIT UNLESS IT'S ABSOLUTELY NECESSARY!

*/

#ifndef __WBGEN2_REGDEFS_NIC
#define __WBGEN2_REGDEFS_NIC

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif


#if defined( __GNUC__)
#define PACKED __attribute__ ((packed))
#else
#error "Unsupported compiler?"
#endif

#ifndef __WBGEN2_MACROS_DEFINED__
#define __WBGEN2_MACROS_DEFINED__
#define WBGEN2_GEN_MASK(offset, size) (((1<<(size))-1) << (offset))
#define WBGEN2_GEN_WRITE(value, offset, size) (((value) & ((1<<(size))-1)) << (offset))
#define WBGEN2_GEN_READ(reg, offset, size) (((reg) >> (offset)) & ((1<<(size))-1))
#define WBGEN2_SIGN_EXTEND(value, bits) (((value) & (1<<bits) ? ~((1<<(bits))-1): 0 ) | (value))
#endif


/* definitions for register: NIC Control Register */

/* definitions for field: Receive enable in reg: NIC Control Register */
#define NIC_CR_RX_EN                          WBGEN2_GEN_MASK(0, 1)

/* definitions for field: Transmit enable in reg: NIC Control Register */
#define NIC_CR_TX_EN                          WBGEN2_GEN_MASK(1, 1)

/* definitions for field: Rx bandwidth throttling enable in reg: NIC Control Register */
#define NIC_CR_RXTHR_EN                       WBGEN2_GEN_MASK(2, 1)

/* definitions for field: Software Reset in reg: NIC Control Register */
#define NIC_CR_SW_RST                         WBGEN2_GEN_MASK(31, 1)

/* definitions for register: NIC Status Register */

/* definitions for field: Buffer Not Available in reg: NIC Status Register */
#define NIC_SR_BNA                            WBGEN2_GEN_MASK(0, 1)

/* definitions for field: Frame Received in reg: NIC Status Register */
#define NIC_SR_REC                            WBGEN2_GEN_MASK(1, 1)

/* definitions for field: Transmission done in reg: NIC Status Register */
#define NIC_SR_TX_DONE                        WBGEN2_GEN_MASK(2, 1)

/* definitions for field: Transmission error in reg: NIC Status Register */
#define NIC_SR_TX_ERROR                       WBGEN2_GEN_MASK(3, 1)

/* definitions for field: Current TX descriptor in reg: NIC Status Register */
#define NIC_SR_CUR_TX_DESC_MASK               WBGEN2_GEN_MASK(8, 3)
#define NIC_SR_CUR_TX_DESC_SHIFT              8
#define NIC_SR_CUR_TX_DESC_W(value)           WBGEN2_GEN_WRITE(value, 8, 3)
#define NIC_SR_CUR_TX_DESC_R(reg)             WBGEN2_GEN_READ(reg, 8, 3)

/* definitions for field: Current RX descriptor in reg: NIC Status Register */
#define NIC_SR_CUR_RX_DESC_MASK               WBGEN2_GEN_MASK(16, 3)
#define NIC_SR_CUR_RX_DESC_SHIFT              16
#define NIC_SR_CUR_RX_DESC_W(value)           WBGEN2_GEN_WRITE(value, 16, 3)
#define NIC_SR_CUR_RX_DESC_R(reg)             WBGEN2_GEN_READ(reg, 16, 3)

/* definitions for register: NIC Current Rx Bandwidth Register */

/* definitions for register: NIC Max Rx Bandwidth Register */

/* definitions for register: TX Descriptor 1 register 1 */

/* definitions for field: Ready in reg: TX Descriptor 1 register 1 */
#define NIC_TX1_D1_READY                      WBGEN2_GEN_MASK(0, 1)

/* definitions for field: Error in reg: TX Descriptor 1 register 1 */
#define NIC_TX1_D1_ERROR                      WBGEN2_GEN_MASK(1, 1)

/* definitions for field: Timestamp Enable in reg: TX Descriptor 1 register 1 */
#define NIC_TX1_D1_TS_E                       WBGEN2_GEN_MASK(2, 1)

/* definitions for field: Pad Enable in reg: TX Descriptor 1 register 1 */
#define NIC_TX1_D1_PAD_E                      WBGEN2_GEN_MASK(3, 1)

/* definitions for field: Timestamp Frame Identifier in reg: TX Descriptor 1 register 1 */
#define NIC_TX1_D1_TS_ID_MASK                 WBGEN2_GEN_MASK(16, 16)
#define NIC_TX1_D1_TS_ID_SHIFT                16
#define NIC_TX1_D1_TS_ID_W(value)             WBGEN2_GEN_WRITE(value, 16, 16)
#define NIC_TX1_D1_TS_ID_R(reg)               WBGEN2_GEN_READ(reg, 16, 16)

/* definitions for register: TX Descriptor 1 register 2 */

/* definitions for field: offset in RAM--in bytes, must be aligned to 32-bit boundary in reg: TX Descriptor 1 register 2 */
#define NIC_TX1_D2_OFFSET_MASK                WBGEN2_GEN_MASK(0, 16)
#define NIC_TX1_D2_OFFSET_SHIFT               0
#define NIC_TX1_D2_OFFSET_W(value)            WBGEN2_GEN_WRITE(value, 0, 16)
#define NIC_TX1_D2_OFFSET_R(reg)              WBGEN2_GEN_READ(reg, 0, 16)

/* definitions for field: Length of buffer--in bytes. Least significant bit must always be 0 (the packet size must be divisible by 2) in reg: TX Descriptor 1 register 2 */
#define NIC_TX1_D2_LEN_MASK                   WBGEN2_GEN_MASK(16, 16)
#define NIC_TX1_D2_LEN_SHIFT                  16
#define NIC_TX1_D2_LEN_W(value)               WBGEN2_GEN_WRITE(value, 16, 16)
#define NIC_TX1_D2_LEN_R(reg)                 WBGEN2_GEN_READ(reg, 16, 16)

/* definitions for register: TX Descriptor 1 register 3 */

/* definitions for field: Destination Port Mask: 0x00000001 means the packet will be sent to port 0, 0x00000002 - port 1, etc.  0xffffffff means broadcast. 0x0 doesn't make any sense yet. in reg: TX Descriptor 1 register 3 */
#define NIC_TX1_D3_DPM_MASK                   WBGEN2_GEN_MASK(0, 32)
#define NIC_TX1_D3_DPM_SHIFT                  0
#define NIC_TX1_D3_DPM_W(value)               WBGEN2_GEN_WRITE(value, 0, 32)
#define NIC_TX1_D3_DPM_R(reg)                 WBGEN2_GEN_READ(reg, 0, 32)

/* definitions for register: RX Descriptor 1 register 1 */

/* definitions for field: Empty in reg: RX Descriptor 1 register 1 */
#define NIC_RX1_D1_EMPTY                      WBGEN2_GEN_MASK(0, 1)

/* definitions for field: Error in reg: RX Descriptor 1 register 1 */
#define NIC_RX1_D1_ERROR                      WBGEN2_GEN_MASK(1, 1)

/* definitions for field: Port number of the receiving endpoint--0 to n-1. Indicated in RX OOB block. in reg: RX Descriptor 1 register 1 */
#define NIC_RX1_D1_PORT_MASK                  WBGEN2_GEN_MASK(8, 6)
#define NIC_RX1_D1_PORT_SHIFT                 8
#define NIC_RX1_D1_PORT_W(value)              WBGEN2_GEN_WRITE(value, 8, 6)
#define NIC_RX1_D1_PORT_R(reg)                WBGEN2_GEN_READ(reg, 8, 6)

/* definitions for field: Got RX Timestamp in reg: RX Descriptor 1 register 1 */
#define NIC_RX1_D1_GOT_TS                     WBGEN2_GEN_MASK(14, 1)

/* definitions for field: RX Timestamp (possibly) incorrect in reg: RX Descriptor 1 register 1 */
#define NIC_RX1_D1_TS_INCORRECT               WBGEN2_GEN_MASK(15, 1)

/* definitions for register: RX Descriptor 1 register 2 */

/* definitions for field: RX_TS_R in reg: RX Descriptor 1 register 2 */
#define NIC_RX1_D2_TS_R_MASK                  WBGEN2_GEN_MASK(0, 28)
#define NIC_RX1_D2_TS_R_SHIFT                 0
#define NIC_RX1_D2_TS_R_W(value)              WBGEN2_GEN_WRITE(value, 0, 28)
#define NIC_RX1_D2_TS_R_R(reg)                WBGEN2_GEN_READ(reg, 0, 28)

/* definitions for field: RX_TS_F in reg: RX Descriptor 1 register 2 */
#define NIC_RX1_D2_TS_F_MASK                  WBGEN2_GEN_MASK(28, 4)
#define NIC_RX1_D2_TS_F_SHIFT                 28
#define NIC_RX1_D2_TS_F_W(value)              WBGEN2_GEN_WRITE(value, 28, 4)
#define NIC_RX1_D2_TS_F_R(reg)                WBGEN2_GEN_READ(reg, 28, 4)

/* definitions for register: RX Descriptor 1 register 3 */

/* definitions for field: Offset in packet RAM (in bytes, 32-bit aligned) in reg: RX Descriptor 1 register 3 */
#define NIC_RX1_D3_OFFSET_MASK                WBGEN2_GEN_MASK(0, 16)
#define NIC_RX1_D3_OFFSET_SHIFT               0
#define NIC_RX1_D3_OFFSET_W(value)            WBGEN2_GEN_WRITE(value, 0, 16)
#define NIC_RX1_D3_OFFSET_R(reg)              WBGEN2_GEN_READ(reg, 0, 16)

/* definitions for field: Length of buffer in bytes. After reception of the packet, it's updated with the length of the received packet. in reg: RX Descriptor 1 register 3 */
#define NIC_RX1_D3_LEN_MASK                   WBGEN2_GEN_MASK(16, 16)
#define NIC_RX1_D3_LEN_SHIFT                  16
#define NIC_RX1_D3_LEN_W(value)               WBGEN2_GEN_WRITE(value, 16, 16)
#define NIC_RX1_D3_LEN_R(reg)                 WBGEN2_GEN_READ(reg, 16, 16)

/* definitions for register: Interrupt disable register */

/* definitions for field: Receive Complete in reg: Interrupt disable register */
#define NIC_EIC_IDR_RCOMP                     WBGEN2_GEN_MASK(0, 1)

/* definitions for field: Transmit Complete in reg: Interrupt disable register */
#define NIC_EIC_IDR_TCOMP                     WBGEN2_GEN_MASK(1, 1)

/* definitions for field: Transmit Error in reg: Interrupt disable register */
#define NIC_EIC_IDR_TXERR                     WBGEN2_GEN_MASK(2, 1)

/* definitions for register: Interrupt enable register */

/* definitions for field: Receive Complete in reg: Interrupt enable register */
#define NIC_EIC_IER_RCOMP                     WBGEN2_GEN_MASK(0, 1)

/* definitions for field: Transmit Complete in reg: Interrupt enable register */
#define NIC_EIC_IER_TCOMP                     WBGEN2_GEN_MASK(1, 1)

/* definitions for field: Transmit Error in reg: Interrupt enable register */
#define NIC_EIC_IER_TXERR                     WBGEN2_GEN_MASK(2, 1)

/* definitions for register: Interrupt mask register */

/* definitions for field: Receive Complete in reg: Interrupt mask register */
#define NIC_EIC_IMR_RCOMP                     WBGEN2_GEN_MASK(0, 1)

/* definitions for field: Transmit Complete in reg: Interrupt mask register */
#define NIC_EIC_IMR_TCOMP                     WBGEN2_GEN_MASK(1, 1)

/* definitions for field: Transmit Error in reg: Interrupt mask register */
#define NIC_EIC_IMR_TXERR                     WBGEN2_GEN_MASK(2, 1)

/* definitions for register: Interrupt status register */

/* definitions for field: Receive Complete in reg: Interrupt status register */
#define NIC_EIC_ISR_RCOMP                     WBGEN2_GEN_MASK(0, 1)

/* definitions for field: Transmit Complete in reg: Interrupt status register */
#define NIC_EIC_ISR_TCOMP                     WBGEN2_GEN_MASK(1, 1)

/* definitions for field: Transmit Error in reg: Interrupt status register */
#define NIC_EIC_ISR_TXERR                     WBGEN2_GEN_MASK(2, 1)
/* definitions for RAM: TX descriptors mem */
#define NIC_DTX_BASE 0x00000080 /* base address */                                
#define NIC_DTX_BYTES 0x00000080 /* size in bytes */                               
#define NIC_DTX_WORDS 0x00000020 /* size in 32-bit words, 32-bit aligned */        
/* definitions for RAM: RX descriptors mem */
#define NIC_DRX_BASE 0x00000100 /* base address */                                
#define NIC_DRX_BYTES 0x00000080 /* size in bytes */                               
#define NIC_DRX_WORDS 0x00000020 /* size in 32-bit words, 32-bit aligned */        

PACKED struct NIC_WB {
  /* [0x0]: REG NIC Control Register */
  uint32_t CR;
  /* [0x4]: REG NIC Status Register */
  uint32_t SR;
  /* [0x8]: REG NIC Current Rx Bandwidth Register */
  uint32_t RXBW;
  /* [0xc]: REG NIC Max Rx Bandwidth Register */
  uint32_t MAXRXBW;
  /* [0x10]: REG TX Descriptor 1 register 1 */
  uint32_t TX1_D1;
  /* [0x14]: REG TX Descriptor 1 register 2 */
  uint32_t TX1_D2;
  /* [0x18]: REG TX Descriptor 1 register 3 */
  uint32_t TX1_D3;
  /* padding to: 8 words */
  uint32_t __padding_0[1];
  /* [0x20]: REG RX Descriptor 1 register 1 */
  uint32_t RX1_D1;
  /* [0x24]: REG RX Descriptor 1 register 2 */
  uint32_t RX1_D2;
  /* [0x28]: REG RX Descriptor 1 register 3 */
  uint32_t RX1_D3;
  /* padding to: 16 words */
  uint32_t __padding_1[5];
  /* [0x40]: REG Interrupt disable register */
  uint32_t EIC_IDR;
  /* [0x44]: REG Interrupt enable register */
  uint32_t EIC_IER;
  /* [0x48]: REG Interrupt mask register */
  uint32_t EIC_IMR;
  /* [0x4c]: REG Interrupt status register */
  uint32_t EIC_ISR;
  /* padding to: 32 words */
  uint32_t __padding_2[12];
  /* [0x80 - 0xff]: RAM TX descriptors mem, 32 32-bit words, 32-bit aligned, word-addressable */
  uint32_t DTX [32];
  /* padding to: 64 words */
  uint32_t __padding_3[32];
  /* [0x100 - 0x17f]: RAM RX descriptors mem, 32 32-bit words, 32-bit aligned, word-addressable */
  uint32_t DRX [32];
};

#endif
