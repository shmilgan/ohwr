#ifndef WRS_SPLL_STATUS_GROUP_H
#define WRS_SPLL_STATUS_GROUP_H

#define WRSSPLLSTATUS_CACHE_TIMEOUT 5
#define WRSSPLLSTATUS_OID WRS_OID, 6, 3, 2

struct wrsSpllStatus_s {
	int32_t wrsSpllMode;
	int32_t wrsSpllIrqCnt;
	int32_t wrsSpllSeqState;
	int32_t wrsSpllAlignState;
	int32_t wrsSpllHlock;
	int32_t wrsSpllMlock;
	int32_t wrsSpllHY;
	int32_t wrsSpllMY;
	int32_t wrsSpllDelCnt;
};

extern struct wrsSpllStatus_s wrsSpllStatus_s;
time_t wrsSpllStatus_data_fill(void);

void init_wrsSpllStatusGroup(void);
#endif /* WRS_SPLL_STATUS_GROUP_H */
