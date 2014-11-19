#ifndef __LIBWR_SWITCH_HW_H
#define __LIBWR_SWITCH_HW_H

#include <libwr/pio.h>
#include <libwr/trace.h>
#include <libwr/pps_gen.h>
#include <libwr/fan.h>

/* Some global, very important constants */

/* System reference clock period, in picoseconds. 62.5 MHz clock in V3 gives 16000 ps period */
#define REF_CLOCK_PERIOD_PS 				16000

/* System reference clock rate (in Hertz). Update together with REF_CLOCK_PERIOD_PS */
#define REF_CLOCK_RATE_HZ						62500000

/* return 0 on success */
int shw_init(void);
int shw_fpga_mmap_init(void);
#endif /* __LIBWR_SWITCH_HW_H */
