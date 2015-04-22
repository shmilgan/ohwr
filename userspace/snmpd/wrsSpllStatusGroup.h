#ifndef WRS_SPLL_STATUS_GROUP_H
#define WRS_SPLL_STATUS_GROUP_H

#define WRSSPLLSTATUS_CACHE_TIMEOUT 5
#define WRSSPLLSTATUS_OID WRS_OID, 7, 3, 2

/* values taken from softpll_ng.h */
#define WRS_SPLL_MODE_GRAND_MASTER 1
#define WRS_SPLL_MODE_MASTER 2 /* free running master */
#define WRS_SPLL_MODE_SLAVE 3

/* values taken from file spll_external.c in wrpc-sw repo */
#define WRS_SPLL_ALIGN_STATE_EXT_OFF 0
#define WRS_SPLL_ALIGN_STATE_START 1
#define WRS_SPLL_ALIGN_STATE_INIT_CSYNC 2
#define WRS_SPLL_ALIGN_STATE_WAIT_CSYNC 3
#define WRS_SPLL_ALIGN_STATE_START_ALIGNMENT 7
#define WRS_SPLL_ALIGN_STATE_WAIT_SAMPLE 4
#define WRS_SPLL_ALIGN_STATE_COMPENSATE_DELAY 5
#define WRS_SPLL_ALIGN_STATE_LOCKED 6
#define WRS_SPLL_ALIGN_STATE_START_MAIN 8

/* values taken from file softpll_ng.c in wrpc-sw repo */
#define WRS_SPLL_SEQ_STATE_START_EXT 1
#define WRS_SPLL_SEQ_STATE_WAIT_EXT 2
#define WRS_SPLL_SEQ_STATE_START_HELPER 3
#define WRS_SPLL_SEQ_STATE_WAIT_HELPER 4
#define WRS_SPLL_SEQ_STATE_START_MAIN 5
#define WRS_SPLL_SEQ_STATE_WAIT_MAIN 6
#define WRS_SPLL_SEQ_STATE_DISABLED 7
#define WRS_SPLL_SEQ_STATE_READY 8
#define WRS_SPLL_SEQ_STATE_CLEAR_DACS 9
#define WRS_SPLL_SEQ_STATE_WAIT_CLEAR_DACS 10

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
