#ifndef WRS_OSSTATUS_GROUP_H
#define WRS_OSSTATUS_GROUP_H

#define WRSOSSTATUS_OID WRS_OID, 6, 2, 1

#define WRS_BOOT_SUCCESSFUL_OK 1			/* ok */
#define WRS_BOOT_SUCCESSFUL_ERROR 2			/* error */
#define WRS_BOOT_SUCCESSFUL_WARNING 3			/* warning */
#define WRS_BOOT_SUCCESSFUL_WARNING_NA 4 /* warning, at least one field is
					  * equal to 0 (NA),shouldn't happen in
					  * normal operation */
#define WRS_BOOT_SUCCESSFUL_BUG 5			/* warning */

#define WRS_TEMPERATURE_WARNING_THOLD_NOT_SET 1		/* warning */
#define WRS_TEMPERATURE_WARNING_OK 2			/* ok */
#define WRS_TEMPERATURE_WARNING_TOO_HIGH 3		/* warning */

#define WRS_MEMORY_FREE_LOW_OK 1			/* ok */
#define WRS_MEMORY_FREE_LOW_ERROR 2			/* error */
#define WRS_MEMORY_FREE_LOW_WARNING 3			/* warning */
#define WRS_MEMORY_FREE_LOW_WARNING_NA 4 /* warning, at least one field is
					  * equal to 0 (NA),shouldn't happen in
					  * normal operation */

struct wrsOSStatus_s {
	int wrsBootSuccessful;
	int wrsTemperatureWarning;
	int wrsMemoryFreeLow;
};

extern struct wrsOSStatus_s wrsOSStatus_s;
time_t wrsOSStatus_data_fill(void);
void init_wrsOSStatusGroup(void);

#endif /* WRS_OSSTATUS_GROUP_H */
