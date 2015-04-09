#ifndef WRS_GENERAL_STATUS_GROUP_H
#define WRS_GENERAL_STATUS_GROUP_H

#define WRSGENERALSTATUS_OID WRS_OID, 6, 1

#define WRS_OS_STATUS_OK 1			/* ok */
#define WRS_OS_STATUS_ERROR 2			/* error */
#define WRS_OS_STATUS_WARNING 3			/* warning */
#define WRS_OS_STATUS_WARNING_NA 4	/* warning, at least one field is
					 * equal to 0 (NA),shouldn't happen in
					 * normal operation */
#define WRS_OS_STATUS_BUG 5			/* warning */

struct wrsGeneralStatus_s {
	int wrsMainSystemStatus;
	int wrsOSStatus;
};

extern struct wrsGeneralStatus_s wrsGeneralStatus_s;
time_t wrsGeneralStatus_data_fill(void);
void init_wrsGeneralStatusGroup(void);

#endif /* WRS_GENERAL_STATUS_GROUP_H */
