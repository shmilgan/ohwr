#include "wrsSnmp.h"
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

time_t wrsTimingStatus_data_fill(void)
{
	static time_t time_update; /* time of last update */
	time_t time_spll; /* time when softPLL data was updated */
	time_t time_port_status; /* time when port status table was updated */
	unsigned int port_status_nrows; /* number of rows in PortStatusTable */
	int i;
	struct wrsSpllStatus_s *s;
	struct wrsPortStatusTable_s *p_a;

	time_spll = wrsSpllStatus_data_fill();
	time_port_status = wrsPortStatusTable_data_fill(&port_status_nrows);

	if (time_spll <= time_update
	    && time_port_status <= time_update) {
		/* cache not updated, return last update time */
		return time_update;
	}
	time_update = time(NULL);

	memset(&wrsTimingStatus_s, 0, sizeof(wrsTimingStatus_s));

	/*********************************************************************\
	|************************* wrsSoftPLLStatus  *************************|
	\*********************************************************************/
	/*
	 * DelCnt - warning if > 0
	 * seqstate has to be 8 (ready)
	 * mode = 1 (grand master) aligin state must be 6 (LOCKED)
	 * mode = 2 (free running master)
	 * mode = 3 (slave)
	*/
	s = &wrsSpllStatus_s;
	if ( /* check if error */
		s->wrsSpllSeqState != WRS_SPLL_SEQ_STATE_READY
		|| (s->wrsSpllMode == WRS_SPLL_MODE_GRAND_MASTER && s->wrsSpllAlignState != WRS_SPLL_ALIGN_STATE_LOCKED)
		|| ((s->wrsSpllMode != WRS_SPLL_MODE_GRAND_MASTER)
		    && (s->wrsSpllMode != WRS_SPLL_MODE_MASTER)
		    && (s->wrsSpllMode != WRS_SPLL_MODE_SLAVE))
	) {
		wrsTimingStatus_s.wrsSoftPLLStatus =
						WRS_SOFTPLL_STATUS_ERROR;

	} else if ( /* check if warning */
		s->wrsSpllDelCnt > 0
	) { /* warning */
		wrsTimingStatus_s.wrsSoftPLLStatus =
						WRS_SOFTPLL_STATUS_WARNING;

	} else if ( /* check if any of fields equal to 0 or WARNING_NA */
		s->wrsSpllMode == 0
	) { /* warning NA */
		wrsTimingStatus_s.wrsSoftPLLStatus =
					      WRS_SOFTPLL_STATUS_WARNING_NA;

	} else if ( /* check if OK */
		s->wrsSpllDelCnt == 0
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

	/*********************************************************************\
	|************************ wrsSlaveLinksStatus ************************|
	\*********************************************************************/
	/*
	 * ok when every slave port is up when switch is in slave mode
	 * and when every slave port is down when switch in master/grandmaster
	 * mode. Don't care about non-wr and auto ports.
	*/
	p_a = wrsPortStatusTable_array;
	wrsTimingStatus_s.wrsSlaveLinksStatus = WRS_SLAVE_LINK_STATUS_OK;
	for (i = 0; i < port_status_nrows; i++) {
		/* warning N/A */
		if (s->wrsSpllMode == 0
		    || p_a[i].port_mode == 0
		    || p_a[i].link_up == 0){
			wrsTimingStatus_s.wrsSlaveLinksStatus =
					WRS_SLAVE_LINK_STATUS_WARNING_NA;
		}
		/* error when slave port is down when switch is in slave mode
		   */
		if (s->wrsSpllMode == WRS_SPLL_MODE_SLAVE
		    && (p_a[i].port_mode == WRS_PORT_STATUS_CONFIGURED_MODE_SLAVE)
		    && (p_a[i].link_up == WRS_PORT_STATUS_LINK_DOWN)) {
			wrsTimingStatus_s.wrsSlaveLinksStatus =
						WRS_SLAVE_LINK_STATUS_ERROR;
			snmp_log(LOG_ERR, "SNMP: wrsSlaveLinksStatus (slave) "
					  "failed for port %d\n", i);
		}
		/* error when slave port is up when switch is in master or
		 * grandmaster mode */
		if (((s->wrsSpllMode == WRS_SPLL_MODE_GRAND_MASTER) || (s->wrsSpllMode == WRS_SPLL_MODE_MASTER))
		    && (p_a[i].port_mode == WRS_PORT_STATUS_CONFIGURED_MODE_SLAVE)
		    && (p_a[i].link_up == WRS_PORT_STATUS_LINK_UP)) {
			wrsTimingStatus_s.wrsSlaveLinksStatus =
						WRS_SLAVE_LINK_STATUS_ERROR;
			snmp_log(LOG_ERR, "SNMP: wrsSlaveLinksStatus (master) "
					  "failed for port %d\n", i);
		}
	}

	/* there was an update, return current time */
	return time_update;
}

#define GT_OID WRSTIMINGSTATUS_OID
#define GT_PICKINFO wrsTimingStatus_pickinfo
#define GT_DATA_FILL_FUNC wrsTimingStatus_data_fill
#define GT_DATA_STRUCT wrsTimingStatus_s
#define GT_GROUP_NAME "wrsTimingStatusGroup"
#define GT_INIT_FUNC init_wrsTimingStatusGroup

#include "wrsGroupTemplate.h"
