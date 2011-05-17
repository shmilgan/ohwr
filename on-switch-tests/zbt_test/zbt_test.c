/* ZBT test program */
/* Firmware. MAIN: zbt_test, CLKB: new_wishbone_bridge */


#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/time.h>

#include <hw/switch_hw.h>

#define BASE_ZBT 0x40000

void test_the_zbt()
{
	int i;
	
	int iter=0;
	
	for(;;)
	{
	
//write some random data
			srand(iter*1231931+31231);
 		for(i=0;i<32768;i++)
 		{
			_fpga_writel(BASE_ZBT + i*4, random()&0xffffffff);
		}

// read it out, and verify
			srand(iter*1231931+31231);
 		for(i=0;i<32768;i++)
 		{
			uint32_t rv = _fpga_readl(BASE_ZBT + i*4);
			uint32_t v = random()&0xffffffff;
			if(v!=rv) { printf("ERROR: V %08x RV  %08x XOR %08x\n", v,rv,v^rv); }
		}
		iter++;
		fprintf(stderr,".");
	}
	
}

main()
{
  
	trace_log_stderr();
	shw_init();
 
	printf("Testing the ZBT memory...");
	test_the_zbt();
	
	return 0;
}
