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
#define HIST_SHMEM_STRUCTURES
#include <libwr/hist_shmem.h>
#include <libwr/hal_shmem.h>
#include <libwr/util.h>
#include "wrs_hist.h"
#include "hist_crc.h"

struct hist_shmem_data *hist_shmem;
struct wrs_shm_head *hist_shmem_hdr;
static struct wrs_shm_head *hal_shmem_hdr;
static struct hal_temp_sensors *temp_sensors;
static struct hal_port_state *hal_ports;
static struct hal_port_state hal_ports_local_copy[HAL_MAX_PORTS];

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

static void hal_shm_init(void)
{
	
	int ret;
	int n_wait = 0;
	struct hal_shmem_header *h;
	
	while ((ret = wrs_shm_get_and_check(wrs_shm_hal, &hal_shmem_hdr)) != 0) {
		n_wait++;
		if (ret == WRS_SHM_OPEN_FAILED) {
			pr_error("Unable to open HAL's shm !\n");
		}
		if (ret == WRS_SHM_WRONG_VERSION) {
			pr_error("Unable to read HAL's version!\n");
		}
		if (ret == WRS_SHM_INCONSISTENT_DATA) {
			pr_error("Unable to read consistent data from HAL's "
				 "shmem!\n");
		}
		if (n_wait > 10) {
			/* timeout! */
			exit(-1);
		}
		sleep(1);
	}

	if (hal_shmem_hdr->version != HAL_SHMEM_VERSION) {
		pr_error("Unknown HAL's shm version %i (known is %i)\n",
			 hal_shmem_hdr->version, HAL_SHMEM_VERSION);
		exit(1);
	}
	h = (void *)hal_shmem_hdr + hal_shmem_hdr->data_off;
	temp_sensors = &(h->temp);
	hal_ports = wrs_shm_follow(hal_shmem_hdr, h->ports);
	if (!hal_ports) {
		pr_error("Unable to follow hal_ports pointer in HAL's "
			 "shmem\n");
		exit(1);
	}
}


int hal_shmem_read_temp(struct hal_temp_sensors * temp){
	unsigned ii;
	unsigned retries = 0;

	/* read data, with the sequential lock to have all data consistent */
	while (1) {
		ii = wrs_shm_seqbegin(hal_shmem_hdr);
		memcpy(temp, temp_sensors,
		       sizeof(*temp_sensors));
		retries++;
		if (retries > 100)
			return -1;
		if (!wrs_shm_seqretry(hal_shmem_hdr, ii))
			break; /* consistent read */
		usleep(1000);
	}

	return 0;
}

struct hal_port_state * hal_shmem_read_ports(void){
	unsigned ii;
	unsigned retries = 0;

	/* read data, with the sequential lock to have all data consistent */
	while (1) {
		ii = wrs_shm_seqbegin(hal_shmem_hdr);
		memcpy(hal_ports_local_copy, hal_ports,
		       HAL_MAX_PORTS * sizeof(struct hal_port_state));
		retries++;
		if (retries > 100)
			return NULL;
		if (!wrs_shm_seqretry(hal_shmem_hdr, ii))
			break; /* consistent read */
		usleep(1000);
	}

	return hal_ports_local_copy;
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
	time_t t;
	time_t last_update_nand_s;
	time_t last_update_spi_s;
	time_t last_update_sfp_s;

	wrs_msg_init(argc, argv);

	/* Print HAL's version */
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
	assert_init(hist_uptime_init()); /* move it? */
	/* If HAL was running before add all SFPs to the local database */
	assert_init(hist_sfp_init());

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
	last_update_spi_s = t;
	last_update_sfp_s = t;
	for (;;) {
		t = get_monotonic_sec();

		if (last_update_nand_s + NAND_UPDATE_PERIOD <= t) {
			last_update_nand_s += NAND_UPDATE_PERIOD;
			hist_uptime_nand_save();
		}

		if (last_update_spi_s + SPI_UPDATE_PERIOD <= t) {
			last_update_spi_s += SPI_UPDATE_PERIOD;
			hist_uptime_spi_save();
		}

		if (last_update_sfp_s + SFP_UPDATE_PERIOD <= t) {
			last_update_sfp_s += SFP_UPDATE_PERIOD;
			hist_sfp_nand_save();
		}
	  
		hist_wripc_update(1000 /* max ms delay */);
	}

	return 0;
}
