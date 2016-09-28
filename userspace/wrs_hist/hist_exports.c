/* HAL public API available via WR-IPC */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

#include <minipc.h>
#include <libwr/wrs-msg.h>
#include <libwr/shmem.h>

#define HIST_EXPORT_STRUCTURES
#include <hist_exports.h> /* for exported structs/function protos */

#include "wrs_hist.h"

static struct minipc_ch *hist_ch;

/* Define shortcut for exporting */
#define MINIPC_EXP_FUNC(stru,func) stru.f= func; \
		if (minipc_export(hist_ch, &stru) < 0) { \
			pr_error("Could not export %s (rtu_ch=%p)\n",\
				 stru.name,hist_ch); }

static void hist_cleanup_wripc(void)
{
	minipc_close(hist_ch);
}

static int histexp_sfp_action(const struct minipc_pd *pd, uint32_t * args,
			      void *ret)
{
	int rval;
	int *sfp_action = (void *)args;
	char *sfp_vn;
	char *sfp_pn;
	char *sfp_sn;

	/* jump over the string */
	args = minipc_get_next_arg(args, pd->args[1]);
	sfp_vn = (char *)args;
	args = minipc_get_next_arg(args, pd->args[2]);
	sfp_pn = (char *)args;
	args = minipc_get_next_arg(args, pd->args[3]);
	sfp_sn = (char *)args;

	pr_error("action %d, vn %s, pn %s, sn %s\n", *sfp_action, sfp_vn,
		 sfp_pn, sfp_sn);

	switch (*sfp_action) {
	case HIST_SFP_INSERT:
		hist_sfp_insert(sfp_vn, sfp_pn, sfp_sn);
		break;
	case HIST_SFP_REMOVE:
		hist_sfp_remove(sfp_vn, sfp_pn, sfp_sn);
		break;
	default:
		pr_error("Unrecognized action %d\n", *sfp_action);
		break;
	}
	
	rval = 0;
	*(int *)ret = rval;
	return 0;
}

/* Creates a wripc server and exports all public API functions */
int hist_wripc_init(void)
{
	hist_ch = minipc_server_create("wrs_hist", 0);

	if (hist_ch < 0) {
		pr_error("Failed to create mini-rpc server '%s'\n",
		      WRS_HIST_SERVER_ADDR);
		return -1;
	}

	MINIPC_EXP_FUNC(hist_export_sfp_action, histexp_sfp_action);

	pr_info("Started mini-rpc server '%s'\n", WRS_HIST_SERVER_ADDR);

	return 0;
}

/* wripc update function, must be called in the main program loop */
int hist_wripc_update(int ms_timeout)
{
	minipc_server_action(hist_ch, ms_timeout);
	return 0;
}

/* Returns 1 if there's already an instance of the HAL running. Used
   to prevent from launching multiple HALs simultaneously. */
int hist_check_running(void)
{
	struct wrs_shm_head *hist_head;
	hist_head = wrs_shm_get(wrs_shm_hist, "", WRS_SHM_READ);
	if (!hist_head) {
		pr_error("Unable to open shm for wrs_hist! Unable to check if"
			 " there is another wrs_hist instance running. Error:"
			 " %s\n",
			 strerror(errno));
		exit(1);
	}

	/* check if pid is 0 (shm not filled) or process with provided
	 * pid does not exist (probably crashed) */
	if ((hist_head->pid == 0) || (kill(hist_head->pid, 0) != 0))
		return 0;

	wrs_shm_put(hist_head);
	return 1;
}
