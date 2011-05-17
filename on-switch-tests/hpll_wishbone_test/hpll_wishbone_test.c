#include <stdio.h>
#include <inttypes.h>
#include <sys/time.h>

#include <hw/switch_hw.h>


main()
{
  trace_log_stderr();
  shw_init();

// force the PHY enable/sync enable line HI
  _fpga_writel(FPGA_BASE_GIGASPY_UP1 | GSPY_REG_GSTESTCTL, GSPY_GSTESTCTL_PHYIO_ENABLE | GSPY_GSTESTCTL_PHYIO_SYNCEN);

// set up the crosspoint to pass the data from PHY to SFPs
  xpoint_configure();

  shw_hpll_init();
  
  for(;;)
    {
      printf("Locked: %d\n", shw_hpll_check_lock() ? 1: 0);
      shw_hpll_update();
    }

}
