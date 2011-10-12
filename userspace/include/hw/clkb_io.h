#ifndef __CLKB_H
#define __CLKB_H

#include <inttypes.h>


#define _CLKB_WB_BUS(x) ((x)<<8)

/* Base addresses of the Timing FPGA wishbone peripherals */
#define CLKB_BASE_REVID _CLKB_WB_BUS(0)
#define CLKB_BASE_HPLL  _CLKB_WB_BUS(1)
#define CLKB_BASE_GPIO  _CLKB_WB_BUS(2)
#define CLKB_BASE_DMPLL  _CLKB_WB_BUS(3)
#define CLKB_BASE_CALIBRATOR  _CLKB_WB_BUS(4)


#define CLKB_REG_IDCODE 		0

int shw_clkb_init();
int shw_clkb_init_cmi();

//void shw_clkb_dac_write(int dac, int val);

#endif
