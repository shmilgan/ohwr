/* Port initialization and state machine */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include <libwr/switch_hw.h>

#include "wrsw_hal.h"
#include "timeout.h"
#include <rt_ipc.h>
#include <hal/hal_exports.h>

#include "gps_resync/gps_resync.h"

static int timing_mode;

#define LOCK_TIMEOUT_EXT 60000
#define LOCK_TIMEOUT_INT 10000

int hal_init_timing()
{
	char str[128];
	timeout_t lock_tmo;
	int use_utc = 0;
	
	if(rts_connect() < 0)
	{
		TRACE(TRACE_ERROR, "Failed to establish communication with the RT subsystem.");
		return -1;	
	}

  if( hal_config_get_string("timing.mode", str, sizeof(str)) < 0)
  {
  	TRACE(TRACE_INFO,"Not timing mode specified in the config file. Defaulting to Boundary Clock.");
  	timing_mode = HAL_TIMING_MODE_BC;
  	strcpy (str, "BoundaryClock");
  } else {
  	if(!strcasecmp(str, "GrandMaster") || !strcasecmp(str, "GM"))
  		timing_mode = HAL_TIMING_MODE_GRAND_MASTER;
  	else if (!strcasecmp(str, "FreeMaster") || !strcasecmp(str, "FM"))
  		timing_mode = HAL_TIMING_MODE_FREE_MASTER;
  	else if (!strcasecmp(str, "BoundaryClock") || !strcasecmp(str,"BC"))
  		timing_mode = HAL_TIMING_MODE_BC;
  	else {
  		TRACE(TRACE_ERROR, "Unrecognized timing mode '%s'", str);
  		return -1;
  	}
  }

	TRACE(TRACE_INFO, "Timing mode: %s", str);
  
  /* initialize the RT Subsys */
	switch(timing_mode)
	{
		case HAL_TIMING_MODE_GRAND_MASTER:
			rts_set_mode(RTS_MODE_GM_EXTERNAL);
			tmo_init(&lock_tmo, LOCK_TIMEOUT_EXT, 0);
			break;			

		case HAL_TIMING_MODE_FREE_MASTER:
		case HAL_TIMING_MODE_BC:
			rts_set_mode(RTS_MODE_GM_FREERUNNING);
			tmo_init(&lock_tmo, LOCK_TIMEOUT_INT, 0);
			break;
	}

	while(1)
	{
		struct rts_pll_state pstate;

		if(tmo_expired(&lock_tmo))
		{
			TRACE(TRACE_ERROR, "Can't lock the PLL. If running in the GrandMaster mode, are you sure the 1-PPS and 10 MHz reference clock signals are properly connected?, retrying...");
			if(timing_mode == HAL_TIMING_MODE_GRAND_MASTER) {
				/*ups... something went wrong, but instead of giving-up, try one more time*/
				rts_set_mode(RTS_MODE_GM_EXTERNAL);
				tmo_init(&lock_tmo, LOCK_TIMEOUT_EXT, 0);
			}
			else
				return -1;
		}

		if(rts_get_state(&pstate) < 0)
			return -1;

		if(pstate.flags & RTS_DMTD_LOCKED)
			break;

		usleep(100000);
	}


	if(hal_config_get_int("timing.use_nmea", &use_utc) < 0)
		use_utc = 0;
	
	if(timing_mode == HAL_TIMING_MODE_GRAND_MASTER && use_utc)
	{
		TRACE(TRACE_INFO, "re-syncing to UTC from serial port");
		nmea_resync_ppsgen("/dev/ttyS2");
	}
			
	return 0;
}

int hal_get_timing_mode()
{
	return timing_mode;
}
