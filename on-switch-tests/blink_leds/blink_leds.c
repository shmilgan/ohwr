#include <stdio.h>

#include <hw/switch_hw.h>

#define NUM_LEDS 8

main()
{
  unsigned int i=0,di=1,previ=0;


	trace_log_stderr();
	
  shw_request_fpga_firmware(FPGA_ID_MAIN, "board_test");
  shw_init();

  for(;;)
    {
      shw_set_fp_led(previ, LED_OFF);
      shw_set_fp_led(i, LED_GREEN);
      usleep(30000);
      shw_set_fp_led(i, LED_RED);
      usleep(30000);
      previ=i;
      i+=di;
      if(i == 0 || i==(NUM_LEDS-1)) di=-di;
    }
}
