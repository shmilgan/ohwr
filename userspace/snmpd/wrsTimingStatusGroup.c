#include "wrsSnmp.h"
#include "wrsSpllStatusGroup.h"
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
	struct wrsSpllStatus_s *s;

	time_spll = wrsSpllStatus_data_fill();

	if (time_spll <= time_update) {
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
