#include "wrsSnmp.h"
#include "snmp_mmap.h"
#include "wrsBootStatusGroup.h"

#define BOOTCOUNT_FILE "/proc/wrs-bootcount"

#define DOTCONFIGDIR "/tmp"
#define DOTCONFIG_PROTO "dot-config_proto"
#define DOTCONFIG_HOST "dot-config_host"
#define DOTCONFIG_FILENAME "dot-config_filename"
#define DOTCONFIG_DOWNLOAD "dot-config_status"

#define HWINFO_FILE "/tmp/hwinfo_read_status"
#define LOAD_FPGA_STATUS_FILE "/tmp/load_fpga_status"

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
	FIELD(wrsBootStatus_s, ASN_OCTET_STR, wrsConfigSourceHost),
	FIELD(wrsBootStatus_s, ASN_OCTET_STR, wrsConfigSourceFilename),
	FIELD(wrsBootStatus_s, ASN_INTEGER, wrsBootConfigStatus),
	FIELD(wrsBootStatus_s, ASN_INTEGER, wrsBootHwinfoReadout),
	FIELD(wrsBootStatus_s, ASN_INTEGER, wrsBootLoadFPGA),
};

struct wrsBootStatus_s wrsBootStatus_s;

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
		snmp_log(LOG_ERR, "SNMP: wrsBootStatusGroup unable to map "
			 "CPU's Reset Controller Status Register\n");
		/* pass error to SNMP, assign 1 */
		wrsBootStatus_s.wrsRestartReason = WRS_RESTART_REASON_ERROR;
	} else {
		/* reset reason values are from 0 to 4, SNMP enum is from
		 * 2 to 6, so "+ 2", 1 is reserved for error */
		wrsBootStatus_s.wrsRestartReason = 2 +
				((*rcsr_map & ARM_RCSR_RESET_TYPE_MASK) >> 8);
	}

	f = fopen(BOOTCOUNT_FILE, "r");
	if (!f) {
		snmp_log(LOG_ERR, "SNMP: wrsBootStatusGroup filed to open "
			 BOOTCOUNT_FILE"\n");
		/* notify snmp about error in restart reason */
		wrsBootStatus_s.wrsRestartReason = WRS_RESTART_REASON_ERROR;
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
}

static void get_dotconfig_source(void)
{
	char buff[21]; /* 1 for null char */
	FILE *f;

	/* Check dotconfig source.
	 * dotconfig source can change in runtime, i.e. from remote to local by
	 * web-interface */
	/* read protocol used to get dotconfig */
	f = fopen(DOTCONFIGDIR "/" DOTCONFIG_PROTO, "r");
	if (f) {
		/* readline without newline */
		fscanf(f, LINE_READ_LEN(20), buff);
		fclose(f);
		if (!strncmp(buff, "tftp", 10))
			wrsBootStatus_s.wrsConfigSource =
						WRS_CONFIG_SOURCE_PROTO_TFTP;
		else if (!strncmp(buff, "http", 10))
			wrsBootStatus_s.wrsConfigSource =
						WRS_CONFIG_SOURCE_PROTO_HTTP;
		else if (!strncmp(buff, "ftp", 10))
			wrsBootStatus_s.wrsConfigSource =
						WRS_CONFIG_SOURCE_PROTO_FTP;
		else if (!strncmp(buff, "local", 10))
			wrsBootStatus_s.wrsConfigSource =
						WRS_CONFIG_SOURCE_PROTO_LOCAL;
		else /* unknown proto */
			wrsBootStatus_s.wrsConfigSource =
						WRS_CONFIG_SOURCE_PROTO_ERROR;
	} else {
		/* proto file not found, probably something else caused
		 * a problem */
		wrsBootStatus_s.wrsConfigSource =
					WRS_CONFIG_SOURCE_PROTO_ERROR_MINOR;
	}

	/* read host used to get dotconfig */
	f = fopen(DOTCONFIGDIR "/" DOTCONFIG_HOST, "r");
	if (f) {
		/* readline without newline */
		fscanf(f, LINE_READ_LEN(WRS_CONFIG_SOURCE_HOST_LEN),
		       wrsBootStatus_s.wrsConfigSourceHost);
		fclose(f);
	} else {
		/* host file not found, put "error" into wrsConfigSourceHost */
		strcpy(wrsBootStatus_s.wrsConfigSourceHost, "error");
	}

	/* read filename used to get dotconfig */
	f = fopen(DOTCONFIGDIR "/" DOTCONFIG_FILENAME, "r");
	if (f) {
		/* readline without newline */
		fscanf(f, LINE_READ_LEN(WRS_CONFIG_SOURCE_FILENAME_LEN),
		       wrsBootStatus_s.wrsConfigSourceFilename);
		fclose(f);
	} else {
		/* host file not found, put "error" into
		 * wrsConfigSourceFilename */
		strcpy(wrsBootStatus_s.wrsConfigSourceFilename, "error");
	}

	f = fopen(DOTCONFIGDIR "/" DOTCONFIG_DOWNLOAD, "r");
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
		else
			wrsBootStatus_s.wrsBootConfigStatus =
						WRS_CONFIG_STATUS_ERROR;
	} else {
		/* status file not found, probably something else caused
		 * a problem */
		wrsBootStatus_s.wrsConfigSource =
						WRS_CONFIG_STATUS_ERROR_MINOR;
	}
}

/* get status of execution of following scripts:
 * /etc/init.d/S90hwinfo
 * /wr/sbin/startup-mb.sh
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

	/* read result of S90hwinfo (HWinfo) execution */
	f = fopen(HWINFO_FILE, "r");
	if (f) {
		/* readline without newline */
		fscanf(f, LINE_READ_LEN(20), buff);
		fclose(f);
		if (!strncmp(buff, "hwinfo_ok", 20))
			wrsBootStatus_s.wrsBootHwinfoReadout =
						WRS_BOOT_HWINFO_OK;
		else if (!strncmp(buff, "hwinfo_warning", 20))
			wrsBootStatus_s.wrsBootHwinfoReadout =
						WRS_BOOT_HWINFO_WARNING;
		else /*  */
			wrsBootStatus_s.wrsBootHwinfoReadout =
						WRS_BOOT_HWINFO_ERROR;
	} else {
		/* status file not found, probably something else caused
		 * a problem */
		wrsBootStatus_s.wrsBootHwinfoReadout =
					WRS_BOOT_HWINFO_ERROR_MINOR;
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
		else if (!strncmp(buff, "load_file_not_found", 20))
			wrsBootStatus_s.wrsBootLoadFPGA =
					WRS_BOOT_LOAD_FPGA_FILE_NOT_FOUND;
		else /*  */
			wrsBootStatus_s.wrsBootLoadFPGA =
						WRS_BOOT_LOAD_FPGA_ERROR;
	} else {
		/* status file not found, probably something else caused
		 * a problem */
		wrsBootStatus_s.wrsBootLoadFPGA =
					WRS_BOOT_LOAD_FPGA_ERROR_MINOR;
	}
}


time_t wrsBootStatus_data_fill(void)
{
	static time_t time_update;
	time_t time_cur;
	time_cur = time(NULL);

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
