/* Step-by-step switch hardware initialization */

#include <stdio.h>
#include <stdlib.h>

#include <switch_hw.h>
#include "i2c_sfp.h"

#define assert_init(proc) { int ret; if((ret = proc) < 0) return ret; }

int shw_init()
{
    /* Map the the FPGA memory space */
    assert_init(shw_fpga_mmap_init());
    /* Map CPU's ping into memory space */
    assert_init(shw_pio_mmap_init());
    shw_pio_configure_all();
		
		assert_init(shw_sfp_buses_init());
		shw_sfp_gpio_init();

		assert_init(shw_init_fans());

	TRACE(TRACE_INFO, "HW initialization done!");
}

int shw_exit_fatal()
{
	TRACE(TRACE_FATAL, "exiting due to fatal error.");
	exit(-1);
}

#undef assert_init
