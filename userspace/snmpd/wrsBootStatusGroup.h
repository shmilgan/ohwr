#ifndef WRS_BOOT_STATUS_GROUP_H
#define WRS_BOOT_STATUS_GROUP_H

#define WRSBOOTSTATUS_CACHE_TIMEOUT 5
#define WRSBOOTSTATUS_OID WRS_OID, 7, 1, 2

#define WRS_RESTART_REASON_ERROR 1		/* error */

#define WRS_CONFIG_SOURCE_HOST_LEN 64
#define WRS_CONFIG_SOURCE_FILENAME_LEN 128
#define WRS_CONFIG_SOURCE_PROTO_ERROR 1		/* error */
#define WRS_CONFIG_SOURCE_PROTO_ERROR_MINOR 2	/* warning */
#define WRS_CONFIG_SOURCE_PROTO_LOCAL 3		/* ok */
/* below proto are ok, if host and filename not empty */
#define WRS_CONFIG_SOURCE_PROTO_TFTP 4
#define WRS_CONFIG_SOURCE_PROTO_HTTP 5
#define WRS_CONFIG_SOURCE_PROTO_FTP 6

struct wrsBootStatus_s {
	uint32_t wrsBootCnt;		/* boots since power-on must be != 0 */
	uint32_t wrsRebootCnt;		/* soft reboots since hard reboot
					 * (i.e. caused by reset button) */
	int32_t wrsRestartReason;	/* reason of last restart */
	char wrsFaultIP[11];	/* faulty instruction pointer as string */
	char wrsFaultLR[11];	/* link register at fault as string */
	int32_t wrsConfigSource;
	char wrsConfigSourceHost[WRS_CONFIG_SOURCE_HOST_LEN+1];
	char wrsConfigSourceFilename[WRS_CONFIG_SOURCE_FILENAME_LEN+1];
};

extern struct wrsBootStatus_s wrsBootStatus_s;
time_t wrsBootStatus_data_fill(void);

void init_wrsBootStatusGroup(void);
#endif /* WRS_BOOT_STATUS_GROUP_H */
