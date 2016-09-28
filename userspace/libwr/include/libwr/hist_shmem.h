#ifndef __LIBWR_HIST_SHMEM_H__
#define __LIBWR_HIST_SHMEM_H__

#include <libwr/hist.h>

/* This is the overall structure stored in shared memory */
#define HIST_SHMEM_VERSION 1 /* Version 1 first version */

struct hist_shmem_data {
	struct wrs_hist_run_nand hist_run_nand;
	struct wrs_hist_run_spi hist_run_spi;
	struct wrs_hist_sfp_nand hist_sfp_nand;
	/* Histogram of SFPs temperatures */
	//struct wrs_hist_sfp_temp hist_sfp_temp[WRS_HIST_MAX_SFPS];
};

#endif /*  __LIBWR_HIST_SHMEM_H__ */
