#ifndef WRS_BOOT_STATUS_GROUP_H
#define WRS_BOOT_STATUS_GROUP_H

#define WRSBOOTSTATUS_CACHE_TIMEOUT 5
#define WRSBOOTSTATUS_OID WRS_OID, 7, 1, 2

#define WRS_RESTART_REASON_MONIT_LEN 32
#define WRS_RESTART_REASON_ERROR 1		/* error */
#define WRS_RESTART_REASON_MONIT 7		/* ok */

#define WRS_CONFIG_SOURCE_URL_LEN 128
#define WRS_CONFIG_SOURCE_ERROR 1		/* error */
#define WRS_CONFIG_SOURCE_ERROR_MINOR 2		/* warning */
#define WRS_CONFIG_SOURCE_LOCAL 3		/* ok */
#define WRS_CONFIG_SOURCE_REMOTE 4		/* ok */
#define WRS_CONFIG_SOURCE_TRY_DHCP 5		/* ok */
#define WRS_CONFIG_SOURCE_FORCE_DHCP 6		/* ok */

#define WRS_CONFIG_STATUS_OK 1			/* ok */
#define WRS_CONFIG_STATUS_ERROR 2		/* error */
#define WRS_CONFIG_STATUS_DL_ERROR 3		/* error */
#define WRS_CONFIG_STATUS_CHECK_ERROR 4		/* error */
#define WRS_CONFIG_STATUS_ERROR_MINOR 5		/* warning */
#define WRS_CONFIG_STATUS_DHCP_ERROR 6		/* ok, for try_dhcp,
						 * error for force_dhcp */

#define WRS_BOOT_HWINFO_OK 1			/* ok */
#define WRS_BOOT_HWINFO_ERROR 2			/* error */
#define WRS_BOOT_HWINFO_ERROR_MINOR 3		/* warning */
#define WRS_BOOT_HWINFO_WARNING 4		/* warning */

#define WRS_BOOT_LOAD_FPGA_OK 1			/* ok */
#define WRS_BOOT_LOAD_FPGA_ERROR 2		/* error */
#define WRS_BOOT_LOAD_FPGA_ERROR_MINOR 3	/* warning */
#define WRS_BOOT_LOAD_FPGA_FILE_NOT_FOUND 4	/* error */

#define WRS_BOOT_LOAD_LM32_OK 1			/* ok */
#define WRS_BOOT_LOAD_LM32_ERROR 2		/* error */
#define WRS_BOOT_LOAD_LM32_ERROR_MINOR 3	/* warning */
#define WRS_BOOT_LOAD_LM32_FILE_NOT_FOUND 4	/* error */

#define WRS_FW_UPDATE_STATUS_OK 1		/* ok */
#define WRS_FW_UPDATE_STATUS_CHECKSUM_ERROR 2	/* warning */

#define WRS_CUSTOM_BOOT_SCRIPT_SOURCE_ERROR 1		/* error */
#define WRS_CUSTOM_BOOT_SCRIPT_SOURCE_ERROR_MINOR 2	/* warning */
#define WRS_CUSTOM_BOOT_SCRIPT_SOURCE_LOCAL 3		/* ok */
#define WRS_CUSTOM_BOOT_SCRIPT_SOURCE_REMOTE 4		/* ok */
#define WRS_CUSTOM_BOOT_SCRIPT_SOURCE_DISABLED 5	/* ok */

#define WRS_CUSTOM_BOOT_SCRIPT_STATUS_OK 1		/* ok */
#define WRS_CUSTOM_BOOT_SCRIPT_STATUS_FAILED 2		/* error */
#define WRS_CUSTOM_BOOT_SCRIPT_STATUS_WRONG_SRC 3	/* error */
#define WRS_CUSTOM_BOOT_SCRIPT_STATUS_DL_ERROR 4	/* error */
#define WRS_CUSTOM_BOOT_SCRIPT_STATUS_DISABLED 5	/* ok */
#define WRS_CUSTOM_BOOT_SCRIPT_STATUS_ERROR 6		/* error */
#define WRS_CUSTOM_BOOT_SCRIPT_STATUS_ERROR_MINOR 7	/* warning */


#define WRS_CUSTOM_BOOT_SCRIPT_SOURCE_URL_LEN 128

struct wrsBootStatus_s {
	uint32_t wrsBootCnt;		/* boots since power-on must be != 0 */
	uint32_t wrsRebootCnt;		/* soft reboots since hard reboot
					 * (i.e. caused by reset button) */
	int32_t wrsRestartReason;	/* reason of last restart */
	char wrsFaultIP[11];	/* faulty instruction pointer as string */
	char wrsFaultLR[11];	/* link register at fault as string */
	int32_t wrsConfigSource;
	char wrsConfigSourceUrl[WRS_CONFIG_SOURCE_URL_LEN + 1];
	char wrsRestartReasonMonit[WRS_RESTART_REASON_MONIT_LEN + 1];
	int32_t wrsBootConfigStatus;
	int32_t wrsBootHwinfoReadout;
	int32_t wrsBootLoadFPGA;
	int32_t wrsBootLoadLM32;
	int32_t wrsBootKernelModulesMissing;
	int32_t wrsBootUserspaceDaemonsMissing;
	int32_t wrsGwWatchdogTimeouts;
	int32_t wrsFwUpdateStatus;
	int32_t wrsCustomBootScriptSource;
	char wrsCustomBootScriptSourceUrl[WRS_CUSTOM_BOOT_SCRIPT_SOURCE_URL_LEN + 1];
	int32_t wrsCustomBootScriptStatus;
};

extern struct wrsBootStatus_s wrsBootStatus_s;
time_t wrsBootStatus_data_fill(void);

void init_wrsBootStatusGroup(void);
#endif /* WRS_BOOT_STATUS_GROUP_H */
