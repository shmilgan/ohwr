#include <stdio.h>
#include <unistd.h>

#include "cpu_io.h"
#include "mb_io.h"

#define N_LEDS 8 

struct {
  int port;
    int pin;
    } leds[] = {
      {PIOC, 5},
        {PIOC, 12},
          {PIOC, 7},
            {PIOC, 19},
              {PIOC, 9},
                {PIOC, 17},
                  {PIOC, 14},
                    {PIOC, 6},
                      {0, 0}
                      };
                      

void blink_leds()
{
  int i;

  for(i=0;i<N_LEDS;i++)
	pio_set_mode(leds[i].port, leds[i].pin, PIN_MODE_GPIO, 1);

  i=0;
  for(;;)
  {
	int prevled=((i-1)<0)?(N_LEDS-1):i-1;

	pio_set_state(leds[prevled].port, leds[prevled].pin, 0);
	pio_set_state(leds[i].port, leds[i].pin, 1);
	
	i++; if(i>=N_LEDS) i = 0;
    usleep(100000);
  }

}

extern int mblaster_boot_fpga(const char *filename, int fpga_sel);


main(int argc, char *argv[])
{
  int fpga_sel = -1;
  io_init();
  pck_enable(0, AT91_PMC_CSS_PLLA, AT91_PMC_PRES_2);

  if(argc<3)
  {
    fprintf(stderr,"Usage: %s <bitstream_file> <MAIN/CLKB>\n\n",argv[0]);
    return -1;
  }

  if(!strcasecmp(argv[2], "main"))
    fpga_sel = FPGA_MAIN;
  else if(!strcasecmp(argv[2], "clkb"))
    fpga_sel = FPGA_CLKB;

  fprintf(stderr,"FPGAsel: %d\n", fpga_sel);

  mblaster_boot_fpga(argv[1], fpga_sel);
  return 0;
}
