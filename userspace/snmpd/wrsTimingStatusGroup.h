#ifndef WRS_TIMING_STATUS_GROUP_H
#define WRS_TIMING_STATUS_GROUP_H

#define WRSTIMINGSTATUS_OID WRS_OID, 6, 2, 2

#define WRS_PTP_STATUS_OK 1		/* ok */
#define WRS_PTP_STATUS_ERROR 2		/* error */
#define WRS_PTP_STATUS_FR 6		/* first read, ok */

#define WRS_SOFTPLL_STATUS_OK 1			/* ok */
#define WRS_SOFTPLL_STATUS_ERROR 2		/* error */
#define WRS_SOFTPLL_STATUS_WARNING 3		/* warning */
#define WRS_SOFTPLL_STATUS_WARNING_NA 4 /* warning, at least one field is
					  * equal to 0 (NA),shouldn't happen in
					  * normal operation */
#define WRS_SOFTPLL_STATUS_BUG 5		/* warning */

#define WRS_SLAVE_LINK_STATUS_OK 1		/* ok */
#define WRS_SLAVE_LINK_STATUS_ERROR 2		/* error */
#define WRS_SLAVE_LINK_STATUS_WARNING_NA 4 /* warning, at least one field is
					  * equal to 0 (NA),shouldn't happen in
					  * normal operation */

#define WRS_PTP_FRAMES_FLOWING_OK 1		/* ok */
#define WRS_PTP_FRAMES_FLOWING_ERROR 2		/* error */
#define WRS_PTP_FRAMES_FLOWING_WARNING_NA 4 /* warning, at least one field is
					  * equal to 0 (NA),shouldn't happen in
					  * normal operation */
#define WRS_PTP_FRAMES_FLOWING_FR 6		/* ok, first run */

struct wrsTimingStatus_s {
	int wrsPTPStatus;
	int wrsSoftPLLStatus;
	int wrsSlaveLinksStatus;
	int wrsPTPFramesFlowing;
};

extern struct wrsTimingStatus_s wrsTimingStatus_s;
time_t wrsTimingStatus_data_fill(void);

void init_wrsTimingStatusGroup(void);
#endif /* WRS_TIMING_STATUS_GROUP_H */
