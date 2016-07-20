#include "wrsSnmp.h"
#include "snmp_mmap.h"
#include "wrsBootStatusGroup.h"

#define BOOTCOUNT_FILE "/proc/wrs-bootcount"
#define MONIT_REASON_FILE "/tmp/monit_restart_reason"

#define DOTCONFIGDIR "/tmp"
#define DOTCONFIG_SOURCE "dot-config_source"
#define DOTCONFIG_SOURCE_URL "dot-config_source_url"
#define DOTCONFIG_STATUS "dot-config_status"

#define HWINFO_FILE "/tmp/hwinfo_read_status"
#define LOAD_FPGA_STATUS_FILE "/tmp/load_fpga_status"
#define LOAD_LM32_STATUS_FILE "/tmp/load_lm32_status"

#define MODULES_FILE "/proc/modules"
/* get process list, output only process' command */
#define PROCESS_COMMAND "/bin/ps axo command"

/* get number of GW watchdog timeouts */
#define WDOG_COMMAND "/wr/bin/wrs_watchdog -g"

#define FW_UPDATE_FILE "/update/wrs-firmware.tar.checksum_error"

/* Macros for fscanf function to read line with maximum of "x" characters
 * without new line. Macro expands to something like: "%10[^\n]" */
#define LINE_READ_LEN_HELPER(x) "%"#x"[^\n]"
#define LINE_READ_LEN(x) LINE_READ_LEN_HELPER(x)
#define ARM_RCSR_ADDR 0xFFFFFD04 /* address of CPU's Reset Controller Status
				  * Register */
#define ARM_RCSR_RESET_TYPE_MASK 0x00000700 /* reset type mast in CPU's Reset
					     * Controller Status Register */

static struct pickinfo wrsBootStatus_pickinfo[] = {
	FIELD(wrsBootStatus_s, ASN_COUNTER, wrsBootCnt),
	FIELD(wrsBootStatus_s, ASN_COUNTER, wrsRebootCnt),
	FIELD(wrsBootStatus_s, ASN_INTEGER, wrsRestartReason),
	FIELD(wrsBootStatus_s, ASN_OCTET_STR, wrsFaultIP),
	FIELD(wrsBootStatus_s, ASN_OCTET_STR, wrsFaultLR),
	FIELD(wrsBootStatus_s, ASN_INTEGER, wrsConfigSource),
	FIELD(wrsBootStatus_s, ASN_OCTET_STR, wrsConfigSourceUrl),
	FIELD(wrsBootStatus_s, ASN_OCTET_STR, wrsRestartReasonMonit),
	FIELD(wrsBootStatus_s, ASN_INTEGER, wrsBootConfigStatus),
	FIELD(wrsBootStatus_s, ASN_INTEGER, wrsBootHwinfoReadout),
	FIELD(wrsBootStatus_s, ASN_INTEGER, wrsBootLoadFPGA),
	FIELD(wrsBootStatus_s, ASN_INTEGER, wrsBootLoadLM32),
	FIELD(wrsBootStatus_s, ASN_INTEGER, wrsBootKernelModulesMissing),
	FIELD(wrsBootStatus_s, ASN_INTEGER, wrsBootUserspaceDaemonsMissing),
	FIELD(wrsBootStatus_s, ASN_COUNTER, wrsGwWatchdogTimeouts),
	FIELD(wrsBootStatus_s, ASN_INTEGER, wrsFwUpdateStatus),
};

struct wrsBootStatus_s wrsBootStatus_s;

struct wrs_km_item {
	char *key;
};

/* list of kernel modules to check */
static struct wrs_km_item kernel_modules[] = {
	[0] = {.key = "wr_clocksource"},
	[1] = {"wr_pstats"},
	[2] = {"wr_rtu"},
	[3] = {"wr_nic"},
	[4] = {"wr_vic"},
	[5] = {"at91_softpwm"},
	[6] = {"g_serial"},
};

/* user space daemon list item */
struct wrs_usd_item {
	char *key;	/* process name */
	int32_t exp;	/* expected number of processes */
	uint32_t cnt;	/* number of processes found */
};

/* user space daemon list */
/* - key contain process name reported by ps command
 * - positive exp describe exact number of expected processes
 * - negative exp describe minimum number of expected processes. Usefull for
 *   processes that is hard to predict number of their instances. For example
 *   new dropbear process is spawned at ssh login.
 */
static struct wrs_usd_item userspace_daemons[] = {
	[0] = {.key = "/usr/sbin/dropbear", .exp = -1}, /* expect at least one
							 * dropbear process */
	[1] = {"/wr/bin/wrsw_hal", 2}, /* two wrsw_hal instances */
	[2] = {"/wr/bin/wrsw_rtud", 1},
	[3] = {"/wr/bin/ppsi", 1},
	[4] = {"/usr/sbin/lighttpd", 1},
	[5] = {"/usr/bin/monit", 1},
	[6] = {"/usr/sbin/snmpd", 1},
	[7] = {"/wr/bin/wrs_watchdog", 1},
};

struct wrs_bc_item {
	char *key;
	uint32_t value;
};

/* items to be read from BOOTCOUNT_FILE */
static struct wrs_bc_item boot_info[] = {
	[0] = {.key = "boot_count:"},
	[1] = {"reboot_count:"},
	[2] = {"fault_ip:"},
	[3] = {"fault_lr:"},
};

static void get_boot_info(void){
	static int run_once = 0;
	FILE *f;
	char s[80], key[40];
	uint32_t value;
	int i;
	uint32_t *rcsr_map;
	if (run_once) {
		/* boot info change only at restart */
		return;
	}
	run_once = 1;

	/* get restart reason */
	rcsr_map = create_map(ARM_RCSR_ADDR, sizeof(uint32_t));
	if (!rcsr_map) {
		snmp_log(LOG_ERR, "SNMP: " SL_ER " wrsRestartReason: unable to"
			 " map CPU's Reset Controller Status Register\n");
		/* pass error to SNMP, assign 1 */
		wrsBootStatus_s.wrsRestartReason = WRS_RESTART_REASON_ERROR;
		/* try again next time */
		run_once = 0;
	} else {
		/* reset reason values are from 0 to 4, SNMP enum is from
		 * 2 to 6, so "+ 2", 1 is reserved for error */
		wrsBootStatus_s.wrsRestartReason = 2 +
				((*rcsr_map & ARM_RCSR_RESET_TYPE_MASK) >> 8);
	}

	f = fopen(BOOTCOUNT_FILE, "r");
	if (!f) {
		snmp_log(LOG_ERR, "SNMP: " SL_ER " wrsRestartReason: failed to"
			 " open " BOOTCOUNT_FILE "\n");
		/* notify snmp about error in restart reason */
		wrsBootStatus_s.wrsRestartReason = WRS_RESTART_REASON_ERROR;
		/* try again next time */
		run_once = 0;
		return;
	}

	while (fgets(s, sizeof(s), f)) {
		if (sscanf(s, "%s %i", key, &value) != 2)
			continue; /* error... */
		for (i = 0; i < ARRAY_SIZE(boot_info); i++) {
			if (strncmp(key, boot_info[i].key, 40))
				continue;
			boot_info[i].value = value;
		}
	}
	fclose(f);

	wrsBootStatus_s.wrsBootCnt = boot_info[0].value;
	wrsBootStatus_s.wrsRebootCnt = boot_info[1].value;

	snprintf(wrsBootStatus_s.wrsFaultIP,
		 sizeof(wrsBootStatus_s.wrsFaultIP), "0x%.8x",
		 boot_info[2].value);
	snprintf(wrsBootStatus_s.wrsFaultLR,
		 sizeof(wrsBootStatus_s.wrsFaultLR), "0x%.8x",
		 boot_info[3].value);

	/* try to find whether monit caused restart */
	f = fopen(MONIT_REASON_FILE, "r");
	if (f) {
		/* when MONIT_REASON_FILE exists means that last restart was
		 * triggered by monit */
		wrsBootStatus_s.wrsRestartReason = WRS_RESTART_REASON_MONIT;
		/* try to get program that caused restart */
		fscanf(f, LINE_READ_LEN(WRS_RESTART_REASON_MONIT_LEN),
		       wrsBootStatus_s.wrsRestartReasonMonit);
		fclose(f);
	}
}

static void get_dotconfig_source(void)
{
	char buff[21]; /* 1 for null char */
	FILE *f;

	/* Check dotconfig source.
	 * dotconfig source can change in runtime, i.e. from remote to local by
	 * web-interface */
	/* read source used to get dotconfig */
	f = fopen(DOTCONFIGDIR "/" DOTCONFIG_SOURCE, "r");
	if (f) {
		/* readline without newline */
		fscanf(f, LINE_READ_LEN(20), buff);
		fclose(f);
		if (!strncmp(buff, "remote", 10))
			wrsBootStatus_s.wrsConfigSource =
						WRS_CONFIG_SOURCE_REMOTE;
		else if (!strncmp(buff, "try_dhcp", 10))
			wrsBootStatus_s.wrsConfigSource =
						WRS_CONFIG_SOURCE_TRY_DHCP;
		else if (!strncmp(buff, "force_dhcp", 10))
			wrsBootStatus_s.wrsConfigSource =
						WRS_CONFIG_SOURCE_FORCE_DHCP;
		else if (!strncmp(buff, "local", 10))
			wrsBootStatus_s.wrsConfigSource =
						WRS_CONFIG_SOURCE_LOCAL;
		else /* unknown source */
			wrsBootStatus_s.wrsConfigSource =
						WRS_CONFIG_SOURCE_ERROR;
	} else {
		/* file with source not found, probably something else caused
		 * a problem */
		wrsBootStatus_s.wrsConfigSource =
					WRS_CONFIG_SOURCE_ERROR_MINOR;
		snmp_log(LOG_ERR, "SNMP: " SL_ER " wrsConfigSource: failed to "
			 "open " DOTCONFIGDIR "/" DOTCONFIG_SOURCE "\n");
	}

	/* read dot-config's URL only when config source is not local */
	if (wrsBootStatus_s.wrsConfigSource != WRS_CONFIG_SOURCE_LOCAL) {
		/* read URL used to get dotconfig */
		f = fopen(DOTCONFIGDIR "/" DOTCONFIG_SOURCE_URL, "r");
		if (f) {
			/* readline without newline */
			fscanf(f, LINE_READ_LEN(WRS_CONFIG_SOURCE_URL_LEN),
			      wrsBootStatus_s.wrsConfigSourceUrl);
			fclose(f);
		} else {
			/* host file not found, put "error" into
			 * wrsConfigSourceHost */
			strcpy(wrsBootStatus_s.wrsConfigSourceUrl, "error");
			snmp_log(LOG_ERR, "SNMP: " SL_ER " wrsConfigSourceUrl:"
				 " failed to open "
				 DOTCONFIGDIR "/" DOTCONFIG_SOURCE_URL"\n");
		}
	} else {
		memset(wrsBootStatus_s.wrsConfigSourceUrl, 0,
		       sizeof(wrsBootStatus_s.wrsConfigSourceUrl));
	}

	f = fopen(DOTCONFIGDIR "/" DOTCONFIG_STATUS, "r");
	if (f) {
		/* readline without newline */
		fscanf(f, LINE_READ_LEN(20), buff);
		fclose(f);
		if (!strncmp(buff, "config_ok", 20))
			wrsBootStatus_s.wrsBootConfigStatus =
						WRS_CONFIG_STATUS_OK;
		else if (!strncmp(buff, "check_error", 20))
			wrsBootStatus_s.wrsBootConfigStatus =
						WRS_CONFIG_STATUS_CHECK_ERROR;
		else if (!strncmp(buff, "download_error", 20))
			wrsBootStatus_s.wrsBootConfigStatus =
						WRS_CONFIG_STATUS_DL_ERROR;
		else if (!strncmp(buff, "dhcp_error", 20))
			wrsBootStatus_s.wrsBootConfigStatus =
						WRS_CONFIG_STATUS_DHCP_ERROR;
		else
			wrsBootStatus_s.wrsBootConfigStatus =
						WRS_CONFIG_STATUS_ERROR;
	} else {
		/* status file not found, probably something else caused
		 * a problem */
		wrsBootStatus_s.wrsConfigSource =
						WRS_CONFIG_STATUS_ERROR_MINOR;
		snmp_log(LOG_ERR, "SNMP: " SL_ER " wrsConfigSource: failed to "
			 "open " DOTCONFIGDIR "/" DOTCONFIG_STATUS"\n");
	}
}

/* get status of execution of following scripts:
 * /etc/init.d/hwinfo
 * /wr/sbin/startup-mb.sh (load FPGA and LM32)
 * */
static void get_boot_scripts_status(void){
	static int run_once = 0;
	FILE *f;
	char buff[21]; /* 1 for null char */

	if (run_once) {
		/* HWinfo, load of FPGA and LM32 is done only at boot */
		return;
	}
	run_once = 1;

	/* read result of /etc/init.d/hwinfo (HWinfo) execution */
	f = fopen(HWINFO_FILE, "r");
	if (f) {
		/* readline without newline */
		fscanf(f, LINE_READ_LEN(20), buff);
		fclose(f);
		if (!strncmp(buff, "hwinfo_ok", 20))
			wrsBootStatus_s.wrsBootHwinfoReadout =
						WRS_BOOT_HWINFO_OK;
		else if (!strncmp(buff, "hwinfo_warning", 20)) {
			wrsBootStatus_s.wrsBootHwinfoReadout =
						WRS_BOOT_HWINFO_WARNING;
			/* try again next time */
			run_once = 0;
		}
		else {/*  */
			wrsBootStatus_s.wrsBootHwinfoReadout =
						WRS_BOOT_HWINFO_ERROR;
			/* try again next time */
			run_once = 0;
		}
	} else {
		/* status file not found, probably something else caused
		 * a problem */
		wrsBootStatus_s.wrsBootHwinfoReadout =
					WRS_BOOT_HWINFO_ERROR_MINOR;
		snmp_log(LOG_ERR, "SNMP: " SL_ER " wrsBootHwinfoReadout: "
			 "failed to open " HWINFO_FILE "\n");
		/* try again next time */
		run_once = 0;
	}

	/* result of loading FPGA */
	f = fopen(LOAD_FPGA_STATUS_FILE, "r");
	if (f) {
		/* readline without newline */
		fscanf(f, LINE_READ_LEN(20), buff);
		fclose(f);
		if (!strncmp(buff, "load_ok", 20))
			wrsBootStatus_s.wrsBootLoadFPGA =
						WRS_BOOT_LOAD_FPGA_OK;
		else if (!strncmp(buff, "load_file_not_found", 20)) {
			wrsBootStatus_s.wrsBootLoadFPGA =
					WRS_BOOT_LOAD_FPGA_FILE_NOT_FOUND;
			/* try again next time */
			run_once = 0;
		}
		else {/* Failed to load FPGA */
			wrsBootStatus_s.wrsBootLoadFPGA =
						WRS_BOOT_LOAD_FPGA_ERROR;
			/* try again next time */
			run_once = 0;

		}
	} else {
		/* status file not found, probably something else caused
		 * a problem */
		wrsBootStatus_s.wrsBootLoadFPGA =
					WRS_BOOT_LOAD_FPGA_ERROR_MINOR;
		snmp_log(LOG_ERR, "SNMP: " SL_ER " wrsBootLoadFPGA: failed to "
			 "open " LOAD_FPGA_STATUS_FILE "\n");
		/* try again next time */
		run_once = 0;
	}

	/* result of loading LM32 */
	f = fopen(LOAD_LM32_STATUS_FILE, "r");
	if (f) {
		/* readline without newline */
		fscanf(f, LINE_READ_LEN(20), buff);
		fclose(f);
		if (!strncmp(buff, "load_ok", 20))
			wrsBootStatus_s.wrsBootLoadLM32 =
						WRS_BOOT_LOAD_LM32_OK;
		else if (!strncmp(buff, "load_file_not_found", 20)) {
			wrsBootStatus_s.wrsBootLoadLM32 =
					WRS_BOOT_LOAD_LM32_FILE_NOT_FOUND;
			/* try again next time */
			run_once = 0;
		}
		else {/*  */
			wrsBootStatus_s.wrsBootLoadLM32 =
						WRS_BOOT_LOAD_LM32_ERROR;
			/* try again next time */
			run_once = 0;

		}
	} else {
		/* status file not found, probably something else caused
		 * a problem */
		wrsBootStatus_s.wrsBootLoadLM32 =
					WRS_BOOT_LOAD_LM32_ERROR_MINOR;
		snmp_log(LOG_ERR, "SNMP: " SL_ER " wrsBootLoadLM32: failed to "
			 "open " LOAD_FPGA_STATUS_FILE "\n");
		/* try again next time */
		run_once = 0;
	}
}

/* check if all modules are loaded */
static void get_loaded_kernel_modules_status(void)
{
	FILE *f;
	char key[41]; /* 1 for null char */
	int modules_found = 0;
	int ret = 0;
	int i;
	int guess_index = 0;
	int modules_missing;

	f = fopen(MODULES_FILE, "r");
	if (!f) {
		snmp_log(LOG_ERR, "SNMP: " SL_ER " wrsBootKernelModulesMissing"
			 ": failed to open " MODULES_FILE "\n");
		/* notify snmp about error in kernel modules */
		wrsBootStatus_s.wrsBootKernelModulesMissing =
						ARRAY_SIZE(kernel_modules);
		return;
	}

	while (ret != EOF) {
		/* read first word from line (module name) ignore rest of
		 * the line */
		ret = fscanf(f, "%40s%*[^\n]", key);
		if (ret != 1)
			continue; /* error... or EOF */

		/* try educated guess to find position in array */
		if (!strncmp(key, kernel_modules[guess_index].key, 40)) {
			modules_found++;
			guess_index++;
			continue;
		}

		for (i = 0; i < ARRAY_SIZE(kernel_modules); i++) {
			if (strncmp(key, kernel_modules[i].key, 40))
				continue;
			modules_found++;
		}
		guess_index++;
	}

	modules_missing = ARRAY_SIZE(kernel_modules) - modules_found;
	/* save number of missing modules */
	wrsBootStatus_s.wrsBootKernelModulesMissing = modules_missing;

	fclose(f);
}

/* check if daemons from userspace_daemons array are running */
static void get_daemons_status(void)
{
	FILE *f;
	char key[41]; /* 1 for null char */
	int ret = 0;
	int i;
	int processes_wrong = 0; /* number of too many or too few processes */


	/* clear user space daemon counters */
	for (i = 0; i < ARRAY_SIZE(userspace_daemons); i++) {
		userspace_daemons[i].cnt = 0;
	}

	/* Use ps command to get process list, more portable, less error prone
	 * but probably slower than manually parsing /proc/ */
	f = popen(PROCESS_COMMAND, "r");
	if (!f) {
		snmp_log(LOG_ERR, "SNMP: " SL_ER
			 " wrsBootUserspaceDaemonsMissing: failed to execute "
			 PROCESS_COMMAND "\n");
		wrsBootStatus_s.wrsBootUserspaceDaemonsMissing = 0;
		/* Notify snmp about error in processes list */
		/* Count number of expected processes */
		for (i = 0; i < ARRAY_SIZE(userspace_daemons); i++) {
			/* when exp < 0 then expect at least number of
			 * -exp processes */
			wrsBootStatus_s.wrsBootUserspaceDaemonsMissing +=
						abs(userspace_daemons[i].exp);
		}

		return;
	}

	/* count processes */
	while (ret != EOF) {
		/* read first word from line (process name) ignore rest of
		 * the line */
		ret = fscanf(f, "%40s%*[^\n]", key);
		if (ret != 1)
			continue; /* error... or EOF */

		for (i = 0; i < ARRAY_SIZE(userspace_daemons); i++) {
			if (strncmp(key, userspace_daemons[i].key, 40))
				continue;
			userspace_daemons[i].cnt++;
			break;
		}
	}

	for (i = 0; i < ARRAY_SIZE(userspace_daemons); i++) {
		if (userspace_daemons[i].exp < 0) {
			/* if exp < 0 then expect at least -exp processes,
			 * useful in situation when we cannot predict exact
			 * number of processes.
			 * NOTE: exp in this case is negative number */
			/* saturate cnt */
			if (userspace_daemons[i].cnt > (-userspace_daemons[i].exp)) {
				userspace_daemons[i].cnt =
						(-userspace_daemons[i].exp);
			}
		}
		/* Calculate delta between expected and counted number
		 * of processes. Neither too much or too few are ok.
		 * NOTE: abs "exp" too */
		processes_wrong += abs(abs(userspace_daemons[i].exp)
				       - userspace_daemons[i].cnt);
	}

	/* save number of processes missing */
	wrsBootStatus_s.wrsBootUserspaceDaemonsMissing = processes_wrong;

	pclose(f);
}

static void get_n_watchdog_timouts(void)
{
	FILE *f;

	wrsBootStatus_s.wrsGwWatchdogTimeouts = 0;

	f = popen(WDOG_COMMAND, "r");
	if (!f) {
		snmp_log(LOG_ERR, "SNMP: " SL_ER " wrsGwWatchdogTimeouts: "
			 "failed to execute " WDOG_COMMAND "\n");
		return;
	}

	/* Number of watchdog restarts is returned as a int value,
	 * if error there nothing we can do */
	fscanf(f, "%d", &wrsBootStatus_s.wrsGwWatchdogTimeouts);

	pclose(f);
}


static void get_fw_update_status(void)
{
	wrsBootStatus_s.wrsFwUpdateStatus = 0;

	if (access(FW_UPDATE_FILE, F_OK ) == 0 ) {
		/* FW_UPDATE_FILE exists, last update was not successful */
			wrsBootStatus_s.wrsFwUpdateStatus =
					WRS_FW_UPDATE_STATUS_CHECKSUM_ERROR;
	} else {
		wrsBootStatus_s.wrsFwUpdateStatus = WRS_FW_UPDATE_STATUS_OK;
	}
}


time_t wrsBootStatus_data_fill(void)
{
	static time_t time_update;
	time_t time_cur;
	time_cur = get_monotonic_sec();

	if (time_update
	    && time_cur - time_update < WRSBOOTSTATUS_CACHE_TIMEOUT) {
		/* cache not updated, return last update time */
		return time_update;
	}
	time_update = time_cur;

	get_boot_info();

	/* get dotconfig source information */
	get_dotconfig_source();

	/* get result of execution of hwinfo script */
	get_boot_scripts_status();

	/* get loaded kernel modules */
	get_loaded_kernel_modules_status();

	/* get info about running daemons */
	get_daemons_status();

	/* get info about number of gateware watchdog timeouts */
	get_n_watchdog_timouts();

	/* get info about the firmware update status */
	get_fw_update_status();

	/* there was an update, return current time */
	return time_update;
}

#define GT_OID WRSBOOTSTATUS_OID
#define GT_PICKINFO wrsBootStatus_pickinfo
#define GT_DATA_FILL_FUNC wrsBootStatus_data_fill
#define GT_DATA_STRUCT wrsBootStatus_s
#define GT_GROUP_NAME "wrsBootStatusGroup"
#define GT_INIT_FUNC init_wrsBootStatusGroup

#include "wrsGroupTemplate.h"
