#include <stdio.h>
#include <stdlib.h>

#include <hw/switch_hw.h>

#define assert_init(proc) { int ret; if((ret = proc) < 0) return ret; }



int shw_init()
{
    assert_init(shw_fpga_mmap_init());
    assert_init(shw_pio_mmap_init());
    assert_init(shw_main_fpga_init());
    assert_init(shw_ad9516_init());
    assert_init(shw_pio_configure_all_cpu_pins());
    assert_init(shw_fpgaboot_init());
    assert_init(shw_boot_fpga(FPGA_ID_MAIN));
    assert_init(shw_main_fpga_init());
    assert_init(shw_pio_configure_all_fpga_pins());
    assert_init(shw_clkb_init_cmi()); // Initialize the CMI link prior to booting the CLKB FPGA, so the FPGA bootloader could check the REVID
    assert_init(shw_boot_fpga(FPGA_ID_CLKB));
    assert_init(shw_clkb_init());
    assert_init(shw_hpll_init());
    assert_init(shw_dmpll_init());
    assert_init(shw_cal_init());
    assert_init(shw_pps_gen_init());
    assert_init(shw_watchdog_init());

//    shw_fpga_boot();
	TRACE(TRACE_INFO, "HW initialization done!");
}

int shw_exit_fatal()
{
	TRACE(TRACE_FATAL, "exiting due to fatal error.");
	exit(-1);
}

#undef assert_init
