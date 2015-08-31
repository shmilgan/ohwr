#include "wrsSnmp.h"
#include <snmp_shmem.h>
#include "wrsPtpDataTable.h"
#include "wrsSpllStatusGroup.h"
#include "wrsPortStatusTable.h"
#include "wrsTimingStatusGroup.h"

static struct pickinfo wrsTimingStatus_pickinfo[] = {
	FIELD(wrsTimingStatus_s, ASN_INTEGER, wrsPTPStatus),
	FIELD(wrsTimingStatus_s, ASN_INTEGER, wrsSoftPLLStatus),
	FIELD(wrsTimingStatus_s, ASN_INTEGER, wrsSlaveLinksStatus),
	FIELD(wrsTimingStatus_s, ASN_INTEGER, wrsPTPFramesFlowing),
};

struct wrsTimingStatus_s wrsTimingStatus_s;

static void get_wrsPTPStatus(unsigned int ptp_data_nrows);
static void get_wrsSoftPLLStatus();
static void get_wrsSlaveLinksStatus(unsigned int port_status_nrows);
static void get_wrsPTPFramesFlowing(unsigned int port_status_nrows);

time_t wrsTimingStatus_data_fill(void)
{
	static time_t time_update; /* time of last update */
	time_t time_ptp_data; /* time when wrsPtpDataTable was updated */
	time_t time_spll; /* time when softPLL data was updated */
	time_t time_port_status; /* time when port status table was updated */
	unsigned int ptp_data_nrows; /* number of rows in wrsPtpDataTable */
	unsigned int port_status_nrows; /* number of rows in PortStatusTable */

	time_ptp_data = wrsPtpDataTable_data_fill(&ptp_data_nrows);
	time_spll = wrsSpllStatus_data_fill();
	time_port_status = wrsPortStatusTable_data_fill(&port_status_nrows);

	if (ptp_data_nrows > WRS_MAX_N_SERVO_INSTANCES) {
		snmp_log(LOG_ERR, "SNMP: wrsTimingStatusGroup too many PTP "
				  "instances(%d), only %d supported!\n",
			 WRS_MAX_N_SERVO_INSTANCES, ptp_data_nrows);
		ptp_data_nrows = WRS_MAX_N_SERVO_INSTANCES;
	}

	if (port_status_nrows > WRS_N_PORTS) {
		snmp_log(LOG_ERR, "SNMP: wrsTimingStatusGroup too many ports"
				  "(%d), only %d supported!\n",
			 WRS_N_PORTS, port_status_nrows);
		port_status_nrows = WRS_N_PORTS;
	}

	if (time_ptp_data <= time_update
	    && time_spll <= time_update
	    && time_port_status <= time_update) {
		/* cache not updated, return last update time */
		return time_update;
	}

	/* update when ptp_data and spll status were updated
	 * otherwise there may be comparison between the same data */
	if (time_ptp_data > time_update
	    && time_spll > time_update) {
		get_wrsPTPStatus(ptp_data_nrows);
	}

	/* update when the spll was updated
	 * otherwise there may be comparison between the same data */
	if (time_spll > time_update) {
		get_wrsSoftPLLStatus();
	}

	/* update only when the port_status was updated
	 * otherwise there may be comparison between the same data */
	if (time_port_status > time_update) {
		get_wrsSlaveLinksStatus(port_status_nrows);
		get_wrsPTPFramesFlowing(port_status_nrows);
	}

	time_update = time(NULL);
	/* there was an update, return current time */
	return time_update;
}

static void get_wrsPTPStatus(unsigned int ptp_data_nrows)
{
	struct wrsSpllStatus_s *s;
	struct wrsPtpDataTable_s *pd_a;
	int i;
	static int first_run = 1;

	/* store old values of ptp servo error counters and number of updates */
	static uint32_t servo_updates_prev[WRS_MAX_N_SERVO_INSTANCES];
	static uint32_t n_err_state_prev[WRS_MAX_N_SERVO_INSTANCES];
	static uint32_t n_err_offset_prev[WRS_MAX_N_SERVO_INSTANCES];
	static uint32_t n_err_delta_rtt_prev[WRS_MAX_N_SERVO_INSTANCES];

	/*********************************************************************\
	|*************************** wrsPTPStatus  ***************************|
	\*********************************************************************/
	/*
	 * Error when SPLL is in slave mode and at least one error counter in
	 * PTP increased or no PTP servo updates
	 */
	s = &wrsSpllStatus_s;
	pd_a = wrsPtpDataTable_array;

	wrsTimingStatus_s.wrsPTPStatus = WRS_PTP_STATUS_OK;
	/* NOTE: only one PTP instance is used right now. When switchover is
	 * implemented it will change */
	for (i = 0; i < ptp_data_nrows; i++) {
		if (first_run == 1) {
			/* don't report errors during first run */
			wrsTimingStatus_s.wrsPTPStatus = WRS_PTP_STATUS_FR;

		/* check if error */
		} else if ((s->wrsSpllMode == WRS_SPLL_MODE_SLAVE)
		    && ((pd_a[i].servo_updates == servo_updates_prev[i])
			|| (pd_a[i].n_err_state != n_err_state_prev[i])
			|| (pd_a[i].n_err_offset != n_err_offset_prev[i])
			|| (pd_a[i].n_err_delta_rtt != n_err_delta_rtt_prev[i])
			|| (pd_a[i].delta_tx_m == 0)
			|| (pd_a[i].delta_rx_m == 0)
			|| (pd_a[i].delta_tx_s == 0)
			|| (pd_a[i].delta_rx_s == 0))) {
			wrsTimingStatus_s.wrsPTPStatus = WRS_PTP_STATUS_ERROR;
			snmp_log(LOG_ERR, "SNMP: wrsPTPStatus "
					  "failed for instance %d\n", i);

			/* don't break! Check all other PTP instances,
			 * to update all prev values */
		}

		/* update old values */
		servo_updates_prev[i] = pd_a[i].servo_updates;
		n_err_state_prev[i] = pd_a[i].n_err_state;
		n_err_offset_prev[i] = pd_a[i].n_err_offset;
		n_err_delta_rtt_prev[i] = pd_a[i].n_err_delta_rtt;
	}

	first_run = 0;
}

static void get_wrsSoftPLLStatus(void)
{
	struct wrsSpllStatus_s *s;

	/* store old values of SPLL status */
	static int32_t spll_DelCnt_prev;
	/*********************************************************************\
	|************************* wrsSoftPLLStatus  *************************|
	\*********************************************************************/
	/*
	 * DelCnt - warning if > 0 or no change in DelCnt
	 * seqstate has to be 8 (ready)
	 * mode = 1 (grand master) aligin state must be 6 (LOCKED)
	 * mode = 2 (free running master)
	 * mode = 3 (slave) and wrsSpllHlock != 0 and wrsSpllMlock != 0
	*/
	s = &wrsSpllStatus_s;
	if ( /* check if error */
		s->wrsSpllSeqState != WRS_SPLL_SEQ_STATE_READY
		|| (s->wrsSpllMode == WRS_SPLL_MODE_GRAND_MASTER && s->wrsSpllAlignState != WRS_SPLL_ALIGN_STATE_LOCKED)
		|| ((s->wrsSpllMode != WRS_SPLL_MODE_GRAND_MASTER)
		    && (s->wrsSpllMode != WRS_SPLL_MODE_MASTER)
		    && (s->wrsSpllMode != WRS_SPLL_MODE_SLAVE))
		|| ((s->wrsSpllMode == WRS_SPLL_MODE_SLAVE) && ((s->wrsSpllHlock == 0) || (s->wrsSpllMlock == 0)))
	) {
		wrsTimingStatus_s.wrsSoftPLLStatus =
						WRS_SOFTPLL_STATUS_ERROR;
		/*snmp_log(LOG_ERR, "SNMP: wrsSoftPLLStatus"
				"%d %d %d %d\n",
				s->wrsSpllSeqState != WRS_SPLL_SEQ_STATE_READY,
				s->wrsSpllMode == WRS_SPLL_MODE_GRAND_MASTER && s->wrsSpllAlignState != WRS_SPLL_ALIGN_STATE_LOCKED,
				((s->wrsSpllMode != WRS_SPLL_MODE_GRAND_MASTER)
					&& (s->wrsSpllMode != WRS_SPLL_MODE_MASTER)
					&& (s->wrsSpllMode != WRS_SPLL_MODE_SLAVE)),
				((s->wrsSpllMode == WRS_SPLL_MODE_SLAVE) && ((s->wrsSpllHlock == 0) || (s->wrsSpllMlock == 0)))
			);*/
	} else if ( /* check if warning */
		(s->wrsSpllMode == WRS_SPLL_MODE_GRAND_MASTER && s->wrsSpllDelCnt > 0)
		|| (s->wrsSpllDelCnt != spll_DelCnt_prev)
	) { /* warning */
		wrsTimingStatus_s.wrsSoftPLLStatus =
						WRS_SOFTPLL_STATUS_WARNING;

	} else if ( /* check if any of fields equal to 0 or WARNING_NA */
		s->wrsSpllMode == 0
	) { /* warning NA */
		wrsTimingStatus_s.wrsSoftPLLStatus =
					      WRS_SOFTPLL_STATUS_WARNING_NA;

	} else if ( /* check if OK */
		((s->wrsSpllMode == WRS_SPLL_MODE_GRAND_MASTER && s->wrsSpllDelCnt == 0)
		    || (s->wrsSpllDelCnt == spll_DelCnt_prev))
		&& s->wrsSpllSeqState == WRS_SPLL_SEQ_STATE_READY
		&& ((s->wrsSpllMode == WRS_SPLL_MODE_GRAND_MASTER && s->wrsSpllAlignState == WRS_SPLL_ALIGN_STATE_LOCKED)
		    || (s->wrsSpllMode == WRS_SPLL_MODE_MASTER)
		    || (s->wrsSpllMode == WRS_SPLL_MODE_SLAVE))
	) { /* OK */
		wrsTimingStatus_s.wrsSoftPLLStatus =
						WRS_SOFTPLL_STATUS_OK;

	} else { /* probably bug in previous conditions,
		  * this should never happen */
		wrsTimingStatus_s.wrsSoftPLLStatus =
						WRS_SOFTPLL_STATUS_BUG;
	}

	spll_DelCnt_prev = s->wrsSpllDelCnt;
}

static void get_wrsSlaveLinksStatus(unsigned int port_status_nrows)
{
	struct wrsPortStatusTable_s *p_a;
	int i;

	/*********************************************************************\
	|************************ wrsSlaveLinksStatus ************************|
	\*********************************************************************/
	/*
	 * ok when every slave port is up when switch is in slave mode
	 * and when every slave port is down when switch in master/grandmaster
	 * mode. Don't care about non-wr and auto ports.
	*/
	p_a = wrsPortStatusTable_array;
	/* check whether hal_shmem is available */
	if (shmem_ready_hald()) {
		wrsTimingStatus_s.wrsSlaveLinksStatus = WRS_SLAVE_LINK_STATUS_OK;
		for (i = 0; i < port_status_nrows; i++) {
			/* warning N/A */
			if (/*hal_shmem->s->wrsSpllMode == 0
			    || */p_a[i].port_mode == 0
			    || p_a[i].link_up == 0){
				wrsTimingStatus_s.wrsSlaveLinksStatus =
						WRS_SLAVE_LINK_STATUS_WARNING_NA;
			}
			/* error when slave port is down when switch is in slave mode
			  */
			if (hal_shmem->hal_mode == HAL_TIMING_MODE_BC
			    && (p_a[i].port_mode == WRS_PORT_STATUS_CONFIGURED_MODE_SLAVE)
			    && (p_a[i].link_up == WRS_PORT_STATUS_LINK_DOWN)) {
				wrsTimingStatus_s.wrsSlaveLinksStatus =
							WRS_SLAVE_LINK_STATUS_ERROR;
				snmp_log(LOG_ERR, "SNMP: wrsSlaveLinksStatus (slave) "
						  "failed for port %d\n", i);
			}
			/* error when slave port is up when switch is in master or
			* grandmaster mode */
			if (((hal_shmem->hal_mode == HAL_TIMING_MODE_GRAND_MASTER) || (hal_shmem->hal_mode == HAL_TIMING_MODE_FREE_MASTER))
			    && (p_a[i].port_mode == WRS_PORT_STATUS_CONFIGURED_MODE_SLAVE)
			    && (p_a[i].link_up == WRS_PORT_STATUS_LINK_UP)) {
				wrsTimingStatus_s.wrsSlaveLinksStatus =
							WRS_SLAVE_LINK_STATUS_ERROR;
				snmp_log(LOG_ERR, "SNMP: wrsSlaveLinksStatus (master) "
						  "failed for port %d\n", i);
			}
		}
	}
}

static void get_wrsPTPFramesFlowing(unsigned int port_status_nrows)
{
	struct wrsPortStatusTable_s *p_a;
	int i;
	static int first_run = 1;

	/* store old values of TX and RX PTP counters to calculate delta */
	static unsigned long ptp_tx_count_prev[WRS_N_PORTS];
	static unsigned long ptp_rx_count_prev[WRS_N_PORTS];

	/*********************************************************************\
	|************************ wrsPTPFramesFlowing ************************|
	\*********************************************************************/
	/*
	 * Check if PTP frames are flowing. Check only on ports that are
	 * non-wr and up.
	 */
	p_a = wrsPortStatusTable_array;
	wrsTimingStatus_s.wrsPTPFramesFlowing = WRS_PTP_FRAMES_FLOWING_OK;
	for (i = 0; i < port_status_nrows; i++) {
		if (first_run == 1) {
			/* don't report errors during first run */
			wrsTimingStatus_s.wrsPTPFramesFlowing =
						WRS_PTP_FRAMES_FLOWING_FR;

		/* Error when there is no increase in TX/RX PTP counters.
		   Check only when port is non-wr and port is down */
		} else if ((p_a[i].port_mode != WRS_PORT_STATUS_CONFIGURED_MODE_NON_WR)
		    && (p_a[i].link_up == WRS_PORT_STATUS_LINK_UP)
		    && ((ptp_tx_count_prev[i] == p_a[i].ptp_tx_count)
			|| (ptp_rx_count_prev[i] == p_a[i].ptp_rx_count))) {
			wrsTimingStatus_s.wrsPTPFramesFlowing =
						WRS_PTP_FRAMES_FLOWING_ERROR;
			snmp_log(LOG_ERR, "SNMP: wrsPTPFramesFlowing "
					  "failed for port %d\n", i);

		/* warning N/A */
		} else if (p_a[i].port_mode == 0
		    || p_a[i].link_up == 0){
			wrsTimingStatus_s.wrsPTPFramesFlowing =
					WRS_PTP_FRAMES_FLOWING_WARNING_NA;
		}
		/* save current values */
		ptp_tx_count_prev[i] = p_a[i].ptp_tx_count;
		ptp_rx_count_prev[i] = p_a[i].ptp_rx_count;
	}

	first_run = 0;
}

#define GT_OID WRSTIMINGSTATUS_OID
#define GT_PICKINFO wrsTimingStatus_pickinfo
#define GT_DATA_FILL_FUNC wrsTimingStatus_data_fill
#define GT_DATA_STRUCT wrsTimingStatus_s
#define GT_GROUP_NAME "wrsTimingStatusGroup"
#define GT_INIT_FUNC init_wrsTimingStatusGroup

#include "wrsGroupTemplate.h"
