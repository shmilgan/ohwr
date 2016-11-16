/* Main HAL file */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#include <libwr/wrs-msg.h>
#include <libwr/shmem.h>
#define HIST_SHMEM_STRUCTURES
#include <libwr/hist_shmem.h>
#include <libwr/util.h>
#include "wrs_hist.h"
#include "hist_crc.h"

struct hist_shmem_data *hist_shmem;
struct wrs_shm_head *hist_shmem_hdr;

static void sighandler(int sig);
static void wrs_exit(void);

/* Interates via all the ports defined in the config file and
 * intializes them one after another. */
int hist_shmem_init(void)
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
	printf("Commit %s, built on " __DATE__ "\n", __GIT_VER__);
}

static void hist_parse_cmdline(int argc, char *argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, "hqv")) != -1) {
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

int main(int argc, char *argv[]) __attribute__((weak));
int main(int argc, char *argv[])
{
	time_t t;
	time_t last_update_nand_s;
	time_t last_update_sfp_s;

	wrs_msg_init(argc, argv);

	/* Print wrs_hist's version */
	pr_info("Commit %s, built on " __DATE__ "\n", __GIT_VER__);

	hist_parse_cmdline(argc, argv);

	/* make sure only one copy of wrs_hist is running */
	if (hist_check_running()) {
		pr_error("Fatal: There is another wrs_hist instance running. "
			 "We can't work together.\n");
		return 1;
	}
	crc_init();
	hal_shm_init();
	assert_init(hist_shmem_init());
	assert_init(hist_wripc_init());
	assert_init(hist_up_spi_init());
	/* uses histograms read from spi, call hist_up_spi_init before
	 * hist_up_nand_init */
	assert_init(hist_up_nand_init()); 
	/* If HAL was running before add all SFPs to the local database */
	/* hist_sfp_init, may update lifetime information based on a value
	 * written in the sfp DB */
	assert_init(hist_sfp_init());
	
	signal(SIGSEGV, sighandler);
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);
	signal(SIGILL, sighandler);

	/* update data in the shmem and write them back */
	hist_sfp_nand_save();

	/*
	 * Main loop update - polls for WRIPC requests and rolls the port
	 * state machines. This is not a busy loop, as wripc waits for
	 * the "max ms delay". Unless an RPC call comes, in which
	 * case it returns earlier.
	 *
	 * We thus check the actual time, and only proceed with update
	 * functions every defined period. There still is some jitter from all
	 * called functions.
	 */

	t = get_monotonic_sec();
	last_update_nand_s = t;
	last_update_sfp_s = t;
	for (;;) {
		t = get_monotonic_sec();

		if (last_update_nand_s + NAND_UPDATE_PERIOD <= t) {
			last_update_nand_s += NAND_UPDATE_PERIOD;
			hist_up_nand_save();
		}

		if ((hist_shmem->hist_up_spi.lifetime + SPI_UPDATE_PERIOD)
		    <= hist_up_lifetime_get()) {
			hist_up_spi_save();
		}

		if (last_update_sfp_s + SFP_UPDATE_PERIOD <= t) {
			last_update_sfp_s += SFP_UPDATE_PERIOD;
			hist_sfp_nand_save();
		}

		hist_wripc_update(1000 /* max ms delay */);
	}

	return 0;
}

static void sighandler(int sig)
{
	pr_error("signal caught (%d)!\n", sig);
	wrs_exit();
	exit(0);
}

/* function to be called at exit */
static void wrs_exit(void)
{
	hist_sfp_nand_exit();
	hist_up_spi_exit();
	hist_up_nand_exit();
}
