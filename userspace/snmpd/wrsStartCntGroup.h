#ifndef WRS_START_CNT_GROUP_H
#define WRS_START_CNT_GROUP_H

#define WRSSTARTCNT_CACHE_TIMEOUT 5
#define WRSSTARTCNT_OID WRS_OID, 6, 2

struct wrsStartCnt_s {
	uint32_t wrsStartCntHAL;
	uint32_t wrsStartCntPPSI;
};

extern struct wrsStartCnt_s wrsStartCnt_s;
time_t wrsStartCnt_data_fill(void);

void init_wrsStartCntGroup(void);
#endif /* WRS_START_CNT_GROUP_H */
