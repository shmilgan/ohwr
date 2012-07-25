#include "defs.h"
#include "uart.h"
#include "timer.h"

#include "softpll_ng.h"

#include "minipc.h"

const char *build_revision;
const char *build_date;

main()
{
	uint32_t start_tics = 0;

	uart_init();
	
	TRACE("WR Switch Real Time Subsystem (c) CERN 2011-2012\n");
	TRACE("Revision: %s, built at %s.\n", build_revision, build_date);
	TRACE("--");

	ad9516_init();
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
