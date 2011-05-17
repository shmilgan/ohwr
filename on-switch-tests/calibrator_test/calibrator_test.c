#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sys/time.h>
#include <signal.h>

#include <hw/switch_hw.h>
#include <hw/clkb_io.h>
#include <hw/minic_regs.h>
#include <hw/endpoint_regs.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
//#include <net/if.h>

#include <asm/types.h>

#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <linux/errqueue.h>
#include <sys/ioctl.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <errno.h>

#include <hw/dmtd_calibrator_regs.h>
#include <hw/endpoint_regs.h>
#include <hw/phy_calibration.h>



main()
{
  int i, rval;
	system("/sbin/rmmod /tmp/wr_minic.ko");
	system("/sbin/rmmod /tmp/wr_vic.ko");

  trace_log_stderr();
  shw_init();
 
	system("/sbin/insmod /tmp/wr_vic.ko");
	system("/sbin/insmod /tmp/wr_minic.ko");
	system("/sbin/ifconfig wru1 hw ether 00:50:fc:96:9b:0e");
	system("/sbin/ifconfig wru1 up 192.168.100.100");
  
  
  _fpga_writel(FPGA_BASE_MINIC_UP0 + (1<<14) + EP_REG_PHIO, EP_PHIO_ENABLE);
  _fpga_writel(FPGA_BASE_MINIC_UP1 + (1<<14) + EP_REG_PHIO, EP_PHIO_ENABLE);
  
  
  shw_hpll_switch_reference("wru1");
  while(!shw_hpll_check_lock());
  shw_dmpll_switch_reference("wru1");
  while(!shw_dmpll_check_lock());

  



	shw_cal_enable_feedback("wru1", 1, PHY_CALIBRATE_RX);
	double phase_rx;

	for(;;)
	{
	while(!(rval = shw_cal_measure(&phase_rx)));

	if(rval < 0)
	{
		printf("error measuring RX delay\n");
		return -1;
	}
	
	printf("rval = %d, deltaRX(uncorrected) = %.3f ns\n", rval, phase_rx);
	}
}

