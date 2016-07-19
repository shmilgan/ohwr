#ifndef WRS_PORT_STATUS_TABLE_H
#define WRS_PORT_STATUS_TABLE_H

#define WRSPORTSTATUSTABLE_CACHE_TIMEOUT 5
#define WRSPORTSTATUSTABLE_OID WRS_OID, 7, 6

#define WRS_PORT_STATUS_SFP_ERROR_SFP_OK 1	/* ok */
#define WRS_PORT_STATUS_SFP_ERROR_SFP_ERROR 2	/* error */
#define WRS_PORT_STATUS_SFP_ERROR_PORT_DOWN 3	/* ok */

#define WRS_PORT_STATUS_LINK_DOWN 1
#define WRS_PORT_STATUS_LINK_UP 2

#define WRS_PORT_STATUS_CONFIGURED_MODE_MASTER 1
#define WRS_PORT_STATUS_CONFIGURED_MODE_SLAVE 2
#define WRS_PORT_STATUS_CONFIGURED_MODE_NON_WR 3
#define WRS_PORT_STATUS_CONFIGURED_MODE_AUTO 4


struct wrsPortStatusTable_s {
	uint32_t index;		/* not reported, index fields has to be marked
				 * as not-accessible in MIB */
	char wrsPortStatusPortName[12];	/* port name */
	ClockIdentity wrsPortStatusPeer;
	/* These can't be "unsigned char" because we scanf a %i in there */
	unsigned wrsPortStatusLink;
	unsigned wrsPortStatusConfiguredMode;
	unsigned wrsPortStatusLocked;
	char wrsPortStatusSfpVN[16];	/* vendor name */
	char wrsPortStatusSfpPN[16];	/* part name */
	char wrsPortStatusSfpVS[16];	/* vendor serial */
	int wrsPortStatusSfpInDB;
	int wrsPortStatusSfpGbE;
	int wrsPortStatusSfpError;
	unsigned long wrsPortStatusPtpTxFrames;
	unsigned long wrsPortStatusPtpRxFrames;
};


extern struct wrsPortStatusTable_s wrsPortStatusTable_array[WRS_N_PORTS];
time_t wrsPortStatusTable_data_fill(unsigned int *rows);
void init_wrsPortStatusTable(void);

#endif /* WRS_PORT_STATUS_TABLE_H */
