#include "wrsSnmp.h"
#include "wrsBootStatusGroup.h"
#include "wrsTemperatureGroup.h"
#include "wrsMemoryGroup.h"
#include "wrsCpuLoadGroup.h"
#include "wrsDiskTable.h"
#include "wrsOSStatusGroup.h"

#define WRSMEMORYFREELOW_TRESHOLD_ERROR 80
#define WRSMEMORYFREELOW_TRESHOLD_WARNING 50

/* To avoid float values for cpu load, they are multiplied by 100 */
#define WRSCPULOAD_1MIN_WARNING 200
#define WRSCPULOAD_5MIN_WARNING 150
#define WRSCPULOAD_15MIN_WARNING 100
#define WRSCPULOAD_1MIN_ERROR 300
#define WRSCPULOAD_5MIN_ERROR 200
#define WRSCPULOAD_15MIN_ERROR 150

#define WRSDISKSPACELOW_TRESHOLD_ERROR 90
#define WRSDISKSPACELOW_TRESHOLD_WARNING 80

static struct pickinfo wrsOSStatus_pickinfo[] = {
	FIELD(wrsOSStatus_s, ASN_INTEGER, wrsBootSuccessful),
	FIELD(wrsOSStatus_s, ASN_INTEGER, wrsTemperatureWarning),
	FIELD(wrsOSStatus_s, ASN_INTEGER, wrsMemoryFreeLow),
	FIELD(wrsOSStatus_s, ASN_INTEGER, wrsCpuLoadHigh),
	FIELD(wrsOSStatus_s, ASN_INTEGER, wrsDiskSpaceLow),
};

struct wrsOSStatus_s wrsOSStatus_s;

time_t wrsOSStatus_data_fill(void)
{
	static time_t time_update; /* time of last update */
	time_t time_temp; /* time when temperature data was updated */
	time_t time_boot; /* time when boot data was updated */
	time_t time_free_mem; /* time when free memory data was updated */
	time_t time_cpu_load; /* time when cpu load data was updated */
	time_t time_disk_space; /* time when disk space data was updated */
	unsigned int n_rows_disk_space; /* number of rows in wrsDiskTable_array
					 */
	unsigned int i;
	struct wrsBootStatus_s *b;
	struct wrsMemory_s *f;
	struct wrsCpuLoad_s *c;
	struct wrsDiskTable_s *d;

	time_boot = wrsBootStatus_data_fill();
	time_temp = wrsTemperature_data_fill();
	time_free_mem = wrsMemory_data_fill();
	time_cpu_load = wrsCpuLoad_data_fill();
	time_disk_space = wrsDiskTable_data_fill(&n_rows_disk_space);

	if (time_boot <= time_update
		&& time_temp <= time_update
		&& time_free_mem <= time_update
		&& time_cpu_load <= time_update
		&& time_disk_space <= time_update) {
		/* cache not updated, return last update time */
		return time_update;
	}
	time_update = get_monotonic_sec();

	memset(&wrsOSStatus_s, 0, sizeof(wrsOSStatus_s));
	/*********************************************************************\
	|************************* wrsBootSuccessful *************************|
	\*********************************************************************/
	b = &wrsBootStatus_s;
	if ( /* check if error */
		b->wrsBootCnt == 0
		|| b->wrsRestartReason == WRS_RESTART_REASON_ERROR
		|| b->wrsConfigSource == WRS_CONFIG_SOURCE_ERROR
		|| b->wrsBootConfigStatus == WRS_CONFIG_STATUS_ERROR
		|| b->wrsBootConfigStatus == WRS_CONFIG_STATUS_DL_ERROR
		|| b->wrsBootConfigStatus == WRS_CONFIG_STATUS_CHECK_ERROR
		    /* error only when dhcp failed for force_dhcp */
		|| (b->wrsBootConfigStatus == WRS_CONFIG_STATUS_DHCP_ERROR
		    && b->wrsConfigSource == WRS_CONFIG_SOURCE_FORCE_DHCP)
		|| b->wrsBootHwinfoReadout == WRS_BOOT_HWINFO_ERROR
		|| b->wrsBootLoadFPGA == WRS_BOOT_LOAD_FPGA_ERROR
		|| b->wrsBootLoadFPGA == WRS_BOOT_LOAD_FPGA_FILE_NOT_FOUND
		|| b->wrsBootLoadLM32 == WRS_BOOT_LOAD_LM32_ERROR
		|| b->wrsBootLoadLM32 == WRS_BOOT_LOAD_LM32_FILE_NOT_FOUND
		|| b->wrsBootKernelModulesMissing > 0 /* contain number of missing modules */
		|| b->wrsBootUserspaceDaemonsMissing > 0 /* contain number of missing daemons */
	) {
		wrsOSStatus_s.wrsBootSuccessful = WRS_BOOT_SUCCESSFUL_ERROR;

	} else if ( /* check if warning */
		b->wrsConfigSource == WRS_CONFIG_SOURCE_ERROR_MINOR
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
		&& b->wrsConfigSource != WRS_CONFIG_SOURCE_ERROR
		&& b->wrsConfigSource != WRS_CONFIG_SOURCE_ERROR_MINOR /* warning */
		&& (b->wrsBootConfigStatus == WRS_CONFIG_STATUS_OK
		    || b->wrsConfigSource == WRS_CONFIG_SOURCE_TRY_DHCP)
		&& b->wrsBootHwinfoReadout == WRS_BOOT_HWINFO_OK
		&& b->wrsBootLoadFPGA == WRS_BOOT_LOAD_FPGA_OK
		&& b->wrsBootLoadLM32 == WRS_BOOT_LOAD_LM32_OK
		&& b->wrsBootKernelModulesMissing == 0
		&& b->wrsBootUserspaceDaemonsMissing == 0
	) { /* OK, but check source */
		/* additional check of source */
		if (
			b->wrsConfigSource == WRS_CONFIG_SOURCE_LOCAL
			|| b->wrsConfigSource == WRS_CONFIG_SOURCE_TRY_DHCP
			|| (
				(
					b->wrsConfigSource == WRS_CONFIG_SOURCE_REMOTE
					|| b->wrsConfigSource == WRS_CONFIG_SOURCE_FORCE_DHCP
				)
				&& strnlen(b->wrsConfigSourceUrl, WRS_CONFIG_SOURCE_URL_LEN + 1)
			)
		) { /* OK */
			/* when dotconfig is local or try_dhcp or
			 * ((remote or force_dhcp) and url not empty) */
			wrsOSStatus_s.wrsBootSuccessful = WRS_BOOT_SUCCESSFUL_OK;
		} else { /* error because of empty source url */
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

	/*********************************************************************\
	|************************* wrsMemoryFreeLow  *************************|
	\*********************************************************************/
	/* Check memory usage */
	f = &wrsMemory_s;
	if (f->wrsMemoryUsedPerc > WRSMEMORYFREELOW_TRESHOLD_ERROR) {
		/* Memory usage above error threshold level */
		wrsOSStatus_s.wrsMemoryFreeLow = WRS_MEMORY_FREE_LOW_ERROR;
	} else if (f->wrsMemoryUsedPerc > WRSMEMORYFREELOW_TRESHOLD_WARNING) {
		/* Memory usage above warning threshold level */
		wrsOSStatus_s.wrsMemoryFreeLow = WRS_MEMORY_FREE_LOW_WARNING;
	} else if (f->wrsMemoryTotal == 0) {
		/* Problem with read memory size */
		wrsOSStatus_s.wrsMemoryFreeLow =
					WRS_MEMORY_FREE_LOW_WARNING_NA;
	} else {
		/* Memory usage below threshold levels */
		wrsOSStatus_s.wrsMemoryFreeLow = WRS_MEMORY_FREE_LOW_OK;
	}

	/*********************************************************************\
	|************************** wrsCpuLoadHigh  **************************|
	\*********************************************************************/
	/* Check CPU load */
	c = &wrsCpuLoad_s;
	if (c->wrsCPULoadAvg1min > WRSCPULOAD_1MIN_ERROR
	    || c->wrsCPULoadAvg5min > WRSCPULOAD_5MIN_ERROR
	    || c->wrsCPULoadAvg15min > WRSCPULOAD_15MIN_ERROR) {
		/* CPU load above error threshold level */
		wrsOSStatus_s.wrsCpuLoadHigh = WRS_CPU_LOAD_HIGH_ERROR;
	} else if (c->wrsCPULoadAvg1min > WRSCPULOAD_1MIN_WARNING
	    || c->wrsCPULoadAvg5min > WRSCPULOAD_5MIN_WARNING
	    || c->wrsCPULoadAvg15min > WRSCPULOAD_15MIN_WARNING) {
		/* CPU load above warning threshold level */
		wrsOSStatus_s.wrsCpuLoadHigh = WRS_CPU_LOAD_HIGH_WARNING;
	} else {
		/* CPU load below threshold levels */
		wrsOSStatus_s.wrsCpuLoadHigh = WRS_CPU_LOAD_HIGH_OK;
	}

	/*********************************************************************\
	|************************** wrsDiskSpaceLow **************************|
	\*********************************************************************/
	/* Check disk usage */
	d = wrsDiskTable_array;
	wrsOSStatus_s.wrsDiskSpaceLow = WRS_DISK_SPACE_LOW_OK;
	for (i = 0; i < n_rows_disk_space; i++) {
		if (d[i].wrsDiskUseRate > WRSDISKSPACELOW_TRESHOLD_ERROR) {
			/* Disk usage above error threshold level */
			wrsOSStatus_s.wrsDiskSpaceLow =
						WRS_DISK_SPACE_LOW_ERROR;
			snmp_log(LOG_ERR, "SNMP: wrsDiskSpaceLow error for "
				 "disk %s\n", d[i].wrsDiskMountPath);
			/* error, can't be worst so break */
			break;
		} else if (d[i].wrsDiskUseRate > WRSDISKSPACELOW_TRESHOLD_WARNING) {
			/* Disk usage above warning threshold level */
			wrsOSStatus_s.wrsDiskSpaceLow =
						WRS_DISK_SPACE_LOW_WARNING;
			snmp_log(LOG_ERR, "SNMP: wrsDiskSpaceLow warning for "
				 "disk %s\n", d[i].wrsDiskMountPath);
		} else if (d[i].wrsDiskSize == 0
			   && wrsOSStatus_s.wrsDiskSpaceLow == WRS_DISK_SPACE_LOW_OK) {
			/* disk size is 0, propably error while reading size,
			 * but don't overwrite regular warning */
			wrsOSStatus_s.wrsDiskSpaceLow =
						WRS_DISK_SPACE_LOW_WARNING_NA;
		}
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
