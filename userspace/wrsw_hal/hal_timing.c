/* Port initialization and state machine */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include <libwr/switch_hw.h>
#include <libwr/config.h>
#include <libwr/wrs-msg.h>

#include "wrsw_hal.h"
#include "timeout.h"
#include <rt_ipc.h>
#include <hal/hal_exports.h>

static int timing_mode;

#define LOCK_TIMEOUT_EXT 60000
#define LOCK_TIMEOUT_INT 10000

int hal_init_timing(char *filename)
{
	timeout_t lock_tmo;
	static struct {
		char *cfgname;
		int modevalue;
	} *m, modes[] = {
		{"TIME_GM", HAL_TIMING_MODE_GRAND_MASTER},
		{"TIME_FM", HAL_TIMING_MODE_FREE_MASTER},
		{"TIME_BC", HAL_TIMING_MODE_BC},
		{NULL, HAL_TIMING_MODE_BC /* default */},
	};

	if (rts_connect(NULL) < 0) {
		pr_error(
		      "Failed to establish communication with the RT subsystem.\n");
		return -1;
	}

	for (m = modes; m->cfgname; m++)
		if (libwr_cfg_get(m->cfgname))
			break;
	timing_mode = m->modevalue;

	if (!m->cfgname)
		pr_error("%s: no config variable set, defaults used\n",
			__func__);

	/* initialize the RT Subsys */
	switch (timing_mode) {
	case HAL_TIMING_MODE_GRAND_MASTER:
		rts_set_mode(RTS_MODE_GM_EXTERNAL);
		tmo_init(&lock_tmo, LOCK_TIMEOUT_EXT, 0);
		break;

	case HAL_TIMING_MODE_FREE_MASTER:
	case HAL_TIMING_MODE_BC:
	default: /* never hit, but having it here prevents a warning */
		rts_set_mode(RTS_MODE_GM_FREERUNNING);
		tmo_init(&lock_tmo, LOCK_TIMEOUT_INT, 0);
		break;
	}

	while (1) {
		struct rts_pll_state pstate;

		if (tmo_expired(&lock_tmo)) {
			pr_error("Can't lock the PLL. "
			      "If running in the GrandMaster mode, "
			      "are you sure the 1-PPS and 10 MHz "
			      "reference clock signals are properly connected?,"
			      " retrying...\n");
			if (timing_mode == HAL_TIMING_MODE_GRAND_MASTER) {
				/*ups... something went wrong, try again */
				rts_set_mode(RTS_MODE_GM_EXTERNAL);
				tmo_init(&lock_tmo, LOCK_TIMEOUT_EXT, 0);
			} else {
				pr_error("Got timeout\n");
				return -1;
			}
		}

		if (rts_get_state(&pstate) < 0) {
			/* Don't give up when rts_get_state fails, it may be
			 * due to race with ppsi at boot. No problems seen
			 * because of waiting here. */
			pr_error("rts_get_state failed try again\n");
			continue;
		}

		if (pstate.flags & RTS_DMTD_LOCKED) {
			if (timing_mode == HAL_TIMING_MODE_GRAND_MASTER)
				pr_info("GrandMaster locked to external "
						"reference\n");
			break;
		}

		usleep(100000);
	}

	/*
	 * We had "timing.use_nmea", but it was hardwired to /dev/ttyS2
	 * which is not wired out any more, so this is removed after v4.1
	 */

	return 0;
}

int hal_get_timing_mode()
{
	return timing_mode;
}
