#ifndef WRS_GENERAL_STATUS_GROUP_H
#define WRS_GENERAL_STATUS_GROUP_H

#define WRSGENERALSTATUS_OID WRS_OID, 254, 254
struct wrsGeneralStatus_s {
	int wrsMainSystemStatus;
};

extern struct wrsGeneralStatus_s wrsGeneralStatus_s;
time_t wrsGeneralStatus_data_fill(void);
void init_wrsGeneralStatusGroup(void);

#endif /* WRS_GENERAL_STATUS_GROUP_H */
