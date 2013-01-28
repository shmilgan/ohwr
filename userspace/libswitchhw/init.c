/* Step-by-step switch hardware initialization */

#include <stdio.h>
#include <stdlib.h>

#include <switch_hw.h>
#include "i2c_sfp.h"
#include "shw_io.h"

#define assert_init(proc) { int ret; if((ret = proc) < 0) return ret; }

int shw_init()
{
	/* Init input/output (GPIO & CPU I2C) */
	assert_init(shw_io_init());

	/* Map the the FPGA memory space */
	assert_init(shw_fpga_mmap_init());

	/* Configure all (GPIO & CPU I2C) */
	shw_io_configure_all();

	/* Initialize SFP Buses (FPGA I2C) */
	assert_init(shw_sfp_buses_init());

	/* Init the SFP GPIO */
	shw_sfp_gpio_init();

	/* Init the FANs */
	assert_init(shw_init_fans());

	/* Finally setup the green led */
	shw_io_write(shw_io_led_state_o,0);
	shw_io_write(shw_io_led_state_g,1);

	TRACE(TRACE_INFO, "HW initialization done!");
}

int shw_exit_fatal()
{
	TRACE(TRACE_FATAL, "exiting due to fatal error.");
	exit(-1);
}

#undef assert_init
