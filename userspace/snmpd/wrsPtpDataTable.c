#include "wrsSnmp.h"
#include "snmp_shmem.h"
#include "wrsPtpDataTable.h"

struct wrsPtpDataTable_s wrsPtpDataTable_array[WRS_MAX_N_SERVO_INSTANCES];

static struct pickinfo wrsPtpDataTable_pickinfo[] = {
	/* Warning: strings are a special case for snmp format */
	FIELD(wrsPtpDataTable_s, ASN_UNSIGNED, wrsPtpDataIndex), /* not reported */
	FIELD(wrsPtpDataTable_s, ASN_OCTET_STR, wrsPtpPortName),
	FIELD(wrsPtpDataTable_s, ASN_OCTET_STR, wrsPtpGrandmasterID),
	FIELD(wrsPtpDataTable_s, ASN_OCTET_STR, wrsPtpOwnID),
	FIELD(wrsPtpDataTable_s, ASN_INTEGER, wrsPtpMode),
	FIELD(wrsPtpDataTable_s, ASN_OCTET_STR, wrsPtpServoState),
	FIELD(wrsPtpDataTable_s, ASN_INTEGER, wrsPtpServoStateN),
	FIELD(wrsPtpDataTable_s, ASN_INTEGER, wrsPtpPhaseTracking),
	FIELD(wrsPtpDataTable_s, ASN_OCTET_STR, wrsPtpSyncSource),
	FIELD(wrsPtpDataTable_s, ASN_COUNTER64, wrsPtpClockOffsetPs),
	FIELD(wrsPtpDataTable_s, ASN_INTEGER, wrsPtpClockOffsetPsHR),
	FIELD(wrsPtpDataTable_s, ASN_INTEGER, wrsPtpSkew),
	FIELD(wrsPtpDataTable_s, ASN_COUNTER64, wrsPtpRTT),
	FIELD(wrsPtpDataTable_s, ASN_UNSIGNED, wrsPtpLinkLength),
	FIELD(wrsPtpDataTable_s, ASN_COUNTER, wrsPtpServoUpdates),
	FIELD(wrsPtpDataTable_s, ASN_INTEGER, wrsPtpDeltaTxM),
	FIELD(wrsPtpDataTable_s, ASN_INTEGER, wrsPtpDeltaRxM),
	FIELD(wrsPtpDataTable_s, ASN_INTEGER, wrsPtpDeltaTxS),
	FIELD(wrsPtpDataTable_s, ASN_INTEGER, wrsPtpDeltaRxS),
	FIELD(wrsPtpDataTable_s, ASN_COUNTER, wrsPtpServoStateErrCnt),
	FIELD(wrsPtpDataTable_s, ASN_COUNTER, wrsPtpClockOffsetErrCnt),
	FIELD(wrsPtpDataTable_s, ASN_COUNTER, wrsPtpRTTErrCnt),
	FIELD(wrsPtpDataTable_s, ASN_COUNTER64, wrsPtpServoUpdateTime),

};

static int32_t int_saturate(int64_t value)
{
	if (value >= INT32_MAX)
		return INT32_MAX;
	else if (value <= INT32_MIN)
		return INT32_MIN;

	return value;
}

time_t wrsPtpDataTable_data_fill(unsigned int *n_rows)
{
	unsigned ii;
	unsigned retries = 0;
	static time_t time_update;
	time_t time_cur;
	static int n_rows_local = 0;

	/* number of rows does not change for wrsPortStatusTable */
	if (n_rows)
		*n_rows = n_rows_local;

	time_cur = get_monotonic_sec();
	if (time_update
	    && time_cur - time_update < WRSPTPDATATABLE_CACHE_TIMEOUT) {
		/* cache not updated, return last update time */
		return time_update;
	}
	time_update = time_cur;

	memset(&wrsPtpDataTable_array, 0, sizeof(wrsPtpDataTable_array));

	/* check whether shmem is available */
	if (!shmem_ready_ppsi()) {
		snmp_log(LOG_ERR, "%s: Unable to read PPSI's shmem\n", __func__);
		n_rows_local = 0;
		return time_update;
	} else {
		n_rows_local = WRS_MAX_N_SERVO_INSTANCES;
	}

	if (n_rows)
		*n_rows = n_rows_local;

	/* assume that there is only one servo, will change when switchover is
	 * implemented */
	while (1) {
		ii = wrs_shm_seqbegin(ppsi_head);

		strncpy(wrsPtpDataTable_array[0].wrsPtpServoState,
			ppsi_servo->servo_state_name,
			sizeof(ppsi_servo->servo_state_name));
		wrsPtpDataTable_array[0].wrsPtpServoStateN = ppsi_servo->state;
		/* Keep value 0 for Not available */
		wrsPtpDataTable_array[0].wrsPtpPhaseTracking =
					1 + ppsi_servo->tracking_enabled;
		wrsPtpDataTable_array[0].wrsPtpRTT = ppsi_servo->picos_mu;
		wrsPtpDataTable_array[0].wrsPtpClockOffsetPs =
						ppsi_servo->offset;
		wrsPtpDataTable_array[0].wrsPtpClockOffsetPsHR =
					int_saturate(ppsi_servo->offset);
		wrsPtpDataTable_array[0].wrsPtpSkew =
						int_saturate(ppsi_servo->skew);
		wrsPtpDataTable_array[0].wrsPtpLinkLength =
			(uint32_t)(ppsi_servo->delta_ms/1e12 * 300e6 / 1.55);
		wrsPtpDataTable_array[0].wrsPtpServoUpdates =
						ppsi_servo->update_count;
		wrsPtpDataTable_array[0].wrsPtpDeltaTxM = ppsi_servo->delta_tx_m;
		wrsPtpDataTable_array[0].wrsPtpDeltaRxM = ppsi_servo->delta_rx_m;
		wrsPtpDataTable_array[0].wrsPtpDeltaTxS = ppsi_servo->delta_tx_s;
		wrsPtpDataTable_array[0].wrsPtpDeltaRxS = ppsi_servo->delta_rx_s;
		wrsPtpDataTable_array[0].wrsPtpServoStateErrCnt = ppsi_servo->n_err_state;
		wrsPtpDataTable_array[0].wrsPtpClockOffsetErrCnt = ppsi_servo->n_err_offset;
		wrsPtpDataTable_array[0].wrsPtpRTTErrCnt = ppsi_servo->n_err_delta_rtt;
		wrsPtpDataTable_array[0].wrsPtpServoUpdateTime =
			(((uint64_t) ppsi_servo->update_time.seconds) * 1000000000LL)
			+ ppsi_servo->update_time.nanoseconds;
		retries++;
		if (retries > 100) {
			snmp_log(LOG_ERR, "%s: too many retries to read PPSI\n",
				 __func__);
			retries = 0;
			}
		if (!wrs_shm_seqretry(ppsi_head, ii))
			break; /* consistent read */
		usleep(1000);
	}
	/* there was an update, return current time */
	return time_update;
}

#define TT_OID WRSPTPDATATABLE_OID
#define TT_PICKINFO wrsPtpDataTable_pickinfo
#define TT_DATA_FILL_FUNC wrsPtpDataTable_data_fill
#define TT_DATA_ARRAY wrsPtpDataTable_array
#define TT_GROUP_NAME "wrsPtpDataTable"
#define TT_INIT_FUNC init_wrsPtpDataTable
#define TT_CACHE_TIMEOUT WRSPTPDATATABLE_CACHE_TIMEOUT

#include "wrsTableTemplate.h"
