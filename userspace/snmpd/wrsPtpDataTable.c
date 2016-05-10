#include "wrsSnmp.h"
#include "snmp_shmem.h"
#include "wrsPtpDataTable.h"

struct wrsPtpDataTable_s wrsPtpDataTable_array[WRS_MAX_N_SERVO_INSTANCES];

static struct pickinfo wrsPtpDataTable_pickinfo[] = {
	/* Warning: strings are a special case for snmp format */
	FIELD(wrsPtpDataTable_s, ASN_UNSIGNED, index), /* not reported */
	FIELD(wrsPtpDataTable_s, ASN_OCTET_STR, port_name),
	FIELD(wrsPtpDataTable_s, ASN_OCTET_STR, gm_id),
	FIELD(wrsPtpDataTable_s, ASN_OCTET_STR, my_id),
	FIELD(wrsPtpDataTable_s, ASN_INTEGER, ppsi_mode),
	FIELD(wrsPtpDataTable_s, ASN_OCTET_STR, servo_state_name),
	FIELD(wrsPtpDataTable_s, ASN_INTEGER, servo_state),
	FIELD(wrsPtpDataTable_s, ASN_INTEGER, tracking_enabled),
	FIELD(wrsPtpDataTable_s, ASN_OCTET_STR, sync_source),
	FIELD(wrsPtpDataTable_s, ASN_COUNTER64, clock_offset),
	FIELD(wrsPtpDataTable_s, ASN_INTEGER, clock_offsetHR),
	FIELD(wrsPtpDataTable_s, ASN_INTEGER, skew),
	FIELD(wrsPtpDataTable_s, ASN_COUNTER64, rtt),
	FIELD(wrsPtpDataTable_s, ASN_UNSIGNED, llength),
	FIELD(wrsPtpDataTable_s, ASN_COUNTER, servo_updates),
	FIELD(wrsPtpDataTable_s, ASN_INTEGER, delta_tx_m),
	FIELD(wrsPtpDataTable_s, ASN_INTEGER, delta_rx_m),
	FIELD(wrsPtpDataTable_s, ASN_INTEGER, delta_tx_s),
	FIELD(wrsPtpDataTable_s, ASN_INTEGER, delta_rx_s),
	FIELD(wrsPtpDataTable_s, ASN_COUNTER, n_err_state),
	FIELD(wrsPtpDataTable_s, ASN_COUNTER, n_err_offset),
	FIELD(wrsPtpDataTable_s, ASN_COUNTER, n_err_delta_rtt),
	FIELD(wrsPtpDataTable_s, ASN_COUNTER64, update_time),

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

		strncpy(wrsPtpDataTable_array[0].servo_state_name,
			ppsi_servo->servo_state_name,
			sizeof(ppsi_servo->servo_state_name));
		wrsPtpDataTable_array[0].servo_state = ppsi_servo->state;
		/* Keep value 0 for Not available */
		wrsPtpDataTable_array[0].tracking_enabled =
					1 + ppsi_servo->tracking_enabled;
		/*
		 * WARNING: the current snmpd is bugged: it has
		 * endianness problems with 64 bit, and the two
		 * halves are swapped. So pre-swap them here
		 */
		wrsPtpDataTable_array[0].rtt = (ppsi_servo->picos_mu << 32)
				    | (ppsi_servo->picos_mu >> 32);
		wrsPtpDataTable_array[0].clock_offset =
						(ppsi_servo->offset << 32)
						| (ppsi_servo->offset >> 32);
		wrsPtpDataTable_array[0].clock_offsetHR =
					int_saturate(ppsi_servo->offset);
		wrsPtpDataTable_array[0].skew = int_saturate(ppsi_servo->skew);
		wrsPtpDataTable_array[0].llength =
			(uint32_t)(ppsi_servo->delta_ms/1e12 * 300e6 / 1.55);
		wrsPtpDataTable_array[0].servo_updates =
						ppsi_servo->update_count;
		wrsPtpDataTable_array[0].delta_tx_m = ppsi_servo->delta_tx_m;
		wrsPtpDataTable_array[0].delta_rx_m = ppsi_servo->delta_rx_m;
		wrsPtpDataTable_array[0].delta_tx_s = ppsi_servo->delta_tx_s;
		wrsPtpDataTable_array[0].delta_rx_s = ppsi_servo->delta_rx_s;
		wrsPtpDataTable_array[0].n_err_state = ppsi_servo->n_err_state;
		wrsPtpDataTable_array[0].n_err_offset = ppsi_servo->n_err_offset;
		wrsPtpDataTable_array[0].n_err_delta_rtt = ppsi_servo->n_err_delta_rtt;
		wrsPtpDataTable_array[0].update_time =
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
