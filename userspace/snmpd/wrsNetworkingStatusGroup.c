#include "wrsSnmp.h"
#include "wrsPortStatusTable.h"
#include "wrsNetworkingStatusGroup.h"

static struct pickinfo wrsNetworkingStatus_pickinfo[] = {
	FIELD(wrsNetworkingStatus_s, ASN_INTEGER, wrsSFPsStatus),
};

struct wrsNetworkingStatus_s wrsNetworkingStatus_s;

time_t wrsNetworkingStatus_data_fill(void)
{
	static time_t time_update; /* time of last update */
	time_t time_port_status; /* time when port status table was updated */
	unsigned int port_status_nrows; /* number of rows in PortStatusTable */
	unsigned int port_status_n_ok; /* number of ok ports */
	unsigned int port_status_n_error; /* number of error ports */
	unsigned int port_status_n_down; /* number of down ports */
	unsigned int port_status_n_na; /* number of N/A ports */
	int i;
	struct wrsPortStatusTable_s *p_a;

	time_port_status = wrsPortStatusTable_data_fill(&port_status_nrows);

	if (time_port_status <= time_update) {
		/* cache not updated, return last update time */
		return time_update;
	}
	time_update = time(NULL);

	memset(&wrsNetworkingStatus_s, 0, sizeof(wrsNetworkingStatus_s));

	/*********************************************************************\
	|*************************** wrsSFPsStatus ***************************|
	\*********************************************************************/

	p_a = wrsPortStatusTable_array;
	port_status_n_ok = 0;
	port_status_n_error = 0;
	port_status_n_down = 0;
	port_status_n_na = 0;
	/* count number of ports of each status */
	for (i = 0; i < port_status_nrows; i++) {
		if (p_a[i].sfp_error == WRS_PORT_STATUS_SFP_ERROR_SFP_OK) {
			port_status_n_ok++;
		}
		if (p_a[i].sfp_error == WRS_PORT_STATUS_SFP_ERROR_SFP_ERROR) {
			port_status_n_error++;
		}
		if (p_a[i].sfp_error == WRS_PORT_STATUS_SFP_ERROR_PORT_DOWN) {
			port_status_n_down++;
		}
		if (p_a[i].sfp_error == 0) {
			port_status_n_na++;
		}
	}

	if ((port_status_n_error > 0)
	    || ((port_status_n_ok + port_status_n_down + port_status_n_na)
			!= port_status_nrows)
	) {
		/* error */
		wrsNetworkingStatus_s.wrsSFPsStatus = WRS_SFPS_STATUS_ERROR;

	} else if (port_status_n_na > 0) { /* warning NA */
		wrsNetworkingStatus_s.wrsSFPsStatus =
						WRS_SFPS_STATUS_WARNING_NA;

	} else if ((port_status_n_ok + port_status_n_down) ==
			port_status_nrows) {
		/* OK is when port is ok or down */
		wrsNetworkingStatus_s.wrsSFPsStatus = WRS_SFPS_STATUS_OK;

	} else { /* probably bug in previous conditions,
		  * this should never happen */
		wrsNetworkingStatus_s.wrsSFPsStatus = WRS_SFPS_STATUS_BUG;
	}

	/* there was an update, return current time */
	return time_update;
}

#define GT_OID WRSNETWORKINGSTATUS_OID
#define GT_PICKINFO wrsNetworkingStatus_pickinfo
#define GT_DATA_FILL_FUNC wrsNetworkingStatus_data_fill
#define GT_DATA_STRUCT wrsNetworkingStatus_s
#define GT_GROUP_NAME "wrsNetworkingStatusGroup"
#define GT_INIT_FUNC init_wrsNetworkingStatusGroup

#include "wrsGroupTemplate.h"
