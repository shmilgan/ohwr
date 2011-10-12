/* Step-by-step switch hardware initialization */

#include <stdio.h>
#include <stdlib.h>

#include <hw/switch_hw.h>

#define assert_init(proc) { int ret; if((ret = proc) < 0) return ret; }

int shw_init()
{
	/* Map the the FPGA memory space */
    assert_init(shw_fpga_mmap_init());
  /* Map the CPU GPIO memory space */
    assert_init(shw_pio_mmap_init());
  /* Initialize the AD9516 and the clock distribution. Now we can start accessing the FPGAs. */
    assert_init(shw_ad9516_init());
  /* Initialize the main FPGA */
    assert_init(shw_main_fpga_init());
  /* Sets up the directions of all CPU GPIO pins */
    assert_init(shw_pio_configure_all_cpu_pins());
  /* Initialize the bootloader and try to boot up the Main FPGA. */
    assert_init(shw_fpgaboot_init());
    assert_init(shw_boot_fpga(FPGA_ID_MAIN));
  /* Initialize the main FPGA and its GPIO controller */
    assert_init(shw_main_fpga_init());
    assert_init(shw_pio_configure_all_fpga_pins());
	/* Initialize the CMI link prior to booting the CLKB FPGA, so the FPGA bootloader could check the REVID. */
    assert_init(shw_clkb_init_cmi()); 
	/* Boot up and init the Timing FPGA */
    assert_init(shw_boot_fpga(FPGA_ID_CLKB));
	  assert_init(shw_clkb_init());
  /* Start-up the PLLs. */
    assert_init(shw_hpll_init());
    assert_init(shw_dmpll_init());
  /* Initialize the calibrator (requires the DMTD clock to be operational) */
    assert_init(shw_cal_init());
  /* Start-up the PPS generator */
    assert_init(shw_pps_gen_init());
  /* ... and the SPI link with the watchdog */
    assert_init(shw_watchdog_init());

	TRACE(TRACE_INFO, "HW initialization done!");
}

int shw_exit_fatal()
{
	TRACE(TRACE_FATAL, "exiting due to fatal error.");
	exit(-1);
}

#undef assert_init
