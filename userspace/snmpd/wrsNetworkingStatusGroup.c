#include "wrsSnmp.h"
#include "wrsPortStatusTable.h"
#include "wrsPstatsHCTable.h"
#include "wrsNetworkingStatusGroup.h"
#include <libwr/config.h>

static struct pickinfo wrsNetworkingStatus_pickinfo[] = {
	FIELD(wrsNetworkingStatus_s, ASN_INTEGER, wrsSFPsStatus),
	FIELD(wrsNetworkingStatus_s, ASN_INTEGER, wrsEndpointStatus),
	FIELD(wrsNetworkingStatus_s, ASN_INTEGER, wrsSwcoreStatus),
	FIELD(wrsNetworkingStatus_s, ASN_INTEGER, wrsRTUStatus),
};

struct wrsNetworkingStatus_s wrsNetworkingStatus_s;
static struct wrsNetworkingStatus_config ns_dotconfig;
static struct ns_pstats ns_pstats_copy[WRS_N_PORTS];

static void copy_pstats(struct ns_pstats *copy, struct wrsPstatsHCTable_s *org,
			unsigned int rows)
{
	int i;
	for (i = 0; i < rows; i++) {
		/* wrsEndpointStatus */
		copy->TXUnderrun = org->TXUnderrun;
		copy->RXOverrun = org->RXOverrun;
		copy->RXInvalidCode = org->RXInvalidCode;
		copy->RXSyncLost = org->RXSyncLost;
		copy->RXPfilterDropped = org->RXPfilterDropped;
		copy->RXPCSErrors = org->RXPCSErrors;
		copy->RXCRCErrors = org->RXCRCErrors;
		/* wrsSwcoreStatus */
		copy->RXFrames = org->RXFrames;
		copy->RXPrio0 = org->RXPrio0;
		copy->RXPrio1 = org->RXPrio1;
		copy->RXPrio2 = org->RXPrio2;
		copy->RXPrio3 = org->RXPrio3;
		copy->RXPrio4 = org->RXPrio4;
		copy->RXPrio5 = org->RXPrio5;
		copy->RXPrio6 = org->RXPrio6;
		copy->RXPrio7 = org->RXPrio7;
		copy->FastMatchPriority = org->FastMatchPriority;
		/* wrsRTUStatus */
		copy->RXDropRTUFull = org->RXDropRTUFull;

		copy++;
		org++;
	}
}

static int get_endpoint_status(struct ns_pstats *old,
			       struct wrsPstatsHCTable_s *new,
			       unsigned int rows,
			       float t_delta)
{
	int i;
	int ret;
	ret = 0;

	/* values from 2.2.2 "Fault in the Endpoint’s transmission/reception
	 * path" in wrs_failures document shouldn't change faster than 1
	 * per second */
	for (i = 0; i < rows; i++) {
		if (
		     ((new[i].TXUnderrun - old[i].TXUnderrun)/t_delta > 1.0)
		     || ((new[i].RXOverrun - old[i].RXOverrun)/t_delta > 1.0)
		     || ((new[i].RXInvalidCode - old[i].RXInvalidCode)/t_delta > 1.0)
		     || ((new[i].RXSyncLost - old[i].RXSyncLost)/t_delta > 1.0)
		     || ((new[i].RXPfilterDropped - old[i].RXPfilterDropped)/t_delta > 1.0)
		     || ((new[i].RXPCSErrors - old[i].RXPCSErrors)/t_delta > 1.0)
		     || ((new[i].RXCRCErrors - old[i].RXCRCErrors)/t_delta > 1.0)
		) {
			/* if error, no need to check more, but do it just for
			 * logs */
			ret = 1;
			snmp_log(LOG_ERR, "SNMP: wrsEndpointStatus failed for "
					  "port %d\n", i);
		}
	}
	return ret;
}

/* don't use this function for now, return OK */
static int get_swcore_status(struct ns_pstats *old,
			     struct wrsPstatsHCTable_s *new,
			     unsigned int rows,
			     float t_delta)
{
	int i;
	int ret;
	ret = 0;

	/* don't use this function for now, return OK */
	return ret;

	for (i = 0; i < rows; i++) {
		/* TXFrames and Forwarded described in 2.2.3 "Problem with the
		 * SwCore or Endpoint HDL module" in wrs_failures document
		 * shouldn't differ more than FORWARD_DELTA in total */
/* counter Forwarded (38) is implemented in HDL, but does not count PTP
 * traffic!!! */
#if 0
		if ( /* shouldn't differ more than FORWARD_DELTA */
		     ((new[i].TXFrames - new[i].Forwarded) > FORWARD_DELTA)
		     || ((new[i].Forwarded - new[i].TXFrames) > FORWARD_DELTA)
		) {
			/* if error, no need to check more, but do it just for
			 * logs */
			ret = 1;
			snmp_log(LOG_ERR, "SNMP: wrsSwcoreStatus failed for "
					  "port %d\n", i);
		}
#endif
		/* values from 2.2.5 "Too much HP traffic / Per-priority queue
		 * full" in wrs_failures document shouldn't change faster
		 * than parameters defined in dotconfig per second */
		if ( /* shouldn't differ more than FORWARD_DELTA */
		     ((new[i].RXFrames - old[i].RXFrames)/t_delta >= ns_dotconfig.rx_frame_rate)
		     || ((new[i].RXPrio0 - old[i].RXPrio0)/t_delta >= ns_dotconfig.rx_prio_frame_rate)
		     || ((new[i].RXPrio1 - old[i].RXPrio1)/t_delta >= ns_dotconfig.rx_prio_frame_rate)
		     || ((new[i].RXPrio2 - old[i].RXPrio2)/t_delta >= ns_dotconfig.rx_prio_frame_rate)
		     || ((new[i].RXPrio3 - old[i].RXPrio3)/t_delta >= ns_dotconfig.rx_prio_frame_rate)
		     || ((new[i].RXPrio4 - old[i].RXPrio4)/t_delta >= ns_dotconfig.rx_prio_frame_rate)
		     || ((new[i].RXPrio5 - old[i].RXPrio5)/t_delta >= ns_dotconfig.rx_prio_frame_rate)
		     || ((new[i].RXPrio6 - old[i].RXPrio6)/t_delta >= ns_dotconfig.rx_prio_frame_rate)
		     || ((new[i].RXPrio7 - old[i].RXPrio7)/t_delta >= ns_dotconfig.rx_prio_frame_rate)
		     || ((new[i].FastMatchPriority - old[i].FastMatchPriority)/t_delta >= ns_dotconfig.hp_frame_rate)
		) {
			/* if error, no need to check more, but do it just for
			 * logs */
			ret = 1;
			snmp_log(LOG_ERR, "SNMP: wrsSwcoreStatus failed for "
					  "port %d\n", i);
		}
	}
	return ret;
}

static int get_rtu_status(struct ns_pstats *old,
			  struct wrsPstatsHCTable_s *new, unsigned int rows,
			  float t_delta)
{
	int i;
	int ret;
	ret = 0;

	/* values from 2.2.4 "RTU is full and cannot accept more requests" in
	 * wrs_failures document shouldn't increase */
	for (i = 0; i < rows; i++) {
		if ((new[i].RXDropRTUFull - old[i].RXDropRTUFull) > 0) {
			/* if error, no need to check more, but do it just for
			 * logs */
			ret = 1;
			snmp_log(LOG_ERR, "SNMP: wrsEndpointStatus failed for "
					  "port %d\n", i);
		}
	}
	/* TODO: add 2.2.7 "Network loop - two or more identical MACs on two or
	 * more ports" when implemented */
	return ret;
}

/* read configuration from dotconfig */
static void load_dot_config(void)
{
	char *tmp;

	tmp = libwr_cfg_get("SNMP_SWCORESTATUS_HP_FRAME_RATE");
	if (tmp)
		ns_dotconfig.hp_frame_rate = atoi(tmp);

	tmp = libwr_cfg_get("SNMP_SWCORESTATUS_RX_FRAME_RATE");
	if (tmp)
		ns_dotconfig.rx_frame_rate = atoi(tmp);

	tmp = libwr_cfg_get("SNMP_SWCORESTATUS_RX_PRIO_FRAME_RATE");
	if (tmp)
		ns_dotconfig.rx_prio_frame_rate = atoi(tmp);
}

time_t wrsNetworkingStatus_data_fill(void)
{
	static time_t time_update; /* time of last update */
	time_t time_port_status; /* time when port status table was updated */
	time_t time_pstats; /* time when pstats table was updated */
	float time_pstats_delta; /* seconds since last update */
	static time_t time_pstats_prev; /* time when previous state of pstats
					 * table was updated */
	unsigned int port_status_nrows; /* number of rows in PortStatusTable */
	unsigned int pstats_nrows; /* number of rows in PstatsTable */
	unsigned int port_status_n_ok; /* number of ok ports */
	unsigned int port_status_n_error; /* number of error ports */
	unsigned int port_status_n_down; /* number of down ports */
	unsigned int port_status_n_na; /* number of N/A ports */
	int i;
	int ret;
	struct wrsPortStatusTable_s *p_a;
	static int run_once = 1;

	time_port_status = wrsPortStatusTable_data_fill(&port_status_nrows);
	time_pstats = wrsPstatsHCTable_data_fill(&pstats_nrows);

	if (time_port_status <= time_update
	    && time_pstats <= time_update) {
		/* cache not updated, return last update time */
		return time_update;
	}
	time_update = get_monotonic_sec();

	if (run_once) {
		run_once = 0;
		/* read configuration from dotconfig */
		load_dot_config();
	}
	time_pstats_delta = time_pstats - time_pstats_prev;
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

	/*********************************************************************\
	|************************* wrsEndpointStatus *************************|
	\*********************************************************************/

	if (time_pstats_prev) { /* never generate error during first check */
		ret = get_endpoint_status(ns_pstats_copy, pstats_array,
					  pstats_nrows, time_pstats_delta);
		if (ret == 0)
			wrsNetworkingStatus_s.wrsEndpointStatus =
						WRS_ENDPOINT_STATUS_OK;
		else
			wrsNetworkingStatus_s.wrsEndpointStatus =
						WRS_ENDPOINT_STATUS_ERROR;
	} else {
		/* first read */
		wrsNetworkingStatus_s.wrsEndpointStatus =
						WRS_ENDPOINT_STATUS_FR;
	}

	/*********************************************************************\
	|************************** wrsSwcoreStatus **************************|
	\*********************************************************************/

	if (time_pstats_prev) { /* never generate error during first check */
		ret = get_swcore_status(ns_pstats_copy, pstats_array,
					  pstats_nrows, time_pstats_delta);
		if (ret == 0)
			wrsNetworkingStatus_s.wrsSwcoreStatus =
						WRS_SWCORE_STATUS_OK;
		else
			wrsNetworkingStatus_s.wrsSwcoreStatus =
						WRS_SWCORE_STATUS_ERROR;
	} else {
		/* first read */
		wrsNetworkingStatus_s.wrsSwcoreStatus = WRS_SWCORE_STATUS_FR;
	}

	/*********************************************************************\
	|*************************** wrsRTUStatus  ***************************|
	\*********************************************************************/

	if (time_pstats_prev) { /* never generate error during first check */
		ret = get_rtu_status(ns_pstats_copy, pstats_array,
					  pstats_nrows, time_pstats_delta);
		if (ret == 0)
			wrsNetworkingStatus_s.wrsRTUStatus =
						WRS_RTU_STATUS_OK;
		else
			wrsNetworkingStatus_s.wrsRTUStatus =
						WRS_RTU_STATUS_ERROR;
	} else {
		/* first read */
		wrsNetworkingStatus_s.wrsRTUStatus = WRS_RTU_STATUS_FR;
	}


	/* save time of pstats copy */
	time_pstats_prev = time_pstats;
	/* copy current set of pstats */
	copy_pstats(ns_pstats_copy, pstats_array, pstats_nrows);
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
