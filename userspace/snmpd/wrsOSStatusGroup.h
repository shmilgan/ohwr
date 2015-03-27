#ifndef WRS_OSSTATUS_GROUP_H
#define WRS_OSSTATUS_GROUP_H

#define WRSOSSTATUS_OID WRS_OID, 254, 1, 1

#define WRS_TEMPERATURE_WARNING_THOLD_NOT_SET 1		/* warning */
#define WRS_TEMPERATURE_WARNING_OK 2			/* ok */
#define WRS_TEMPERATURE_WARNING_TOO_HIGH 3		/* warning */

struct wrsOSStatus_s {
	int wrsBootSuccessful;
	int wrsTemperatureWarning;
};

extern struct wrsOSStatus_s wrsOSStatus_s;
time_t wrsOSStatus_data_fill(void);
void init_wrsOSStatusGroup(void);

#endif /* WRS_OSSTATUS_GROUP_H */
