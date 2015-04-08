#include "wrsSnmp.h"
#include "wrsBootStatusGroup.h"
#include "wrsTemperatureGroup.h"
#include "wrsOSStatusGroup.h"

static struct pickinfo wrsOSStatus_pickinfo[] = {
	FIELD(wrsOSStatus_s, ASN_INTEGER, wrsBootSuccessful),
	FIELD(wrsOSStatus_s, ASN_INTEGER, wrsTemperatureWarning),
};

struct wrsOSStatus_s wrsOSStatus_s;

time_t wrsOSStatus_data_fill(void)
{
	static time_t time_update; /* time of last update */
	time_t time_temp; /* time when temperature data was updated */
	time_t time_boot; /* time when boot data was updated */
	struct wrsBootStatus_s *b;

	time_boot = wrsBootStatus_data_fill();
	time_temp = wrsTemperature_data_fill();

	if (time_boot <= time_update
		&& time_temp <= time_update) {
		/* cache not updated, return last update time */
		return time_update;
	}
	time_update = time(NULL);

	memset(&wrsOSStatus_s, 0, sizeof(wrsOSStatus_s));
	/*********************************************************************\
	|************************* wrsBootSuccessful *************************|
	\*********************************************************************/
	b = &wrsBootStatus_s;
	if ( /* check if error */
		b->wrsBootCnt == 0
		|| b->wrsRestartReason == WRS_RESTART_REASON_ERROR
		|| b->wrsConfigSource == WRS_CONFIG_SOURCE_PROTO_ERROR
		|| b->wrsBootConfigStatus == WRS_CONFIG_STATUS_ERROR
		|| b->wrsBootConfigStatus == WRS_CONFIG_STATUS_DL_ERROR
		|| b->wrsBootConfigStatus == WRS_CONFIG_STATUS_CHECK_ERROR
		|| b->wrsBootHwinfoReadout == WRS_BOOT_HWINFO_ERROR
		|| b->wrsBootLoadFPGA == WRS_BOOT_LOAD_FPGA_ERROR
		|| b->wrsBootLoadFPGA == WRS_BOOT_LOAD_FPGA_FILE_NOT_FOUND
		|| b->wrsBootLoadLM32 == WRS_BOOT_LOAD_LM32_ERROR
		|| b->wrsBootLoadLM32 == WRS_BOOT_LOAD_LM32_FILE_NOT_FOUND
		|| b->wrsBootKernelModulesMissing > 0 /* contain number of missing modules */
		|| b->wrsBootUserspaceDaemonsMissing > 0 /* contain number of missing deamons */
	) {
		wrsOSStatus_s.wrsBootSuccessful = WRS_BOOT_SUCCESSFUL_ERROR;

	} else if ( /* check if warning */
		b->wrsConfigSource == WRS_CONFIG_SOURCE_PROTO_ERROR_MINOR
		|| b->wrsBootConfigStatus == WRS_CONFIG_STATUS_ERROR_MINOR
		|| b->wrsBootHwinfoReadout == WRS_BOOT_HWINFO_ERROR_MINOR
		|| b->wrsBootHwinfoReadout == WRS_BOOT_HWINFO_WARNING
		|| b->wrsBootLoadFPGA == WRS_BOOT_LOAD_FPGA_ERROR_MINOR
		|| b->wrsBootLoadLM32 == WRS_BOOT_LOAD_LM32_ERROR_MINOR
	) { /* warning */
		wrsOSStatus_s.wrsBootSuccessful = WRS_BOOT_SUCCESSFUL_WARNING;

	} else if ( /* check if any of fields equal to 0 */
		b->wrsRestartReason == 0
		|| b->wrsConfigSource == 0
		|| b->wrsBootConfigStatus == 0
		|| b->wrsBootHwinfoReadout == 0
		|| b->wrsBootLoadFPGA == 0
		|| b->wrsBootLoadLM32 == 0
	) { /* warning NA */
		wrsOSStatus_s.wrsBootSuccessful = WRS_BOOT_SUCCESSFUL_WARNING_NA;

	} else if ( /* check if OK */
		b->wrsBootCnt != 0
		&& b->wrsRestartReason != WRS_RESTART_REASON_ERROR
		&& b->wrsConfigSource != WRS_CONFIG_SOURCE_PROTO_ERROR
		&& b->wrsConfigSource != WRS_CONFIG_SOURCE_PROTO_ERROR_MINOR /* warning */
		&& b->wrsBootConfigStatus == WRS_CONFIG_STATUS_OK
		&& b->wrsBootHwinfoReadout == WRS_BOOT_HWINFO_OK
		&& b->wrsBootLoadFPGA == WRS_BOOT_LOAD_FPGA_OK
		&& b->wrsBootLoadLM32 == WRS_BOOT_LOAD_LM32_OK
		&& b->wrsBootKernelModulesMissing == 0
		&& b->wrsBootUserspaceDaemonsMissing == 0
	) { /* OK, but check source */
		/* additional check of source */
		if (
			b->wrsConfigSource == WRS_CONFIG_SOURCE_PROTO_LOCAL
			|| (
				(
					b->wrsConfigSource == WRS_CONFIG_SOURCE_PROTO_TFTP
					|| b->wrsConfigSource == WRS_CONFIG_SOURCE_PROTO_HTTP
					|| b->wrsConfigSource == WRS_CONFIG_SOURCE_PROTO_FTP
				)
				&& strnlen(b->wrsConfigSourceHost, WRS_CONFIG_SOURCE_HOST_LEN + 1)
				&& strnlen(b->wrsConfigSourceFilename, WRS_CONFIG_SOURCE_FILENAME_LEN + 1)
			)
		) { /* OK */
			/* when dotconfig is local or (remote and host not empty and filename not empty) */
			wrsOSStatus_s.wrsBootSuccessful = WRS_BOOT_SUCCESSFUL_OK;
		} else { /* error because of source */
			wrsOSStatus_s.wrsBootSuccessful = WRS_BOOT_SUCCESSFUL_ERROR;
		}
	} else { /* probably bug in previous conditions,
		  * this should never happen */
		wrsOSStatus_s.wrsBootSuccessful = WRS_BOOT_SUCCESSFUL_BUG;
	}

	/*********************************************************************\
	|*********************** wrsTemperatureWarning ***********************|
	\*********************************************************************/
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
#define GT_GROUP_NAME "wrsOSStatusGroup"
#define GT_INIT_FUNC init_wrsOSStatusGroup

#include "wrsGroupTemplate.h"
