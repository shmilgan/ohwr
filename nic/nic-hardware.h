#ifndef __WR_NIC_HARDWARE_H__
#define __WR_NIC_HARDWARE_H__

/* Our host CPU is this one, no way out of it */
#include <mach/at91sam9263.h>

#include "wr_nic.wb.h" /* wbgen2 file, with local fixes (64KB of area) */

/* FIXME: This one is wrong, should stack on wr_vic.c instead */
#define WRN_INTERRUPT		AT91SAM9263_ID_IRQ0

/* Physical memory addresses. Should we import from other places? */

/* FIXME: Where is the wr_nic 64kB area in all this? */

/* The following is fake, just to make stuff compile - FIXME FIXME FIXME */
#define FPGA_BASE_NIC           _FPGA_BUS_2_ADDR(4)
#define FPGA_BASE_PPSG          _FPGA_BUS_2_ADDR(5)
#define FPGA_BASE_CALIB         _FPGA_BUS_2_ADDR(6)
#define FPGA_BASE_STAMP         _FPGA_BUS_2_ADDR(7)
#define FPGA_BASE_EP(x)         _FPGA_BUS_2_ADDR(8+(x))


/* Following defines come straight from board-whiterabbit-mch.c */
#define _FPGA_BUS_2_ADDR(bus) (0x70000000 + (bus) * 0x10000)

#define FPGA_BASE_REVID     _FPGA_BUS_2_ADDR(0)
#define FPGA_BASE_GPIO      _FPGA_BUS_2_ADDR(1)
#define FPGA_BASE_SPIM      _FPGA_BUS_2_ADDR(2)
#define FPGA_BASE_VIC       _FPGA_BUS_2_ADDR(3)
#define FPGA_BASE_MINIC_UP1 _FPGA_BUS_2_ADDR(4)
#define FPGA_BASE_MINIC_UP0 _FPGA_BUS_2_ADDR(5)
#define FPGA_BASE_MINIC_DP0 _FPGA_BUS_2_ADDR(6)
#define FPGA_BASE_MINIC_DP1 _FPGA_BUS_2_ADDR(7)
#define FPGA_BASE_MINIC_DP2 _FPGA_BUS_2_ADDR(8)
#define FPGA_BASE_MINIC_DP3 _FPGA_BUS_2_ADDR(9)
#define FPGA_BASE_MINIC_DP4 _FPGA_BUS_2_ADDR(10)
#define FPGA_BASE_MINIC_DP5 _FPGA_BUS_2_ADDR(11)
#define FPGA_BASE_MINIC_DP6 _FPGA_BUS_2_ADDR(12)
#define FPGA_BASE_MINIC_DP7    _FPGA_BUS_2_ADDR(13)
#define FPGA_BASE_PPS_GEN      _FPGA_BUS_2_ADDR(14)
#define FPGA_BASE_CALIBRATOR   _FPGA_BUS_2_ADDR(15)
#define FPGA_BASE_RTU          _FPGA_BUS_2_ADDR(16)
#define FPGA_BASE_RTU_TESTUNIT _FPGA_BUS_2_ADDR(17)

#define WR_MINIC_SIZE     0x10000

#endif /* __WR_NIC_HARDWARE_H__ */
