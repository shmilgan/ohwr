/* Main HAL file */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <libwr/wrs-msg.h>
#include <libwr/shmem.h>
#include <libwr/hist_shmem.h>
#include <libwr/util.h>
#include "wrs_hist.h"

#define PORT_FAN_MS_PERIOD 250

struct hist_shmem_data *hist_shmem;
struct wrs_shm_head *hist_shmem_hdr;

/* Interates via all the ports defined in the config file and
 * intializes them one after another. */
static int hist_shmem_init(void)
{
	pr_debug("Initializing shmem\n");

	/* Allocate the ports in shared memory, so wr_mon etc can see them
	 * Use lock since some might wait for it int he future to be
	 * available */
	hist_shmem_hdr = wrs_shm_get(wrs_shm_hist, "wrs_hist",
				     WRS_SHM_WRITE | WRS_SHM_LOCKED);
	if (!hist_shmem_hdr) {
		pr_error("Can't join shmem: %s\n", strerror(errno));
		return -1;
	}
	hist_shmem = wrs_shm_alloc(hist_shmem_hdr, sizeof(*hist_shmem));
	if (!hist_shmem) {
		pr_error("Can't allocate in shmem\n");
		return -1;
	}

	/* TODO: clear allocated memory? */
	hist_shmem_hdr->version = HIST_SHMEM_VERSION;
	/* Release processes waiting for wrs_hist's to fill shm with correct
	 * data. When shm is opened successfully data in shm is still not
	 * populated! Read data with wrs_shm_seqbegin and wrs_shm_seqend! */
	wrs_shm_write(hist_shmem_hdr, WRS_SHM_WRITE_END);

	return 0;
}

static void show_help(void)
{
	printf("WR Switch history daemon (wrsw_hist)\n"
	       "Usage: wrsw_hist [options]\n"
	       "  where [options] can be:\n"
	       "    -h         : display help\n"
	       "    -q         : decrease verbosity\n"
	       "    -v         : increase verbosity\n"
	       "\n");
}

static void hist_parse_cmdline(int argc, char *argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, "qv")) != -1) {
		switch (opt) {
		case 'h':
			show_help();
			exit(0);
			break;
		case 'q': break; /* done in wrs_msg_init() */
		case 'v': break; /* done in wrs_msg_init() */

		default:
			pr_error("Unrecognized option. Call %s -h for help.\n",
				 argv[0]);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	uint64_t t1, t2;
	wrs_msg_init(argc, argv);

	/* Print HAL's version */
	pr_info("Commit %s, built on " __DATE__ "\n", __GIT_VER__);

	hist_parse_cmdline(argc, argv);
	if (hist_check_running()) {
		pr_error("Fatal: There is another wrs_hist instance running. "
			 "We can't work together.\n");
		return 1;
	}
	assert_init(hist_shmem_init());
	assert_init(hist_wripc_init());
 

	/*
	 * Main loop update - polls for WRIPC requests and rolls the port
	 * state machines. This is not a busy loop, as wripc waits for
	 * the "max ms delay". Unless an RPC call comes, in which
	 * case it returns earlier.
	 *
	 * We thus check the actual time, and only proceed with
	 * port and fan update every PORT_FAN_MS_PERIOD.  There still
	 * is some jitter from hal_update_wripc() timing.
	 * includes some jitter.
	 */

	/* TODO: make sure only one copy of wrs_hist is running */

	t1 = get_monotonic_tics();
	for (;;) {
		int delay_ms;

		hist_wripc_update(1000 /* max ms delay */);

		t2 = get_monotonic_tics();
		delay_ms = (t2 - t1) * 1000;
		if (delay_ms < PORT_FAN_MS_PERIOD)
			continue;

// 		wrs_shm_write(hal_shmem, WRS_SHM_WRITE_BEGIN);
// 		wrs_shm_write(hal_shmem, WRS_SHM_WRITE_END);
		t1 = t2;
	}

	return 0;
}
