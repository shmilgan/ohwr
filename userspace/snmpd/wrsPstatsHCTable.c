#include "wrsSnmp.h"
#include "wrsPstatsHCTable.h"

struct wrsPstatsHCTable_s pstats_array[WRS_N_PORTS];

static struct pickinfo wrsPstatsHCTable_pickinfo[] = {
	/* Warning: strings are a special case for snmp format */
	FIELD(wrsPstatsHCTable_s, ASN_UNSIGNED, index), /* not reported */
	FIELD(wrsPstatsHCTable_s, ASN_OCTET_STR, port_name),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, TXUnderrun),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXOverrun),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXInvalidCode),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXSyncLost),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXPauseFrames),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXPfilterDropped),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXPCSErrors),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXGiantFrames),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXRuntFrames),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXCRCErrors),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXPclass0),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXPclass1),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXPclass2),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXPclass3),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXPclass4),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXPclass5),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXPclass6),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXPclass7),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, TXFrames),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXFrames),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXDropRTUFull),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXPrio0),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXPrio1),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXPrio2),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXPrio3),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXPrio4),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXPrio5),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXPrio6),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RXPrio7),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RTUValid),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RTUResponses),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, RTUDropped),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, FastMatchPriority),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, FastMatchFastForward),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, FastMatchNonForward),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, FastMatchRespValid),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, FullMatchRespValid),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, Forwarded),
	FIELD(wrsPstatsHCTable_s, ASN_COUNTER, TRURespValid),
};

time_t
wrsPstatsHCTable_data_fill(unsigned int *n_rows)
{
	FILE *f;
	char fname[32];
	int wrport;
	uint32_t i;
	uint32_t counters[PSTATS_MAX_N_COUNTERS];
	static time_t time_update;
	time_t time_cur;
	uint32_t n_counters_in_fpga;
	uint32_t counters_version;
	uint32_t tmp;
	if (n_rows)
		*n_rows = WRS_N_PORTS;

	time_cur = get_monotonic_sec();
	if (time_update
	    && time_cur - time_update < WRSPSTATSHCTABLE_CACHE_TIMEOUT) {
		/* cache not updated, return last update time */
		return time_update;
	}
	time_update = time_cur;

	/* fill array with 0xff, buy this it will be easy visible in case
	 * some counters are invalid */
	memset(&pstats_array, 0xff, sizeof(pstats_array));

	/* read counters version and number of counters */
	f = fopen(PSTATS_SYSCTL_PATH PSTATS_SYSCTL_INFO_FILE, "r");
	if (f) {
		if (fscanf(f, "%u", &counters_version) != 1) {
			/* not able to read valid counters version,
			 * assign invalid version number */
			counters_version = 0;
		}
		/* read number of counters per word, not used */
		fscanf(f, "%u", &tmp);
		if (fscanf(f, "%u", &n_counters_in_fpga) != 1) {
			/* not able to read valid number of counters,
			 * use maximum possible */
			n_counters_in_fpga = PSTATS_MAX_N_COUNTERS;
		}
		if (n_counters_in_fpga > PSTATS_MAX_N_COUNTERS) {
			/* n_counters_in_fpga cannot be bigger than
			 * PSTATS_MAX_N_COUNTERS */
			n_counters_in_fpga = PSTATS_MAX_N_COUNTERS;
		}
		fclose(f);
	} else {
		/* unable to open info file */
		/* use PSTATS_MAX_N_COUNTERS as number of counters */
		n_counters_in_fpga = PSTATS_MAX_N_COUNTERS;
		counters_version = 0; /* invalid version */
	}

	/* read pstats for each port */
	for (wrport = 0; wrport < WRS_N_PORTS; wrport++) {
		snprintf(pstats_array[wrport].port_name, 10,
				 "wr%d", wrport);
		sprintf(fname, PSTATS_SYSCTL_PATH"port%i", wrport);
		f = fopen(fname, "r");
		if (!f) {
			snmp_log(LOG_ERR,
			"SNMP: wrsPstatsTable filed to open file %s\n", fname);
			continue;
		}
		for (i = 0; i < n_counters_in_fpga; i++) {
			if (fscanf(f, "%u", &counters[i]) != 1)
				counters[i] = 0xffffffff;
		}

		fclose(f);

		/* copy counters */
		switch (counters_version) {
		case 1:
			/* copy counters to array */
			pstats_array[wrport].TXUnderrun = counters[0];
			pstats_array[wrport].RXOverrun = counters[1];
			pstats_array[wrport].RXInvalidCode = counters[2];
			pstats_array[wrport].RXSyncLost = counters[3];
			pstats_array[wrport].RXPauseFrames = counters[4];
			pstats_array[wrport].RXPfilterDropped = counters[5];
			pstats_array[wrport].RXPCSErrors = counters[6];
			pstats_array[wrport].RXGiantFrames = counters[7];
			pstats_array[wrport].RXRuntFrames = counters[8];
			pstats_array[wrport].RXCRCErrors = counters[9];
			pstats_array[wrport].RXPclass0 = counters[10];
			pstats_array[wrport].RXPclass1 = counters[11];
			pstats_array[wrport].RXPclass2 = counters[12];
			pstats_array[wrport].RXPclass3 = counters[13];
			pstats_array[wrport].RXPclass4 = counters[14];
			pstats_array[wrport].RXPclass5 = counters[15];
			pstats_array[wrport].RXPclass6 = counters[16];
			pstats_array[wrport].RXPclass7 = counters[17];
			pstats_array[wrport].TXFrames = counters[18];
			pstats_array[wrport].RXFrames = counters[19];
			pstats_array[wrport].RXDropRTUFull = counters[20];
			pstats_array[wrport].RXPrio0 = counters[21];
			pstats_array[wrport].RXPrio1 = counters[22];
			pstats_array[wrport].RXPrio2 = counters[23];
			pstats_array[wrport].RXPrio3 = counters[24];
			pstats_array[wrport].RXPrio4 = counters[25];
			pstats_array[wrport].RXPrio5 = counters[26];
			pstats_array[wrport].RXPrio6 = counters[27];
			pstats_array[wrport].RXPrio7 = counters[28];
			pstats_array[wrport].RTUValid = counters[29];
			pstats_array[wrport].RTUResponses = counters[30];
			pstats_array[wrport].RTUDropped = counters[31];
			pstats_array[wrport].FastMatchPriority = counters[32];
			pstats_array[wrport].FastMatchFastForward = counters[33];
			pstats_array[wrport].FastMatchNonForward = counters[34];
			pstats_array[wrport].FastMatchRespValid = counters[35];
			pstats_array[wrport].FullMatchRespValid = counters[36];
			pstats_array[wrport].Forwarded = counters[37];
			pstats_array[wrport].TRURespValid = counters[38];
			break;
		case 2:
		default:
			break;
		}

	}
	/* there was an update, return current time */
	return time_update;
}

#define TT_OID WRSPSTATSHCTABLE_OID
#define TT_PICKINFO wrsPstatsHCTable_pickinfo
#define TT_DATA_FILL_FUNC wrsPstatsHCTable_data_fill
#define TT_DATA_ARRAY pstats_array
#define TT_GROUP_NAME "wrsPstatsHCTable"
#define TT_INIT_FUNC init_wrsPstatsHCTable
#define TT_CACHE_TIMEOUT WRSPSTATSHCTABLE_CACHE_TIMEOUT

#include "wrsTableTemplate.h"
