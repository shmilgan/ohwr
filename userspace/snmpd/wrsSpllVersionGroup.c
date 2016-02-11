#include "wrsSnmp.h"
#include "wrsSpllVersionGroup.h"
#include "softpll_ng.h"
#include "snmp_mmap.h"

#define FPGA_SPLL_STAT 0x10006800
#define SPLL_MAGIC 0x5b1157a7

static struct spll_stats *spll_stats_p;

static struct pickinfo wrsSpllVersion_pickinfo[] = {
	FIELD(wrsSpllVersion_s, ASN_OCTET_STR, commit_id),
	FIELD(wrsSpllVersion_s, ASN_OCTET_STR, build_date),
};

struct wrsSpllVersion_s wrsSpllVersion_s;

/* change endianess of the string */
static void strncpy_e(char *d, char *s, int len)
{
	int i;
	int len_4;
	uint32_t *s_i, *d_i;

	s_i = (uint32_t *)s;
	d_i = (uint32_t *)d;
	len_4 = (len+3)/4; /* ceil len to word lenth (4) */
	for (i = 0; i < len_4; i++)
		d_i[i] = ntohl(s_i[i]);
}

time_t wrsSpllVersion_data_fill(void)
{
	static time_t time_update;
	time_t time_cur;

	time_cur = get_monotonic_sec();
	if (time_update
	    && time_cur - time_update < WRSSPLLVERSION_CACHE_TIMEOUT) {
		/* cache not updated, return last update time */
		return time_update;
	}
	time_update = time_cur;

	memset(&wrsSpllVersion_s, 0, sizeof(wrsSpllVersion_s));

	if (!spll_stats_p) /* first time, map the fpga space */
		spll_stats_p = create_map(FPGA_SPLL_STAT,
					  sizeof(*spll_stats_p));
	if (!spll_stats_p) {
		/* unable to mmap */
		return time_update;
	}
	/* check magic number in SPLL stat memory */
	if (spll_stats_p->magic != SPLL_MAGIC) {
		/* wrong magic */
		snmp_log(LOG_ERR,
			 "wrsSpllVersionGroup Wrong SPLL magic number\n");
		return time_update;
	}
	/* check version of SPLL's stat structure, version fields are from
	 * version 2 */
	if (spll_stats_p->ver == 2) {
		int len;
		strncpy_e(wrsSpllVersion_s.commit_id, spll_stats_p->commit_id, 32);
		/* concatenate date and time */
		strncpy_e(wrsSpllVersion_s.build_date, spll_stats_p->build_date, 16);
		len = strnlen(wrsSpllVersion_s.build_date, 32);
		wrsSpllVersion_s.build_date[len] = ' '; /* put space instead of null */
		/* add time after added space at the end of string */
		strncpy_e(&wrsSpllVersion_s.build_date[len + 1], spll_stats_p->build_time, 16 - 1);
	}
	/* there was an update, return current time */
	return time_update;
}

#define GT_OID WRSSPLLVERSION_OID
#define GT_PICKINFO wrsSpllVersion_pickinfo
#define GT_DATA_FILL_FUNC wrsSpllVersion_data_fill
#define GT_DATA_STRUCT wrsSpllVersion_s
#define GT_GROUP_NAME "wrsSpllVersionGroup"
#define GT_INIT_FUNC init_wrsSpllVersionGroup

#include "wrsGroupTemplate.h"
