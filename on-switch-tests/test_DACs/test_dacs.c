#include <stdio.h>

#include <hw/switch_hw.h>
#include <hw/clkb_io.h>
//#include <hw/fpga_io.h>


main()
{
  int i=0,di=255,previ=0;

	trace_log_stderr();
	shw_init();

#if 0
	xpoint_test();

for(;;)
{
	shw_clkb_dac_write(DAC_REF, i);
	shw_clkb_dac_write(DAC_DMTD, i);

	i+=di;
	
	if(i<0) {i=0;di=-di;}
	else if(i>65530) {i=65530;di=-di;}
	

}
#endif
	shw_clkb_dac_write(DAC_REF, 30000);
	shw_clkb_dac_write(DAC_DMTD,30000);

	shw_ad9516_set_output_delay(9, 3, 0, 20, 0);


	_fpga_writel(FPGA_BASE_GIGASPY_UP1 + GSPY_REG_GSTESTCTL, GSPY_GSTESTCTL_ENABLE | GSPY_GSTESTCTL_CONNECT | GSPY_GSTESTCTL_PHYIO_ENABLE | GSPY_GSTESTCTL_PHYIO_SYNCEN ) ;

	printf("TREG: %x\n", _fpga_readl(FPGA_BASE_GIGASPY_UP1 + GSPY_REG_GSTESTCTL));

for(;;)
{
	printf("ECnt: %d\n", _fpga_readl(FPGA_BASE_GIGASPY_UP1 + GSPY_REG_GSTESTCNT));
}

}
