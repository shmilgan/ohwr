#ifndef WRS_PTP_DATA_TABLE_H
#define WRS_PTP_DATA_TABLE_H

#define WRSPTPDATATABLE_CACHE_TIMEOUT 5
#define WRSPTPDATATABLE_OID WRS_OID, 6, 5
/* Right now we allow only one servo instance, it will change in the future
 * when switchover is implemented */
#define WRS_MAX_N_SERVO_INSTANCES 1

struct wrsPtpDataTable_s {
	uint32_t index;		/* not reported, index fields has to be marked
				 * as not-accessible in MIB */
	char port_name[12];	/* port name on which ptp servo instance in
				 * running FIXME: not implemented */
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
};


time_t wrsPtpDataTable_data_fill(unsigned int *rows);
void init_wrsPtpDataTable(void);

#endif /* WRS_PTP_DATA_TABLE_H */
