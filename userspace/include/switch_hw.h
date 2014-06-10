#ifndef __SWITCH_HW_H
#define __SWITCH_HW_H

#include "pio.h"
#include "trace.h"
#include "pps_gen.h"
#include "fan.h"

/* Some global, very important constants */

/* System reference clock period, in picoseconds. 62.5 MHz clock in V3 gives 16000 ps period */
#define REF_CLOCK_PERIOD_PS 				16000

/* System reference clock rate (in Hertz). Update together with REF_CLOCK_PERIOD_PS */
#define REF_CLOCK_RATE_HZ						62500000

int shw_init(void);
int shw_fpga_mmap_init(void);
#endif
