#include "wrsSnmp.h"
#include "wrsOSStatusGroup.h"
#include "wrsNetworkingStatusGroup.h"
#include "wrsGeneralStatusGroup.h"

static struct pickinfo wrsGeneralStatus_pickinfo[] = {
	FIELD(wrsGeneralStatus_s, ASN_INTEGER, wrsMainSystemStatus),
	FIELD(wrsGeneralStatus_s, ASN_INTEGER, wrsOSStatus),
	FIELD(wrsGeneralStatus_s, ASN_INTEGER, wrsTimingStatus),
	FIELD(wrsGeneralStatus_s, ASN_INTEGER, wrsNetworkingStatus),
};

struct wrsGeneralStatus_s wrsGeneralStatus_s;

time_t wrsGeneralStatus_data_fill(void)
{
	static time_t time_update; /* time of last update */
	time_t time_osstatus; /* time when wrsOSStatus data was updated */
	time_t time_networking_status; /* time when wrsNetworkingStatus data
					* was updated */
	struct wrsOSStatus_s *o;
	struct wrsNetworkingStatus_s *n;

	time_osstatus = wrsOSStatus_data_fill();
	time_networking_status = wrsNetworkingStatus_data_fill();

	if (time_osstatus <= time_update
	    && time_networking_status <= time_update) {
		/* cache not updated, return last update time */
		snmp_log(LOG_ERR,
			"SNMP: wrsGeneralStatusGroup cache\n");
		return time_update;
	}

	memset(&wrsGeneralStatus_s, 0, sizeof(wrsGeneralStatus_s));

	/*********************************************************************\
	|**************************** wrsOSStatus ****************************|
	\*********************************************************************/
	o = &wrsOSStatus_s;
	if ( /* check if error */
		o->wrsBootSuccessful == WRS_BOOT_SUCCESSFUL_ERROR
	) {
		wrsGeneralStatus_s.wrsOSStatus = WRS_OS_STATUS_ERROR;

	} else if ( /* check if warning */
		o->wrsBootSuccessful == WRS_BOOT_SUCCESSFUL_WARNING
		|| o->wrsTemperatureWarning == WRS_TEMPERATURE_WARNING_THOLD_NOT_SET
		|| o->wrsTemperatureWarning == WRS_TEMPERATURE_WARNING_TOO_HIGH
	) { /* warning */
		wrsGeneralStatus_s.wrsOSStatus = WRS_OS_STATUS_WARNING;

	} else if ( /* check if any of fields equal to 0 or WARNING_NA */
		o->wrsBootSuccessful == WRS_BOOT_SUCCESSFUL_WARNING_NA
		|| o->wrsBootSuccessful == 0
		|| o->wrsTemperatureWarning == 0
	) { /* warning NA */
		wrsGeneralStatus_s.wrsOSStatus = WRS_OS_STATUS_WARNING_NA;

	} else if ( /* check if OK */
		o->wrsBootSuccessful == WRS_BOOT_SUCCESSFUL_OK
		&& o->wrsTemperatureWarning == WRS_TEMPERATURE_WARNING_OK
	) { /* OK */
		wrsGeneralStatus_s.wrsOSStatus = WRS_OS_STATUS_OK;

	} else { /* probably bug in previous conditions,
		  * this should never happen */
		wrsGeneralStatus_s.wrsOSStatus = WRS_OS_STATUS_BUG;
	}

	/*********************************************************************\
	|************************** wrsTimingStatus **************************|
	\*********************************************************************/

	/* not implemented, always return OK */
	wrsGeneralStatus_s.wrsTimingStatus = WRS_TIMING_STATUS_OK;

	/*********************************************************************\
	|************************ wrsNetworkingStatus ************************|
	\*********************************************************************/
	n = &wrsNetworkingStatus_s;
	if ( /* check if error */
		n->wrsSFPsStatus == WRS_SFPS_STATUS_ERROR
		|| n->wrsEndpointStatus == WRS_ENDPOINT_STATUS_ERROR
		|| n->wrsSwcoreStatus == WRS_SWCORE_STATUS_ERROR
		|| n->wrsRTUStatus == WRS_RTU_STATUS_ERROR
	) {
		wrsGeneralStatus_s.wrsNetworkingStatus =
						WRS_NETWORKING_STATUS_ERROR;

	} else if ( /* check if warning */
		n->wrsSFPsStatus == WRS_SFPS_STATUS_WARNING
	) { /* warning */
		wrsGeneralStatus_s.wrsNetworkingStatus =
						WRS_NETWORKING_STATUS_WARNING;

	} else if ( /* check if any of fields equal to 0 or WARNING_NA */
		n->wrsSFPsStatus == WRS_SFPS_STATUS_WARNING_NA
		|| n->wrsSFPsStatus == 0
		|| n->wrsEndpointStatus == 0
		|| n->wrsSwcoreStatus == 0
		|| n->wrsRTUStatus == 0
	) { /* warning NA */
		wrsGeneralStatus_s.wrsNetworkingStatus =
					      WRS_NETWORKING_STATUS_WARNING_NA;

	} else if ( /* check if OK, FR (first read) is also ok */
		n->wrsSFPsStatus == WRS_SFPS_STATUS_OK
		&& (n->wrsEndpointStatus == WRS_ENDPOINT_STATUS_OK
		    || n->wrsEndpointStatus == WRS_ENDPOINT_STATUS_FR) /* FR*/
		&& (n->wrsSwcoreStatus == WRS_SWCORE_STATUS_OK
		    || n->wrsSwcoreStatus == WRS_SWCORE_STATUS_FR) /* FR */
		&& (n->wrsRTUStatus == WRS_RTU_STATUS_OK
		    || n->wrsRTUStatus == WRS_RTU_STATUS_FR) /* FR */
	) { /* OK */
		wrsGeneralStatus_s.wrsNetworkingStatus =
						WRS_NETWORKING_STATUS_OK;

	} else { /* probably bug in previous conditions,
		  * this should never happen */
		wrsGeneralStatus_s.wrsNetworkingStatus =
						WRS_NETWORKING_STATUS_BUG;
	}

	time_update = time(NULL);

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
