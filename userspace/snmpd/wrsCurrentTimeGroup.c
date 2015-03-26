#include "wrsSnmp.h"
#include "wrsCurrentTimeGroup.h"


#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

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

/* FIXME: this is copied from wr_date, should be librarized */
static void *create_map(unsigned long address, unsigned long size)
{
	unsigned long ps = getpagesize();
	unsigned long offset, fragment, len;
	void *mapaddr;
	int fd;

	fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0)
		return NULL;

	offset = address & ~(ps - 1);
	fragment = address & (ps - 1);
	len = address + size - offset;

	mapaddr = mmap(0, len, PROT_READ | PROT_WRITE,
		       MAP_SHARED, fd, offset);
	close(fd);
	if (mapaddr == MAP_FAILED)
		return NULL;
	return mapaddr + fragment;
}

time_t wrsCurrentTime_data_fill(void)
{
	static time_t time_update;
	time_t time_cur;
	unsigned long utch, utcl, tmp1, tmp2;
	time_t t;
	struct tm tm;
	uint64_t wrs_d_current_64;

	time_cur = time(NULL);
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
	/*
	  * WARNING: the current snmpd is bugged: it has
	  * endianness problems with 64 bit, and the two
	  * halves are swapped. So pre-swap them here
	  */
	wrsCurrentTime_s.wrsDateTAI =
		(wrs_d_current_64 << 32) | (wrs_d_current_64 >> 32);

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
#define GT_GROUP_NAME "wrsCurrentTime"
#define GT_INIT_FUNC init_wrsCurrentTimeGroup

#include "wrsGroupTemplate.h"
