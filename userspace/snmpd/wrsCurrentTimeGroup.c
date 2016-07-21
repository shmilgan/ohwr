#include "wrsSnmp.h"
#include "wrsCurrentTimeGroup.h"
#include <libwr/util.h>

/* defines for nic-hardware.h */
#define WR_SWITCH
#define WR_IS_NODE 0
#define WR_IS_SWITCH 1
#include "../../kernel/wr_nic/nic-hardware.h"
#include "../../kernel/wbgen-regs/ppsg-regs.h"

static struct PPSG_WB *pps;

static struct pickinfo wrsCurrentTime_pickinfo[] = {
	FIELD(wrsCurrentTime_s, ASN_COUNTER64, wrsDateTAI),
	FIELD(wrsCurrentTime_s, ASN_OCTET_STR, wrsDateTAIString),
};

struct wrsCurrentTime_s wrsCurrentTime_s;

time_t wrsCurrentTime_data_fill(void)
{
	static time_t time_update;
	time_t time_cur;
	unsigned long utch, utcl, tmp1, tmp2;
	time_t t;
	struct tm tm;
	uint64_t wrs_d_current_64;

	time_cur = get_monotonic_sec();
	if (time_update
	    && time_cur - time_update < WRSCURRENTTIME_CACHE_TIMEOUT) {
		/* cache not updated, return last update time */
		return time_update;
	}
	time_update = time_cur;

	memset(&wrsCurrentTime_s, 0, sizeof(wrsCurrentTime_s));

	/* get TAI time from FPGA */

	if (!pps) /* first time, map the fpga space */
		pps = create_map(FPGA_BASE_PPSG, sizeof(*pps));
	if (!pps) {
		wrs_d_current_64 = 0;
		strcpy(wrsCurrentTime_s.wrsDateTAIString,
		       "0000-00-00-00:00:00 (failed)");
		return time_update;
	}

	do {
		utch = pps->CNTR_UTCHI;
		utcl = pps->CNTR_UTCLO;
		tmp1 = pps->CNTR_UTCHI;
		tmp2 = pps->CNTR_UTCLO;
	} while ((tmp1 != utch) || (tmp2 != utcl));

	wrs_d_current_64 = (uint64_t)(utch) << 32 | utcl;
	wrsCurrentTime_s.wrsDateTAI = wrs_d_current_64;

	t = wrs_d_current_64;
	localtime_r(&t, &tm);
	strftime(wrsCurrentTime_s.wrsDateTAIString,
		 sizeof(wrsCurrentTime_s.wrsDateTAIString),
		 "%Y-%m-%d-%H:%M:%S", &tm);

	/* there was an update, return current time */
	return time_update;
}

#define GT_OID WRSCURRENTTIME_OID
#define GT_PICKINFO wrsCurrentTime_pickinfo
#define GT_DATA_FILL_FUNC wrsCurrentTime_data_fill
#define GT_DATA_STRUCT wrsCurrentTime_s
#define GT_GROUP_NAME "wrsCurrentTimeGroup"
#define GT_INIT_FUNC init_wrsCurrentTimeGroup

#include "wrsGroupTemplate.h"
