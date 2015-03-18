#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/auto_nlist.h>

/* Crap! -- everybody makes them different, and even ppsi::ieee wants them */
#undef FALSE
#undef TRUE

/* conflict between definition in net-snmp-agent-includes.h (which include
 * snmp_vars.h) and ppsi.h where INST is defined as a inline function */
#undef INST
#include <ppsi/ieee1588_types.h> /* for ClockIdentity */
#include <libwr/shmem.h>
#include <ppsi/ppsi.h>
#include <libwr/hal_shmem.h>
#include <stdio.h>

#include "wrsSnmp.h"

extern struct wrs_shm_head *ppsi_head;
extern struct wr_servo_state_t *ppsi_servo;

/* Our data: globals */
static struct wrsPtpData_s {
	ClockIdentity gm_id;	/* FIXME: not implemented */
	ClockIdentity my_id;	/* FIXME: not implemented */
	int ppsi_mode;		/* FIXME: not implemented */
	char servo_state_name[32]; /* State as string */
	int servo_state;	/* state number */
	int tracking_enabled;
	char sync_source[32];	/* FIXME: not implemented */
	int64_t clock_offset;
	int32_t clock_offsetHR;	/* Human readable version of clock_offset,
				 * saturated to int limits */
	int32_t skew;
	int64_t rtt;
	uint32_t llength;
	uint32_t servo_updates;
	int32_t delta_tx_m;
	int32_t delta_rx_m;
	int32_t delta_tx_s;
	int32_t delta_rx_s;
} wrsPtpData_s;

static struct pickinfo wrsPtpData_pickinfo[] = {
	/* Warning: strings are a special case for snmp format */
	FIELD(wrsPtpData_s, ASN_OCTET_STR, gm_id),
	FIELD(wrsPtpData_s, ASN_OCTET_STR, my_id),
	FIELD(wrsPtpData_s, ASN_INTEGER, ppsi_mode),
	FIELD(wrsPtpData_s, ASN_OCTET_STR, servo_state_name),
	FIELD(wrsPtpData_s, ASN_INTEGER, servo_state),
	FIELD(wrsPtpData_s, ASN_INTEGER, tracking_enabled),
	FIELD(wrsPtpData_s, ASN_OCTET_STR, sync_source),
	FIELD(wrsPtpData_s, ASN_COUNTER64, clock_offset),
	FIELD(wrsPtpData_s, ASN_INTEGER, clock_offsetHR),
	FIELD(wrsPtpData_s, ASN_INTEGER, skew),
	FIELD(wrsPtpData_s, ASN_COUNTER64, rtt),
	FIELD(wrsPtpData_s, ASN_UNSIGNED, llength),
	FIELD(wrsPtpData_s, ASN_UNSIGNED, servo_updates),
	FIELD(wrsPtpData_s, ASN_INTEGER, delta_tx_m),
	FIELD(wrsPtpData_s, ASN_INTEGER, delta_rx_m),
	FIELD(wrsPtpData_s, ASN_INTEGER, delta_tx_s),
	FIELD(wrsPtpData_s, ASN_INTEGER, delta_rx_s),
};

static int32_t int_saturate(int64_t value)
{
	if (value >= INT32_MAX)
		return INT32_MAX;
	else if (value <= INT32_MIN)
		return INT32_MIN;

	return value;
}

int  wrsPtpData_data_fill(void)
{
	unsigned ii;
	unsigned retries = 0;
	static time_t t0, t1;

	t1 = time(NULL);
	if (t0 && t1 - t0 < 5) {/* TODO: timeout constatnt */
		/* cache not updated */
		return 1;
	}
	t0 = t1;

	memset(&wrsPtpData_s, 0, sizeof(wrsPtpData_s));
	while (1) {
		ii = wrs_shm_seqbegin(ppsi_head);

		strncpy(wrsPtpData_s.servo_state_name,
			ppsi_servo->servo_state_name,
			sizeof(ppsi_servo->servo_state_name));
		wrsPtpData_s.servo_state = ppsi_servo->state;
		/* Keep value 0 for Not available */
		wrsPtpData_s.tracking_enabled =
					1 + ppsi_servo->tracking_enabled;
		/*
		 * WARNING: the current snmpd is bugged: it has
		 * endianness problems with 64 bit, and the two
		 * halves are swapped. So pre-swap them here
		 */
		wrsPtpData_s.rtt = (ppsi_servo->picos_mu << 32)
				    | (ppsi_servo->picos_mu >> 32);
		wrsPtpData_s.clock_offset = (ppsi_servo->offset << 32)
					     | (ppsi_servo->offset >> 32);
		wrsPtpData_s.clock_offsetHR =
					int_saturate(ppsi_servo->offset);
		wrsPtpData_s.skew = ppsi_servo->skew;
		wrsPtpData_s.llength = (uint32_t)(ppsi_servo->delta_ms/1e12 *
					300e6 / 1.55);
		wrsPtpData_s.servo_updates = ppsi_servo->update_count;
		wrsPtpData_s.delta_tx_m = ppsi_servo->delta_tx_m;
		wrsPtpData_s.delta_rx_m = ppsi_servo->delta_rx_m;
		wrsPtpData_s.delta_tx_s = ppsi_servo->delta_tx_s;
		wrsPtpData_s.delta_rx_s = ppsi_servo->delta_rx_s;
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
	/* there was an update, return 0 */
	return 0;
}

#define GT_OID WRS_OID, 6, 1
#define GT_PICKINFO wrsPtpData_pickinfo
#define GT_DATA_FILL_FUNC wrsPtpData_data_fill
#define GT_DATA_STRUCT wrsPtpData_s
#define GT_GROUP_NAME "wrsPtpData"
#define GT_INIT_FUNC init_wrsPtpData

#include "wrsGroupTemplate.h"
