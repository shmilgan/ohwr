#include "wrsSnmp.h"
#include "shmem_snmp.h"
#include "wrsCurrentTimeGroup.h"
#include "wrsOSStatus.h"
#include "wrsPortStatusTable.h"
#include "wrsPstatsTable.h"
#include "wrsPtpDataTable.h"
#include "wrsStartCntGroup.h"
#include "wrsTemperature.h"
#include "wrsVersionGroup.h"

#include "wrsGeneralStatusGroup.h"

static struct pickinfo wrsGeneralStatus_pickinfo[] = {
	FIELD(wrsGeneralStatus_s, ASN_INTEGER, wrsMainSystemStatus),
};

struct wrsGeneralStatus_s wrsGeneralStatus_s;

time_t wrsGeneralStatus_data_fill(void)
{
	static time_t time_update; /* time of last update */
	time_t time_temp; /* time when temperature data was updated */

	time_temp = wrsTemperature_data_fill();

	if (time_temp <= time_update) {
		/* cache not updated, return last update time */
		snmp_log(LOG_ERR,
			"SNMP: wrsGeneralStatusGroup cache\n");
		return time_update;
	}
	time_update = time(NULL);
	wrsCurrentTime_data_fill();
	wrsOSStatus_data_fill();
	wrsPortStatusTable_data_fill(NULL);
	wrsPstatsTable_data_fill(NULL);
	wrsPtpDataTable_data_fill(NULL);
	wrsStartCnt_data_fill();
	wrsVersion_data_fill();

	/*memset(&wrsGeneralStatus_s, 0, sizeof(wrsGeneralStatus_s));*/
	wrsGeneralStatus_s.wrsMainSystemStatus++;
	/* there was an update, return current time */
	return time_update;
}

#define GT_OID WRSGENERALSTATUS_OID
#define GT_PICKINFO wrsGeneralStatus_pickinfo
#define GT_DATA_FILL_FUNC wrsGeneralStatus_data_fill
#define GT_DATA_STRUCT wrsGeneralStatus_s
#define GT_GROUP_NAME "wrsGeneralStatusGroup"
#define GT_INIT_FUNC init_wrsGeneralStatusGroup

#include "wrsGroupTemplate.h"
