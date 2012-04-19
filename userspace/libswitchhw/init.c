/* Step-by-step switch hardware initialization */

#include <stdio.h>
#include <stdlib.h>

#include <hw/switch_hw.h>

#define assert_init(proc) { int ret; if((ret = proc) < 0) return ret; }

int shw_init()
{
    /* Map the the FPGA memory space */
    assert_init(shw_fpga_mmap_init());
    /* Map CPU's ping into memory space */
    assert_init(shw_pio_mmap_init());
    shw_pio_configure_all();
  /* Initialize the AD9516 and the clock distribution. Now we can start accessing the FPGAs. */
  //    assert_init(shw_pps_gen_init());
  /* ... and the SPI link with the watchdog */
    /* no more shw_watchdog_init(); */

	TRACE(TRACE_INFO, "HW initialization done!");
}

int shw_exit_fatal()
{
	TRACE(TRACE_FATAL, "exiting due to fatal error.");
	exit(-1);
}

#undef assert_init
