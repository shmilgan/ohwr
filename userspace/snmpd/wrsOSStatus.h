#ifndef WRS_WRS_OSSTATUS_H
#define WRS_WRS_OSSTATUS_H

#define WRS_TEMPERATURE_WARNING_THOLD_NOT_SET 1		/* warning */
#define WRS_TEMPERATURE_WARNING_OK 2			/* ok */
#define WRS_TEMPERATURE_WARNING_TOO_HIGH 3		/* warning */

struct wrsOSStatus_s {
	int wrsBootSuccessful;
	int wrsTemperatureWarning;
};

extern struct wrsOSStatus_s wrsOSStatus_s;
int wrsOSStatus_data_fill(void);
void init_wrsOSStatus(void);

#endif /* WRS_WRS_OSSTATUS_H */
