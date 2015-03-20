#include "wrsSnmp.h"
#include "wrsTemperature.h"

static struct pickinfo wrsTemperature_pickinfo[] = {
	FIELD(wrsTemperature_s, ASN_INTEGER, temp_fpga),
	FIELD(wrsTemperature_s, ASN_INTEGER, temp_pll),
	FIELD(wrsTemperature_s, ASN_INTEGER, temp_psl),
	FIELD(wrsTemperature_s, ASN_INTEGER, temp_psr),
	FIELD(wrsTemperature_s, ASN_INTEGER, temp_fpga_thold),
	FIELD(wrsTemperature_s, ASN_INTEGER, temp_pll_thold),
	FIELD(wrsTemperature_s, ASN_INTEGER, temp_psl_thold),
	FIELD(wrsTemperature_s, ASN_INTEGER, temp_psr_thold),
};

struct wrsTemperature_s wrsTemperature_s;

time_t wrsTemperature_data_fill(void)
{
	unsigned ii;
	unsigned retries = 0;
	static time_t time_update;
	time_t time_cur;

	time_cur = time(NULL);
	if (time_update
	    && time_cur - time_update < WRSTEMPERATURE_CACHE_TIMEOUT) {
		/* cache not updated, return last update time */
		return time_update;
	}
	time_update = time_cur;

	memset(&wrsTemperature_s, 0, sizeof(wrsTemperature_s));
	while (1) {
		ii = wrs_shm_seqbegin(hal_head);

		wrsTemperature_s.temp_fpga = hal_shmem->temp.fpga >> 8;
		wrsTemperature_s.temp_pll = hal_shmem->temp.pll >> 8;
		wrsTemperature_s.temp_psl = hal_shmem->temp.psl >> 8;
		wrsTemperature_s.temp_psr = hal_shmem->temp.psr >> 8;
		wrsTemperature_s.temp_fpga_thold = hal_shmem->temp.fpga_thold;
		wrsTemperature_s.temp_pll_thold = hal_shmem->temp.pll_thold;
		wrsTemperature_s.temp_psl_thold = hal_shmem->temp.psl_thold;
		wrsTemperature_s.temp_psr_thold = hal_shmem->temp.psr_thold;

		retries++;
		if (retries > 100) {
			snmp_log(LOG_ERR, "%s: too many retries to read HAL\n",
				 __func__);
			retries = 0;
			}
		if (!wrs_shm_seqretry(hal_head, ii))
			break; /* consistent read */
		usleep(1000);
	}
	/* there was an update, return current time */
	return time_update;
}

#define GT_OID WRSTEMPERATURE_OID
#define GT_PICKINFO wrsTemperature_pickinfo
#define GT_DATA_FILL_FUNC wrsTemperature_data_fill
#define GT_DATA_STRUCT wrsTemperature_s
#define GT_GROUP_NAME "wrsTemperature"
#define GT_INIT_FUNC init_wrsTemperature

#include "wrsGroupTemplate.h"
