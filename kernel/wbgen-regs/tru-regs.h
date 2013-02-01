/*
  Register definitions for slave core: Topology Resolution Unit (TRU)

  * File           : tru-regs.h
  * Author         : auto-generated by wbgen2 from tru_wishbone_slave.wb
  * Created        : Thu Jan 31 21:16:18 2013
  * Standard       : ANSI C

    THIS FILE WAS GENERATED BY wbgen2 FROM SOURCE FILE tru_wishbone_slave.wb
    DO NOT HAND-EDIT UNLESS IT'S ABSOLUTELY NECESSARY!

*/

#ifndef __WBGEN2_REGDEFS_TRU_WISHBONE_SLAVE_WB
#define __WBGEN2_REGDEFS_TRU_WISHBONE_SLAVE_WB

#include <inttypes.h>

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


/* definitions for register: TRU Global Control Register */

/* definitions for field: TRU Global Enable in reg: TRU Global Control Register */
#define TRU_GCR_G_ENA                         WBGEN2_GEN_MASK(0, 1)

/* definitions for field: Swap TRU TAB bank in reg: TRU Global Control Register */
#define TRU_GCR_TRU_BANK                      WBGEN2_GEN_MASK(1, 1)

/* definitions for field: Rx Frame Reset in reg: TRU Global Control Register */
#define TRU_GCR_RX_FRAME_RESET_MASK           WBGEN2_GEN_MASK(8, 24)
#define TRU_GCR_RX_FRAME_RESET_SHIFT          8
#define TRU_GCR_RX_FRAME_RESET_W(value)       WBGEN2_GEN_WRITE(value, 8, 24)
#define TRU_GCR_RX_FRAME_RESET_R(reg)         WBGEN2_GEN_READ(reg, 8, 24)

/* definitions for register: TRU Global Status Register 0 */

/* definitions for field: Active Bank in reg: TRU Global Status Register 0 */
#define TRU_GSR0_STAT_BANK                    WBGEN2_GEN_MASK(0, 1)

/* definitions for field: Stable Ports UP in reg: TRU Global Status Register 0 */
#define TRU_GSR0_STAT_STB_UP_MASK             WBGEN2_GEN_MASK(8, 24)
#define TRU_GSR0_STAT_STB_UP_SHIFT            8
#define TRU_GSR0_STAT_STB_UP_W(value)         WBGEN2_GEN_WRITE(value, 8, 24)
#define TRU_GSR0_STAT_STB_UP_R(reg)           WBGEN2_GEN_READ(reg, 8, 24)

/* definitions for register: TRU Global Status Register 1 */

/* definitions for field: Ports UP in reg: TRU Global Status Register 1 */
#define TRU_GSR1_STAT_UP_MASK                 WBGEN2_GEN_MASK(0, 32)
#define TRU_GSR1_STAT_UP_SHIFT                0
#define TRU_GSR1_STAT_UP_W(value)             WBGEN2_GEN_WRITE(value, 0, 32)
#define TRU_GSR1_STAT_UP_R(reg)               WBGEN2_GEN_READ(reg, 0, 32)

/* definitions for register: Pattern Control Register */

/* definitions for field: Replace Pattern Mode in reg: Pattern Control Register */
#define TRU_MCR_PATTERN_MODE_REP_MASK         WBGEN2_GEN_MASK(0, 4)
#define TRU_MCR_PATTERN_MODE_REP_SHIFT        0
#define TRU_MCR_PATTERN_MODE_REP_W(value)     WBGEN2_GEN_WRITE(value, 0, 4)
#define TRU_MCR_PATTERN_MODE_REP_R(reg)       WBGEN2_GEN_READ(reg, 0, 4)

/* definitions for field: Addition Pattern Mode in reg: Pattern Control Register */
#define TRU_MCR_PATTERN_MODE_ADD_MASK         WBGEN2_GEN_MASK(8, 4)
#define TRU_MCR_PATTERN_MODE_ADD_SHIFT        8
#define TRU_MCR_PATTERN_MODE_ADD_W(value)     WBGEN2_GEN_WRITE(value, 8, 4)
#define TRU_MCR_PATTERN_MODE_ADD_R(reg)       WBGEN2_GEN_READ(reg, 8, 4)

/* definitions for register: Link Aggregation Control Register */

/* definitions for field: HP traffic Distribution Function ID in reg: Link Aggregation Control Register */
#define TRU_LACR_AGG_DF_HP_ID_MASK            WBGEN2_GEN_MASK(0, 4)
#define TRU_LACR_AGG_DF_HP_ID_SHIFT           0
#define TRU_LACR_AGG_DF_HP_ID_W(value)        WBGEN2_GEN_WRITE(value, 0, 4)
#define TRU_LACR_AGG_DF_HP_ID_R(reg)          WBGEN2_GEN_READ(reg, 0, 4)

/* definitions for field: Broadcast Distribution Function ID in reg: Link Aggregation Control Register */
#define TRU_LACR_AGG_DF_BR_ID_MASK            WBGEN2_GEN_MASK(8, 4)
#define TRU_LACR_AGG_DF_BR_ID_SHIFT           8
#define TRU_LACR_AGG_DF_BR_ID_W(value)        WBGEN2_GEN_WRITE(value, 8, 4)
#define TRU_LACR_AGG_DF_BR_ID_R(reg)          WBGEN2_GEN_READ(reg, 8, 4)

/* definitions for field: Unicast Distribution Function ID in reg: Link Aggregation Control Register */
#define TRU_LACR_AGG_DF_UN_ID_MASK            WBGEN2_GEN_MASK(16, 4)
#define TRU_LACR_AGG_DF_UN_ID_SHIFT           16
#define TRU_LACR_AGG_DF_UN_ID_W(value)        WBGEN2_GEN_WRITE(value, 16, 4)
#define TRU_LACR_AGG_DF_UN_ID_R(reg)          WBGEN2_GEN_READ(reg, 16, 4)

/* definitions for register: Transition Control General Register */

/* definitions for field: Transition Enabled in reg: Transition Control General Register */
#define TRU_TCGR_TRANS_ENA                    WBGEN2_GEN_MASK(0, 1)

/* definitions for field: Transition Clear in reg: Transition Control General Register */
#define TRU_TCGR_TRANS_CLEAR                  WBGEN2_GEN_MASK(1, 1)

/* definitions for field: Transition Mode in reg: Transition Control General Register */
#define TRU_TCGR_TRANS_MODE_MASK              WBGEN2_GEN_MASK(4, 3)
#define TRU_TCGR_TRANS_MODE_SHIFT             4
#define TRU_TCGR_TRANS_MODE_W(value)          WBGEN2_GEN_WRITE(value, 4, 3)
#define TRU_TCGR_TRANS_MODE_R(reg)            WBGEN2_GEN_READ(reg, 4, 3)

/* definitions for field: Rx Detected Frame ID in reg: Transition Control General Register */
#define TRU_TCGR_TRANS_RX_ID_MASK             WBGEN2_GEN_MASK(8, 3)
#define TRU_TCGR_TRANS_RX_ID_SHIFT            8
#define TRU_TCGR_TRANS_RX_ID_W(value)         WBGEN2_GEN_WRITE(value, 8, 3)
#define TRU_TCGR_TRANS_RX_ID_R(reg)           WBGEN2_GEN_READ(reg, 8, 3)

/* definitions for field: Priority in reg: Transition Control General Register */
#define TRU_TCGR_TRANS_PRIO_MASK              WBGEN2_GEN_MASK(12, 3)
#define TRU_TCGR_TRANS_PRIO_SHIFT             12
#define TRU_TCGR_TRANS_PRIO_W(value)          WBGEN2_GEN_WRITE(value, 12, 3)
#define TRU_TCGR_TRANS_PRIO_R(reg)            WBGEN2_GEN_READ(reg, 12, 3)

/* definitions for field: Port Time Difference in reg: Transition Control General Register */
#define TRU_TCGR_TRANS_TIME_DIFF_MASK         WBGEN2_GEN_MASK(16, 16)
#define TRU_TCGR_TRANS_TIME_DIFF_SHIFT        16
#define TRU_TCGR_TRANS_TIME_DIFF_W(value)     WBGEN2_GEN_WRITE(value, 16, 16)
#define TRU_TCGR_TRANS_TIME_DIFF_R(reg)       WBGEN2_GEN_READ(reg, 16, 16)

/* definitions for register: Transition Control Port Register */

/* definitions for field: Port A ID in reg: Transition Control Port Register */
#define TRU_TCPR_TRANS_PORT_A_ID_MASK         WBGEN2_GEN_MASK(0, 6)
#define TRU_TCPR_TRANS_PORT_A_ID_SHIFT        0
#define TRU_TCPR_TRANS_PORT_A_ID_W(value)     WBGEN2_GEN_WRITE(value, 0, 6)
#define TRU_TCPR_TRANS_PORT_A_ID_R(reg)       WBGEN2_GEN_READ(reg, 0, 6)

/* definitions for field: Port A Valid in reg: Transition Control Port Register */
#define TRU_TCPR_TRANS_PORT_A_VALID           WBGEN2_GEN_MASK(8, 1)

/* definitions for field: Port B ID in reg: Transition Control Port Register */
#define TRU_TCPR_TRANS_PORT_B_ID_MASK         WBGEN2_GEN_MASK(16, 6)
#define TRU_TCPR_TRANS_PORT_B_ID_SHIFT        16
#define TRU_TCPR_TRANS_PORT_B_ID_W(value)     WBGEN2_GEN_WRITE(value, 16, 6)
#define TRU_TCPR_TRANS_PORT_B_ID_R(reg)       WBGEN2_GEN_READ(reg, 16, 6)

/* definitions for field: Port B Valid in reg: Transition Control Port Register */
#define TRU_TCPR_TRANS_PORT_B_VALID           WBGEN2_GEN_MASK(24, 1)

/* definitions for register: Transition Status Register */

/* definitions for field: Transition Active in reg: Transition Status Register */
#define TRU_TSR_TRANS_STAT_ACTIVE             WBGEN2_GEN_MASK(0, 1)

/* definitions for field: Transition Finished in reg: Transition Status Register */
#define TRU_TSR_TRANS_STAT_FINISHED           WBGEN2_GEN_MASK(1, 1)

/* definitions for register: Real Time Reconfiguration Control Register */

/* definitions for field: RTR Enabled in reg: Real Time Reconfiguration Control Register */
#define TRU_RTRCR_RTR_ENA                     WBGEN2_GEN_MASK(0, 1)

/* definitions for field: RTR Reset in reg: Real Time Reconfiguration Control Register */
#define TRU_RTRCR_RTR_RESET                   WBGEN2_GEN_MASK(1, 1)

/* definitions for field: RTR Handler Mode in reg: Real Time Reconfiguration Control Register */
#define TRU_RTRCR_RTR_MODE_MASK               WBGEN2_GEN_MASK(8, 4)
#define TRU_RTRCR_RTR_MODE_SHIFT              8
#define TRU_RTRCR_RTR_MODE_W(value)           WBGEN2_GEN_WRITE(value, 8, 4)
#define TRU_RTRCR_RTR_MODE_R(reg)             WBGEN2_GEN_READ(reg, 8, 4)

/* definitions for field: RTR Rx Frame ID in reg: Real Time Reconfiguration Control Register */
#define TRU_RTRCR_RTR_RX_MASK                 WBGEN2_GEN_MASK(16, 4)
#define TRU_RTRCR_RTR_RX_SHIFT                16
#define TRU_RTRCR_RTR_RX_W(value)             WBGEN2_GEN_WRITE(value, 16, 4)
#define TRU_RTRCR_RTR_RX_R(reg)               WBGEN2_GEN_READ(reg, 16, 4)

/* definitions for field: RTR Tx Frame ID in reg: Real Time Reconfiguration Control Register */
#define TRU_RTRCR_RTR_TX_MASK                 WBGEN2_GEN_MASK(24, 4)
#define TRU_RTRCR_RTR_TX_SHIFT                24
#define TRU_RTRCR_RTR_TX_W(value)             WBGEN2_GEN_WRITE(value, 24, 4)
#define TRU_RTRCR_RTR_TX_R(reg)               WBGEN2_GEN_READ(reg, 24, 4)

/* definitions for register: TRU Table Register 0 */

/* definitions for field: Filtering Database ID in reg: TRU Table Register 0 */
#define TRU_TTR0_FID_MASK                     WBGEN2_GEN_MASK(0, 8)
#define TRU_TTR0_FID_SHIFT                    0
#define TRU_TTR0_FID_W(value)                 WBGEN2_GEN_WRITE(value, 0, 8)
#define TRU_TTR0_FID_R(reg)                   WBGEN2_GEN_READ(reg, 0, 8)

/* definitions for field: ID withing Filtering Database Entry in reg: TRU Table Register 0 */
#define TRU_TTR0_SUB_FID_MASK                 WBGEN2_GEN_MASK(8, 8)
#define TRU_TTR0_SUB_FID_SHIFT                8
#define TRU_TTR0_SUB_FID_W(value)             WBGEN2_GEN_WRITE(value, 8, 8)
#define TRU_TTR0_SUB_FID_R(reg)               WBGEN2_GEN_READ(reg, 8, 8)

/* definitions for field: Force TRU table sub-entry update in reg: TRU Table Register 0 */
#define TRU_TTR0_UPDATE                       WBGEN2_GEN_MASK(16, 1)

/* definitions for field: Entry Valid in reg: TRU Table Register 0 */
#define TRU_TTR0_MASK_VALID                   WBGEN2_GEN_MASK(17, 1)

/* definitions for field: Pattern Mode in reg: TRU Table Register 0 */
#define TRU_TTR0_PATRN_MODE_MASK              WBGEN2_GEN_MASK(24, 4)
#define TRU_TTR0_PATRN_MODE_SHIFT             24
#define TRU_TTR0_PATRN_MODE_W(value)          WBGEN2_GEN_WRITE(value, 24, 4)
#define TRU_TTR0_PATRN_MODE_R(reg)            WBGEN2_GEN_READ(reg, 24, 4)

/* definitions for register: TRU Table Register 1 */

/* definitions for field: Ingress Mask in reg: TRU Table Register 1 */
#define TRU_TTR1_PORTS_INGRESS_MASK           WBGEN2_GEN_MASK(0, 32)
#define TRU_TTR1_PORTS_INGRESS_SHIFT          0
#define TRU_TTR1_PORTS_INGRESS_W(value)       WBGEN2_GEN_WRITE(value, 0, 32)
#define TRU_TTR1_PORTS_INGRESS_R(reg)         WBGEN2_GEN_READ(reg, 0, 32)

/* definitions for register: TRU Table Register 2 */

/* definitions for field: Egress Mask in reg: TRU Table Register 2 */
#define TRU_TTR2_PORTS_EGRESS_MASK            WBGEN2_GEN_MASK(0, 32)
#define TRU_TTR2_PORTS_EGRESS_SHIFT           0
#define TRU_TTR2_PORTS_EGRESS_W(value)        WBGEN2_GEN_WRITE(value, 0, 32)
#define TRU_TTR2_PORTS_EGRESS_R(reg)          WBGEN2_GEN_READ(reg, 0, 32)

/* definitions for register: TRU Table Register 3 */

/* definitions for field: Egress Mask in reg: TRU Table Register 3 */
#define TRU_TTR3_PORTS_MASK_MASK              WBGEN2_GEN_MASK(0, 32)
#define TRU_TTR3_PORTS_MASK_SHIFT             0
#define TRU_TTR3_PORTS_MASK_W(value)          WBGEN2_GEN_WRITE(value, 0, 32)
#define TRU_TTR3_PORTS_MASK_R(reg)            WBGEN2_GEN_READ(reg, 0, 32)

/* definitions for register: TRU Table Register 4 */

/* definitions for field: Pattern Match in reg: TRU Table Register 4 */
#define TRU_TTR4_PATRN_MATCH_MASK             WBGEN2_GEN_MASK(0, 32)
#define TRU_TTR4_PATRN_MATCH_SHIFT            0
#define TRU_TTR4_PATRN_MATCH_W(value)         WBGEN2_GEN_WRITE(value, 0, 32)
#define TRU_TTR4_PATRN_MATCH_R(reg)           WBGEN2_GEN_READ(reg, 0, 32)

/* definitions for register: TRU Table Register 5 */

/* definitions for field: Patern Mask in reg: TRU Table Register 5 */
#define TRU_TTR5_PATRN_MASK_MASK              WBGEN2_GEN_MASK(0, 32)
#define TRU_TTR5_PATRN_MASK_SHIFT             0
#define TRU_TTR5_PATRN_MASK_W(value)          WBGEN2_GEN_WRITE(value, 0, 32)
#define TRU_TTR5_PATRN_MASK_R(reg)            WBGEN2_GEN_READ(reg, 0, 32)

/* definitions for register: Debug port select */

/* definitions for field: Port ID in reg: Debug port select */
#define TRU_DPS_PID_MASK                      WBGEN2_GEN_MASK(0, 8)
#define TRU_DPS_PID_SHIFT                     0
#define TRU_DPS_PID_W(value)                  WBGEN2_GEN_WRITE(value, 0, 8)
#define TRU_DPS_PID_R(reg)                    WBGEN2_GEN_READ(reg, 0, 8)

/* definitions for register: Packet Injection Debug Register */

/* definitions for field: Injection Request in reg: Packet Injection Debug Register */
#define TRU_PIDR_INJECT                       WBGEN2_GEN_MASK(0, 1)

/* definitions for field: Packet Select in reg: Packet Injection Debug Register */
#define TRU_PIDR_PSEL_MASK                    WBGEN2_GEN_MASK(1, 3)
#define TRU_PIDR_PSEL_SHIFT                   1
#define TRU_PIDR_PSEL_W(value)                WBGEN2_GEN_WRITE(value, 1, 3)
#define TRU_PIDR_PSEL_R(reg)                  WBGEN2_GEN_READ(reg, 1, 3)

/* definitions for field: USER VALUE in reg: Packet Injection Debug Register */
#define TRU_PIDR_UVAL_MASK                    WBGEN2_GEN_MASK(8, 16)
#define TRU_PIDR_UVAL_SHIFT                   8
#define TRU_PIDR_UVAL_W(value)                WBGEN2_GEN_WRITE(value, 8, 16)
#define TRU_PIDR_UVAL_R(reg)                  WBGEN2_GEN_READ(reg, 8, 16)

/* definitions for field: Injection Ready in reg: Packet Injection Debug Register */
#define TRU_PIDR_IREADY                       WBGEN2_GEN_MASK(24, 1)

/* definitions for register: Packet Filter Debug Register */

/* definitions for field: Clear register in reg: Packet Filter Debug Register */
#define TRU_PFDR_CLR                          WBGEN2_GEN_MASK(0, 1)

/* definitions for field: Filtered class in reg: Packet Filter Debug Register */
#define TRU_PFDR_CLASS_MASK                   WBGEN2_GEN_MASK(8, 8)
#define TRU_PFDR_CLASS_SHIFT                  8
#define TRU_PFDR_CLASS_W(value)               WBGEN2_GEN_WRITE(value, 8, 8)
#define TRU_PFDR_CLASS_R(reg)                 WBGEN2_GEN_READ(reg, 8, 8)

/* definitions for field: CNT in reg: Packet Filter Debug Register */
#define TRU_PFDR_CNT_MASK                     WBGEN2_GEN_MASK(16, 16)
#define TRU_PFDR_CNT_SHIFT                    16
#define TRU_PFDR_CNT_W(value)                 WBGEN2_GEN_WRITE(value, 16, 16)
#define TRU_PFDR_CNT_R(reg)                   WBGEN2_GEN_READ(reg, 16, 16)

PACKED struct TRU_WB {
  /* [0x0]: REG TRU Global Control Register */
  uint32_t GCR;
  /* [0x4]: REG TRU Global Status Register 0 */
  uint32_t GSR0;
  /* [0x8]: REG TRU Global Status Register 1 */
  uint32_t GSR1;
  /* [0xc]: REG Pattern Control Register */
  uint32_t MCR;
  /* [0x10]: REG Link Aggregation Control Register */
  uint32_t LACR;
  /* [0x14]: REG Transition Control General Register */
  uint32_t TCGR;
  /* [0x18]: REG Transition Control Port Register */
  uint32_t TCPR;
  /* [0x1c]: REG Transition Status Register */
  uint32_t TSR;
  /* [0x20]: REG Real Time Reconfiguration Control Register */
  uint32_t RTRCR;
  /* [0x24]: REG TRU Table Register 0 */
  uint32_t TTR0;
  /* [0x28]: REG TRU Table Register 1 */
  uint32_t TTR1;
  /* [0x2c]: REG TRU Table Register 2 */
  uint32_t TTR2;
  /* [0x30]: REG TRU Table Register 3 */
  uint32_t TTR3;
  /* [0x34]: REG TRU Table Register 4 */
  uint32_t TTR4;
  /* [0x38]: REG TRU Table Register 5 */
  uint32_t TTR5;
  /* [0x3c]: REG Debug port select */
  uint32_t DPS;
  /* [0x40]: REG Packet Injection Debug Register */
  uint32_t PIDR;
  /* [0x44]: REG Packet Filter Debug Register */
  uint32_t PFDR;
};

#endif
