#include "wrsSnmp.h"
#include "wrsTemperature.h"
#include "wrsOSStatus.h"

static struct pickinfo wrsOSStatus_pickinfo[] = {
	FIELD(wrsOSStatus_s, ASN_INTEGER, wrsBootSuccessful),
	FIELD(wrsOSStatus_s, ASN_INTEGER, wrsTemperatureWarning),
};

struct wrsOSStatus_s wrsOSStatus_s;

time_t wrsOSStatus_data_fill(void)
{
	static time_t time_update; /* time of last update */
	time_t time_temp; /* time when temperature data was updated */

	time_temp = wrsTemperature_data_fill();

	if (time_temp <= time_update) {
		/* cache not updated, return last update time */
		return time_update;
	}
	time_update = time(NULL);

	memset(&wrsOSStatus_s, 0, sizeof(wrsOSStatus_s));
	/* wrsTemperatureWarning */
	if (!wrsTemperature_s.temp_fpga_thold
	    && !wrsTemperature_s.temp_pll_thold
	    && !wrsTemperature_s.temp_psl_thold
	    && !wrsTemperature_s.temp_psr_thold) {
		/* no threshold are set */
		wrsOSStatus_s.wrsTemperatureWarning =
					WRS_TEMPERATURE_WARNING_THOLD_NOT_SET;
	} else {
		/* rise warning when at least one threshold level
		  * is exceeded, add 2, since 0 is readings not available, 1 is
		  * no threshold set (WRS_TEMPERATURE_WARNING_THOLD_NOT_SET)
		  * 2 is ok (WRS_TEMPERATURE_WARNING_OK), 3 is temperature too
		  * high (WRS_TEMPERATURE_WARNING_TOO_HIGH) */
		wrsOSStatus_s.wrsTemperatureWarning = 2 +
		    ((wrsTemperature_s.temp_fpga > wrsTemperature_s.temp_fpga_thold)
		    || (wrsTemperature_s.temp_pll > wrsTemperature_s.temp_pll_thold)
		    || (wrsTemperature_s.temp_psl > wrsTemperature_s.temp_psl_thold)
		    || (wrsTemperature_s.temp_psr > wrsTemperature_s.temp_psr_thold));
	}

	/* there was an update, return current time */
	return time_update;
}

#define GT_OID WRSOSSTATUS_OID
#define GT_PICKINFO wrsOSStatus_pickinfo
#define GT_DATA_FILL_FUNC wrsOSStatus_data_fill
#define GT_DATA_STRUCT wrsOSStatus_s
#define GT_GROUP_NAME "wrsOSStatus"
#define GT_INIT_FUNC init_wrsOSStatus

#include "wrsGroupTemplate.h"
