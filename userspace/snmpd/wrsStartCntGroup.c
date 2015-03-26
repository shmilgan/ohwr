#include "wrsSnmp.h"
#include "shmem_snmp.h"
#include "wrsStartCntGroup.h"

static struct pickinfo wrsStartCnt_pickinfo[] = {
	FIELD(wrsStartCnt_s, ASN_COUNTER, wrsStartCntHAL),
	FIELD(wrsStartCnt_s, ASN_COUNTER, wrsStartCntPPSI),
};

struct wrsStartCnt_s wrsStartCnt_s;

time_t wrsStartCnt_data_fill(void){
	static time_t time_update;
	time_t time_cur;

	time_cur = time(NULL);
	if (time_update
	    && time_cur - time_update < WRSSTARTCNT_CACHE_TIMEOUT) {
		/* cache not updated, return last update time */
		return time_update;
	}
	time_update = time_cur;

	memset(&wrsStartCnt_s, 0, sizeof(wrsStartCnt_s));

	/* get start counters from shmem's */
	wrsStartCnt_s.wrsStartCntHAL = hal_head->pidsequence;
	wrsStartCnt_s.wrsStartCntPPSI = ppsi_head->pidsequence;

	/* there was an update, return current time */
	return time_update;
}


#define GT_OID WRSSTARTCNT_OID
#define GT_PICKINFO wrsStartCnt_pickinfo
#define GT_DATA_FILL_FUNC wrsStartCnt_data_fill
#define GT_DATA_STRUCT wrsStartCnt_s
#define GT_GROUP_NAME "wrsStartCnt"
#define GT_INIT_FUNC init_wrsStartCntGroup

#include "wrsGroupTemplate.h"
