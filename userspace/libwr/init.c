/* Step-by-step switch hardware initialization */

#include <stdio.h>
#include <stdlib.h>

#include <libwr/switch_hw.h>
#include "i2c_sfp.h"
#include <libwr/shw_io.h>

int shw_init()
{
	TRACE(TRACE_INFO,
	      "%s\n=========================================================",
	      __TIME__);

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

	TRACE(TRACE_INFO, "HW initialization done!");
	return 0;
}

int shw_exit_fatal()
{
	TRACE(TRACE_FATAL, "exiting due to fatal error.");
	exit(-1);
}
