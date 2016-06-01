#ifndef WRS_SPLL_STATUS_GROUP_H
#define WRS_SPLL_STATUS_GROUP_H

#include "softpll_export.h"

#define WRSSPLLSTATUS_CACHE_TIMEOUT 5
#define WRSSPLLSTATUS_OID WRS_OID, 7, 3, 2

/* values taken from softpll_export.h */
#define WRS_SPLL_MODE_GRAND_MASTER   SPLL_MODE_GRAND_MASTER
#define WRS_SPLL_MODE_MASTER         SPLL_MODE_FREE_RUNNING_MASTER /* free running master */
#define WRS_SPLL_MODE_SLAVE          SPLL_MODE_SLAVE
#define WRS_SPLL_MODE_DISABLED       SPLL_MODE_DISABLED

/* values taken from file spll_external.c in wrpc-sw repo */
#define WRS_SPLL_ALIGN_STATE_EXT_OFF          ALIGN_STATE_EXT_OFF
#define WRS_SPLL_ALIGN_STATE_START            ALIGN_STATE_START
#define WRS_SPLL_ALIGN_STATE_INIT_CSYNC       ALIGN_STATE_INIT_CSYNC
#define WRS_SPLL_ALIGN_STATE_WAIT_CSYNC       ALIGN_STATE_WAIT_CSYNC
#define WRS_SPLL_ALIGN_STATE_START_ALIGNMENT  ALIGN_STATE_START_ALIGNMENT
#define WRS_SPLL_ALIGN_STATE_WAIT_SAMPLE      ALIGN_STATE_WAIT_SAMPLE
#define WRS_SPLL_ALIGN_STATE_COMPENSATE_DELAY ALIGN_STATE_COMPENSATE_DELAY
#define WRS_SPLL_ALIGN_STATE_LOCKED           ALIGN_STATE_LOCKED
#define WRS_SPLL_ALIGN_STATE_START_MAIN       ALIGN_STATE_START_MAIN
#define WRS_SPLL_ALIGN_STATE_WAIT_CLKIN       ALIGN_STATE_WAIT_CLKIN
#define WRS_SPLL_ALIGN_STATE_WAIT_PLOCK       ALIGN_STATE_WAIT_PLOCK


/* values taken from file softpll_ng.c in wrpc-sw repo */
#define WRS_SPLL_SEQ_STATE_START_EXT         SEQ_START_EXT
#define WRS_SPLL_SEQ_STATE_WAIT_EXT          SEQ_WAIT_EXT
#define WRS_SPLL_SEQ_STATE_START_HELPER      SEQ_START_HELPER
#define WRS_SPLL_SEQ_STATE_WAIT_HELPER       SEQ_WAIT_HELPER
#define WRS_SPLL_SEQ_STATE_START_MAIN        SEQ_START_MAIN
#define WRS_SPLL_SEQ_STATE_WAIT_MAIN         SEQ_WAIT_MAIN
#define WRS_SPLL_SEQ_STATE_DISABLED          SEQ_DISABLED
#define WRS_SPLL_SEQ_STATE_READY             SEQ_READY
#define WRS_SPLL_SEQ_STATE_CLEAR_DACS        SEQ_CLEAR_DACS
#define WRS_SPLL_SEQ_STATE_WAIT_CLEAR_DACS   SEQ_WAIT_CLEAR_DACS

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
