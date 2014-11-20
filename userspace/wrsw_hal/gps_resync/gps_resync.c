#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>


#include "serial.h"
#include "nmea.h"

#include <libwr/switch_hw.h>

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


		TRACE(TRACE_INFO, "NMEA time: %d/%d/%d %02d:%02d:%02d.%02d\n", gprmc.time.year + 1900,gprmc.time.mon+1,gprmc.time.day,gprmc.time.hour, gprmc.time.min, gprmc.time.sec,gprmc.time.hsec);

		*t_out = nmea_time_to_tai(gprmc.time);

		return 0;
}

void nmea_resync_ppsgen(const char *dev_name)
{
	uint32_t nsec;
	uint64_t cur_tai, new_tai;
	
	nmea_read_tai("/dev/ttyS2", (int64_t *)&new_tai);
	shw_pps_gen_read_time(	&cur_tai, &nsec);

	shw_pps_gen_adjust(PPSG_ADJUST_SEC, (int64_t) new_tai - (int64_t) cur_tai);

	while(!shw_pps_gen_busy()) usleep(100000);
}

