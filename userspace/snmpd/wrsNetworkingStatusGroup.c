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
static char *slog_obj_name;
static char *wrsSFPsStatus_str = "wrsSFPsStatus";
static char *wrsEndpointStatus_str = "wrsEndpointStatus";
static char *wrsSwcoreStatus_str = "wrsSwcoreStatus";
static char *wrsRTUStatus_str = "wrsRTUStatus";


static void copy_pstats(struct ns_pstats *copy, struct wrsPstatsHCTable_s *org,
			unsigned int rows)
{
	int i;
	for (i = 0; i < rows; i++) {
		/* wrsEndpointStatus */
		copy->wrsPstatsHCTXUnderrun = org->wrsPstatsHCTXUnderrun;
		copy->wrsPstatsHCRXOverrun = org->wrsPstatsHCRXOverrun;
		copy->wrsPstatsHCRXInvalidCode = org->wrsPstatsHCRXInvalidCode;
		copy->wrsPstatsHCRXSyncLost = org->wrsPstatsHCRXSyncLost;
		copy->wrsPstatsHCRXPfilterDropped = org->wrsPstatsHCRXPfilterDropped;
		copy->wrsPstatsHCRXPCSErrors = org->wrsPstatsHCRXPCSErrors;
		copy->wrsPstatsHCRXCRCErrors = org->wrsPstatsHCRXCRCErrors;
		/* wrsSwcoreStatus */
		copy->wrsPstatsHCRXFrames = org->wrsPstatsHCRXFrames;
		copy->wrsPstatsHCRXPrio0 = org->wrsPstatsHCRXPrio0;
		copy->wrsPstatsHCRXPrio1 = org->wrsPstatsHCRXPrio1;
		copy->wrsPstatsHCRXPrio2 = org->wrsPstatsHCRXPrio2;
		copy->wrsPstatsHCRXPrio3 = org->wrsPstatsHCRXPrio3;
		copy->wrsPstatsHCRXPrio4 = org->wrsPstatsHCRXPrio4;
		copy->wrsPstatsHCRXPrio5 = org->wrsPstatsHCRXPrio5;
		copy->wrsPstatsHCRXPrio6 = org->wrsPstatsHCRXPrio6;
		copy->wrsPstatsHCRXPrio7 = org->wrsPstatsHCRXPrio7;
		copy->wrsPstatsHCFastMatchPriority = org->wrsPstatsHCFastMatchPriority;
		/* wrsRTUStatus */
		copy->wrsPstatsHCRXDropRTUFull = org->wrsPstatsHCRXDropRTUFull;

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

	slog_obj_name = wrsEndpointStatus_str;

	/* values from 2.2.2 "Fault in the Endpoint’s transmission/reception
	 * path" in wrs_failures document shouldn't change faster than 1
	 * per second */
	for (i = 0; i < rows; i++) {
		SLOG_IF_COMP_WNSG(SL_ER, wrsPstatsHCTXUnderrun,       new, old, i, t_delta, 1.0, ret = 1);
		SLOG_IF_COMP_WNSG(SL_ER, wrsPstatsHCRXOverrun,        new, old, i, t_delta, 1.0, ret = 1);
		SLOG_IF_COMP_WNSG(SL_ER, wrsPstatsHCRXInvalidCode,    new, old, i, t_delta, 1.0, ret = 1);
		SLOG_IF_COMP_WNSG(SL_ER, wrsPstatsHCRXSyncLost,       new, old, i, t_delta, 1.0, ret = 1);
		SLOG_IF_COMP_WNSG(SL_ER, wrsPstatsHCRXPfilterDropped, new, old, i, t_delta, 1.0, ret = 1);
		SLOG_IF_COMP_WNSG(SL_ER, wrsPstatsHCRXPCSErrors,      new, old, i, t_delta, 1.0, ret = 1);
		SLOG_IF_COMP_WNSG(SL_ER, wrsPstatsHCRXCRCErrors,      new, old, i, t_delta, 1.0, ret = 1);
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

	slog_obj_name = wrsSwcoreStatus_str;

	for (i = 0; i < rows; i++) {
		/* TXFrames and Forwarded described in 2.2.3 "Problem with the
		 * SwCore or Endpoint HDL module" in wrs_failures document
		 * shouldn't differ more than FORWARD_DELTA in total */
/* counter Forwarded (38) is implemented in HDL, but does not count PTP
 * traffic!!! */
#if 0
		if ( /* shouldn't differ more than FORWARD_DELTA */
		     ((new[i].wrsPstatsHCTXFrames - new[i].wrsPstatsHCForwarded) > FORWARD_DELTA)
		     || ((new[i].wrsPstatsHCForwarded - new[i].wrsPstatsHCTXFrames) > FORWARD_DELTA)
		) {
			/* if error, no need to check more, but do it just for
			 * logs */
			ret = 1;
			snmp_log(LOG_ERR, "SNMP: wrsSwcoreStatus failed for "
					  "port %d (wri %d)\n", i + 1, i + 1);
		}
#endif
		/* values from 2.2.5 "Too much HP traffic / Per-priority queue
		 * full" in wrs_failures document shouldn't change faster
		 * than parameters defined in dotconfig per second */
		SLOG_IF_COMP_WNSG(SL_ER, wrsPstatsHCRXFrames,          new, old, i, t_delta, ns_dotconfig.rx_frame_rate,      ret = 1);
		SLOG_IF_COMP_WNSG(SL_ER, wrsPstatsHCRXPrio0,           new, old, i, t_delta, ns_dotconfig.rx_prio_frame_rate, ret = 1);
		SLOG_IF_COMP_WNSG(SL_ER, wrsPstatsHCRXPrio1,           new, old, i, t_delta, ns_dotconfig.rx_prio_frame_rate, ret = 1);
		SLOG_IF_COMP_WNSG(SL_ER, wrsPstatsHCRXPrio2,           new, old, i, t_delta, ns_dotconfig.rx_prio_frame_rate, ret = 1);
		SLOG_IF_COMP_WNSG(SL_ER, wrsPstatsHCRXPrio3,           new, old, i, t_delta, ns_dotconfig.rx_prio_frame_rate, ret = 1);
		SLOG_IF_COMP_WNSG(SL_ER, wrsPstatsHCRXPrio4,           new, old, i, t_delta, ns_dotconfig.rx_prio_frame_rate, ret = 1);
		SLOG_IF_COMP_WNSG(SL_ER, wrsPstatsHCRXPrio5,           new, old, i, t_delta, ns_dotconfig.rx_prio_frame_rate, ret = 1);
		SLOG_IF_COMP_WNSG(SL_ER, wrsPstatsHCRXPrio6,           new, old, i, t_delta, ns_dotconfig.rx_prio_frame_rate, ret = 1);
		SLOG_IF_COMP_WNSG(SL_ER, wrsPstatsHCRXPrio7,           new, old, i, t_delta, ns_dotconfig.rx_prio_frame_rate, ret = 1);
		SLOG_IF_COMP_WNSG(SL_ER, wrsPstatsHCFastMatchPriority, new, old, i, t_delta, ns_dotconfig.hp_frame_rate,      ret = 1);
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

	slog_obj_name = wrsRTUStatus_str;

	/* values from 2.2.4 "RTU is full and cannot accept more requests" in
	 * wrs_failures document shouldn't increase */
	for (i = 0; i < rows; i++) {
		if ((new[i].wrsPstatsHCRXDropRTUFull - old[i].wrsPstatsHCRXDropRTUFull) > 0) {
			/* if error, no need to check more, but do it just for
			 * logs */
			ret = 1;
			snmp_log(LOG_ERR, "SNMP: " SL_ER " %s: "
				 "wrsPstatsHCRXDropRTUFull counter for port %i (wri %i) increased by %lld, allowed %d\n",
				 slog_obj_name, i + 1, i + 1,
				 new[i].wrsPstatsHCRXDropRTUFull - old[i].wrsPstatsHCRXDropRTUFull, 0);
			snmp_log(LOG_ERR, "SNMP: " SL_ER " %s: "
				 "some frames were lost because RTU could not accept new requests\n",
				 slog_obj_name);
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
	time_t time_pstats_delta; /* seconds since last update */
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

	slog_obj_name = wrsSFPsStatus_str;
	p_a = wrsPortStatusTable_array;
	port_status_n_ok = 0;
	port_status_n_error = 0;
	port_status_n_down = 0;
	port_status_n_na = 0;
	/* count number of ports of each status */
	for (i = 0; i < port_status_nrows; i++) {
		if (p_a[i].wrsPortStatusSfpError == WRS_PORT_STATUS_SFP_ERROR_SFP_OK) {
			port_status_n_ok++;
		}
		if (p_a[i].wrsPortStatusSfpError == WRS_PORT_STATUS_SFP_ERROR_PORT_DOWN) {
			port_status_n_down++;
		}
		if (p_a[i].wrsPortStatusSfpError == WRS_PORT_STATUS_SFP_ERROR_SFP_ERROR) {
			port_status_n_error++;
		}
		if (p_a[i].wrsPortStatusSfpError == 0) {
			snmp_log(LOG_ERR, "SNMP: " SL_NA " %s: Unable to read wrsSFPsStatus "
				 "for port %i (wri%i)\n",
				 slog_obj_name, i + 1, i + 1);
			port_status_n_na++;
		}
	}

	if (port_status_n_error > 0) {
		/* error */
		wrsNetworkingStatus_s.wrsSFPsStatus = WRS_SFPS_STATUS_ERROR;
	} else if ((port_status_n_ok + port_status_n_down + port_status_n_na)
			!= port_status_nrows) {
		snmp_log(LOG_ERR, "SNMP: " SL_ER " %s: Error reading statuses of SFPs\n",
			 slog_obj_name);
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
