#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>


#include "serial.h"
#include "nmea.h"

#include <hw/switch_hw.h>

void read_nmea_msg(char *msgbuf, int len)
{
	int i=0;
	char c;
	
	while((c = serial_read_byte()) != '$');
		*msgbuf++ = c;
	while((c = serial_read_byte()) != '\r' && (i++) < len-1)
		*msgbuf++ = c;
	*msgbuf++ = '\r';
	*msgbuf++ = '\n';

	*msgbuf++ = 0 ;
}

static int nmea_read_tai(const char *dev_name, int64_t *t_out)
{
		char buf[1024];
  	nmea_gprmc_t gprmc;

		serial_open((char *)dev_name, 115200);
		read_nmea_msg(buf, 1024);
		serial_close();
	
    if(nmea_parse_gprmc(buf, strlen(buf), &gprmc) < 0)
    	return -1;


		TRACE(TRACE_INFO, "NMEA time: %d/%d/%d %02d:%02d:%02d.%02d\n", gprmc.utc.year + 1900,gprmc.utc.mon+1,gprmc.utc.day,gprmc.utc.hour, gprmc.utc.min, gprmc.utc.sec,gprmc.utc.hsec);

		*t_out = nmea_time_to_tai(gprmc.utc);

		return 0;
}

void nmea_resync_ppsgen(const char *dev_name)
{
	uint32_t utc, nsec;
	int64_t tai;
	
	nmea_read_tai("/dev/ttyS2", &tai);
	shw_pps_gen_read_time(	&utc, &nsec);

	shw_pps_gen_adjust_utc(tai - utc);

	while(!shw_pps_gen_busy()) usleep(100000);
}

