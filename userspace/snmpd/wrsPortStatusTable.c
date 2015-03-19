#include "wrsSnmp.h"
#include "wrsPortStatusTable.h"

/* Our data: per-port information */
struct wrsPortStatusTable_s wrsPortStatusTable_array[WRS_N_PORTS];

static struct pickinfo wrsPortStatusTable_pickinfo[] = {
	FIELD(wrsPortStatusTable_s, ASN_INTEGER, link_up),
	FIELD(wrsPortStatusTable_s, ASN_INTEGER, port_mode),
	FIELD(wrsPortStatusTable_s, ASN_INTEGER, port_locked),
	FIELD(wrsPortStatusTable_s, ASN_OCTET_STR, peer_id),
	FIELD(wrsPortStatusTable_s, ASN_OCTET_STR, sfp_vn),
	FIELD(wrsPortStatusTable_s, ASN_OCTET_STR, sfp_pn),
	FIELD(wrsPortStatusTable_s, ASN_OCTET_STR, sfp_vs),
	FIELD(wrsPortStatusTable_s, ASN_INTEGER, sfp_in_db),
	FIELD(wrsPortStatusTable_s, ASN_INTEGER, sfp_GbE),
	FIELD(wrsPortStatusTable_s, ASN_INTEGER, sfp_error),
};


time_t wrsPortStatusTable_data_fill(unsigned int *n_rows)
{
	unsigned ii, i;
	unsigned retries = 0;
	static time_t time_update;
	time_t time_cur;

	/* number of rows does not change for wrsPortStatusTable */
	if (n_rows)
		*n_rows = WRS_N_PORTS;

	time_cur = time(NULL);
	if (time_update
	    && time_cur - time_update < WRSPORTSTATUSTABLE_CACHE_TIMEOUT) {
		/* cache not updated, return last update time */
		return time_update;
	}
	time_update = time_cur;

	/* read data, with the sequential lock to have all data consistent */
	struct hal_port_state *port_state;
	memset(wrsPortStatusTable_array, 0, sizeof(wrsPortStatusTable_array));
	while (1) {
		ii = wrs_shm_seqbegin(hal_head);
		for (i = 0; i < hal_nports_local; ++i) {
			/* Assume that number of ports does not change between
			 * reads */
			char if_name[10];

			snprintf(if_name, 10, "wr%d", i);
			port_state = hal_lookup_port(hal_ports,
						    hal_nports_local, if_name);
			/* No need to copy all ports structures, only what
			 * we're interested in.
			 * Keep value 0 for Not available */
			wrsPortStatusTable_array[i].link_up =
					1 + state_up(port_state->state);
			wrsPortStatusTable_array[i].port_mode =
							port_state->mode;
			if (port_state->state == HAL_PORT_STATE_DISABLED) {
				wrsPortStatusTable_array[i].sfp_error =
					  WRS_PORT_STATUS_SFP_ERROR_PORT_DOWN;
				/* if port is disabled don't fill
				 * other fields */
				continue;
			}
			/* Keep value 0 for Not available */
			wrsPortStatusTable_array[i].port_locked =
							1 + port_state->locked;
			/* FIXME: get real peer_id */
			memset(&wrsPortStatusTable_array[i].peer_id, 0xff,
			       sizeof(ClockIdentity));
			wrsPortStatusTable_array[i].sfp_in_db =
			  port_state->calib.sfp.flags & SFP_FLAG_IN_DB ? 2 : 1;
			wrsPortStatusTable_array[i].sfp_GbE =
			  port_state->calib.sfp.flags & SFP_FLAG_1GbE ? 2 : 1;
			strncpy(wrsPortStatusTable_array[i].sfp_vn,
				port_state->calib.sfp.vendor_name,
				sizeof(wrsPortStatusTable_array[i].sfp_vn));
			strncpy(wrsPortStatusTable_array[i].sfp_pn,
				port_state->calib.sfp.part_num,
				sizeof(wrsPortStatusTable_array[i].sfp_pn));
			strncpy(wrsPortStatusTable_array[i].sfp_vs,
				port_state->calib.sfp.vendor_serial,
				sizeof(wrsPortStatusTable_array[i].sfp_vs));
			/* sfp error when SFP is not 1 GbE or
			 * (port is not wr-non mode and sfp not in data base)
			 * Keep value 0 for Not available 
			 * sfp ok is 1 (WRS_PORT_STATUS_SFP_ERROR_SFP_OK)
			 * sfp error is 2 WRS_PORT_STATUS_SFP_ERROR_SFP_ERROR
			 * port down, set above, is 3
			 * (WRS_PORT_STATUS_SFP_ERROR_PORT_DOWN) */
			wrsPortStatusTable_array[i].sfp_error = 1 +
				((wrsPortStatusTable_array[i].sfp_GbE == 1) ||
				((port_state->mode != HEXP_PORT_MODE_NON_WR) &&
				(wrsPortStatusTable_array[i].sfp_in_db == 1)));

			logmsg("reading ports name %s link %d, mode %d, "
				"locked %d\n", port_state->name,
				wrsPortStatusTable_array[i].link_up,
				wrsPortStatusTable_array[i].port_mode,
				wrsPortStatusTable_array[i].port_locked);
		}

		retries++;
		if (retries > 100) {
			snmp_log(LOG_ERR, "%s: too many retries to read HAL\n",
				 __func__);
			retries = 0;
			}
		if (!wrs_shm_seqretry(hal_head, ii))
			break; /* consistent read */
		usleep(1000);
	}

	/* there was an update, return current time */
	return time_cur;
}

#define TT_OID WRS_OID, 6, 4
#define TT_PICKINFO wrsPortStatusTable_pickinfo
#define TT_DATA_FILL_FUNC wrsPortStatusTable_data_fill
#define TT_DATA_ARRAY wrsPortStatusTable_array
#define TT_GROUP_NAME "wrsPortStatusTable"
#define TT_INIT_FUNC init_wrsPortStatusTable


#include "wrsTableTemplate.h"
