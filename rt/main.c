#include "defs.h"
#include "uart.h"
#include "timer.h"

#include "softpll_ng.h"

#include "minipc.h"

#ifdef SCB_VERSION
#define SCB_VERSION 34
#endif

const char *build_revision;
const char *build_date;

int scb_ver = SCB_VERSION;		//SCB version.

main()
{
	uint32_t start_tics = 0;

	uart_init();
	
	TRACE("WR Switch Real Time Subsystem (c) CERN 2011 - 2014\n");
	TRACE("Revision: %s, built %s.\n", build_revision, build_date);

	if ( scb_ver >= 34 )
		TRACE("SCB version %d. 10 MHz SMC Output.\n", scb_ver );
	else
		TRACE("SCB version %d.\n", scb_ver );

	TRACE("--");

	ad9516_init( scb_ver );
	rts_init();
	rtipc_init();

	for(;;)
	{
			uint32_t tics = timer_get_tics();
			
			if(tics - start_tics > TICS_PER_SECOND)
			{
				spll_show_stats();
				start_tics = tics;
			}
	    rts_update();
	    rtipc_action();
	}

	return 0;
}
