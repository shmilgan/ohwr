#ifndef WRS_NETWORKING_STATUS_GROUP_H
#define WRS_NETWORKING_STATUS_GROUP_H

#define WRSNETWORKINGSTATUS_OID WRS_OID, 6, 2, 3

#define WRS_SFPS_STATUS_OK 1			/* ok */
#define WRS_SFPS_STATUS_ERROR 2			/* error */
#define WRS_SFPS_STATUS_WARNING 3		/* warning, not used */
#define WRS_SFPS_STATUS_WARNING_NA 4	/* warning, at least one field is
					 * equal to 0 (NA),shouldn't happen in
					 * normal operation */
#define WRS_SFPS_STATUS_BUG 5			/* warning */

struct wrsNetworkingStatus_s {
	int wrsSFPsStatus;
};

extern struct wrsNetworkingStatus_s wrsNetworkingStatus_s;
time_t wrsNetworkingStatus_data_fill(void);
void init_wrsNetworkingStatusGroup(void);

#endif /* WRS_NETWORKING_STATUS_GROUP_H */
