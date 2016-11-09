#include <stdlib.h>
#include <stdint.h>

#include <minipc.h>

#include <libwr/wrs-msg.h>
#include <libwr/sfp_lib.h>

#define HIST_EXPORT_STRUCTURES
#include <hist_exports.h> /* for exported structs/function protos */


static struct minipc_ch *hist_ch;

static int try_connect_minipc(void)
{
	if (hist_ch) {
		/* close minipc, if connected before */
		minipc_close(hist_ch);
	}
	hist_ch = minipc_client_create("wrs_hist", MINIPC_FLAG_MSG_NOSIGNAL);
	if (!hist_ch) {
		pr_info("Can't establish WRIPC connection to the wrs_hist "
			 "daemon!\n");
		return -1;
	}

	return 0;
}

int hal_hist_init(void)
{
	try_connect_minipc();
	return 0;
}

static int issue_minipc_call(int action, struct shw_sfp_caldata *sfp)
{
	int val;
	return minipc_call(hist_ch, 200, &hist_export_sfp_action, &val, action,
			   sfp->vendor_name, sfp->part_num,
			   sfp->vendor_serial);
}

static void sfp_action(int action, struct shw_sfp_caldata *sfp)
{
	int ret;
	if (!hist_ch) {
		try_connect_minipc();
	}
	if (hist_ch) {
		ret = issue_minipc_call(action, sfp);
		if (ret < 0) {
			if (try_connect_minipc() < 0) {
				pr_error("Failed to reconnect to wrs_hist\n");
				return;
			}
			ret = issue_minipc_call(action, sfp);
			if (ret >= 0)
				pr_info("Successfully reconnected to "
					"wrs_hist\n");
		}
		
	}
}

void hal_hist_sfp_insert(struct shw_sfp_caldata *sfp)
{
	sfp_action(HIST_SFP_INSERT, sfp);
}

void hal_hist_sfp_remove(struct shw_sfp_caldata *sfp)
{
	if (sfp->vendor_name[0] == '\0'
	    && sfp->part_num[0] == '\0'
	    && sfp->vendor_serial[0] == '\0'
	    ) {
		/* Skip notification of SFPs that were not added to the HAL
		 * correctly. For example their eeprom was corrupted. */
		return;
	}
	sfp_action(HIST_SFP_REMOVE, sfp);
}
