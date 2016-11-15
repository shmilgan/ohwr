#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libwr/wrs-msg.h>
#include <libwr/shmem.h>
#include "wrs_hist.h"
#include "hist_crc.h"
#include <byteswap.h>

int hist_check_running(void)
{
	return 0;
}

int hist_wripc_init(void)
{
	return 0;
}

int hist_wripc_update(int ms_timeout)
{
	return 0;
}

void hal_shm_init(void)
{
}

struct hal_port_state * hal_shmem_read_ports(void)
{
	return NULL;
}

int hal_shmem_read_temp(struct hal_temp_sensors * temp)
{
	return 0;
}

void hist_up_spi_exit(void)
{
}

int hist_up_spi_init(void)
{
	return 0;
}

void hist_up_spi_save(void)
{
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

static void parse_cmdline(int argc, char *argv[])
{
	int opt;

	hist_sfp_nand_filename = NULL;
	hist_sfp_nand_filename_backup = NULL;
	wrs_shm_set_path(".");
	while ((opt = getopt(argc, argv, "hqvp:b:H:")) != -1) {
		switch (opt) {
		case 'h':
			show_help();
			exit(0);
			break;
		case 'q': break; /* done in wrs_msg_init() */
		case 'v': break; /* done in wrs_msg_init() */
		case 'p':
			hist_sfp_nand_filename = optarg;
			break;
		case 'b':
			hist_sfp_nand_filename_backup = optarg;
			break;
		case 'H':
			pr_info("using %s as a path for shmem\n", optarg);
			wrs_shm_set_path(optarg);
			break;
		default:
			pr_error("Unrecognized option. Call %s -h for help.\n",
				 argv[0]);
			break;
		}
	}
	if (hist_sfp_nand_filename == NULL) {
		pr_error("Please specify primary file by \"-p\"\n");
		exit(1);
	}
	if (hist_sfp_nand_filename_backup == NULL) {
		pr_error("Please specify backup file by \"-b\"\n");
		exit(1);
	}
}

int main(int argc, char **argv)
{
	/* Print wrs_hist's version */
	wrs_msg_init(argc, argv);
	pr_info("Commit %s, built on " __DATE__ "\n", __GIT_VER__);

	parse_cmdline(argc, argv);
	crc_init();
	assert_init(hist_shmem_init());
	printf("hist_shmem_hdr->pidsequence %d\n", hist_shmem_hdr->pidsequence);
	printf("hist_shmem_hdr->sequence %d\n", hist_shmem_hdr->sequence);
	hist_sfp_init();
	return 0;
}
