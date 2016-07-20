#ifndef WRS_PSTATS_HC_TABLE_H
#define WRS_PSTATS_HC_TABLE_H

#define WRSPSTATSHCTABLE_CACHE_TIMEOUT 5
#define WRSPSTATSHCTABLE_OID WRS_OID, 7, 7

#define PSTATS_MAX_N_COUNTERS 40 /* maximum number of counters */
#define PSTATS_SYSCTL_PATH "/proc/sys/pstats/" /* Path to sysclt entries */
#define PSTATS_SYSCTL_INFO_FILE "info" /* file with version of pstats counters
					* and number of counters */


struct wrsPstatsHCTable_s {
	uint32_t wrsPstatsHCIndex; /* not reported, index fields has to be marked
				  * as not-accessible in MIB */
	char wrsPstatsHCPortName[12];	/* port name of counters */
	uint64_t wrsPstatsHCTXUnderrun;
	uint64_t wrsPstatsHCRXOverrun;
	uint64_t wrsPstatsHCRXInvalidCode;
	uint64_t wrsPstatsHCRXSyncLost;
	uint64_t wrsPstatsHCRXPauseFrames;
	uint64_t wrsPstatsHCRXPfilterDropped;
	uint64_t wrsPstatsHCRXPCSErrors;
	uint64_t wrsPstatsHCRXGiantFrames;
	uint64_t wrsPstatsHCRXRuntFrames;
	uint64_t wrsPstatsHCRXCRCErrors;
	uint64_t wrsPstatsHCRXPclass0;
	uint64_t wrsPstatsHCRXPclass1;
	uint64_t wrsPstatsHCRXPclass2;
	uint64_t wrsPstatsHCRXPclass3;
	uint64_t wrsPstatsHCRXPclass4;
	uint64_t wrsPstatsHCRXPclass5;
	uint64_t wrsPstatsHCRXPclass6;
	uint64_t wrsPstatsHCRXPclass7;
	uint64_t wrsPstatsHCTXFrames;
	uint64_t wrsPstatsHCRXFrames;
	uint64_t wrsPstatsHCRXDropRTUFull;
	uint64_t wrsPstatsHCRXPrio0;
	uint64_t wrsPstatsHCRXPrio1;
	uint64_t wrsPstatsHCRXPrio2;
	uint64_t wrsPstatsHCRXPrio3;
	uint64_t wrsPstatsHCRXPrio4;
	uint64_t wrsPstatsHCRXPrio5;
	uint64_t wrsPstatsHCRXPrio6;
	uint64_t wrsPstatsHCRXPrio7;
	uint64_t wrsPstatsHCRTUValid;
	uint64_t wrsPstatsHCRTUResponses;
	uint64_t wrsPstatsHCRTUDropped;
	uint64_t wrsPstatsHCFastMatchPriority;
	uint64_t wrsPstatsHCFastMatchFastForward;
	uint64_t wrsPstatsHCFastMatchNonForward;
	uint64_t wrsPstatsHCFastMatchRespValid;
	uint64_t wrsPstatsHCFullMatchRespValid;
	uint64_t wrsPstatsHCForwarded;
	uint64_t wrsPstatsHCTRURespValid;
	uint64_t wrsPstatsHCNICTXFrames;
};

extern struct wrsPstatsHCTable_s pstats_array[WRS_N_PORTS];
time_t wrsPstatsHCTable_data_fill(unsigned int *rows);
void init_wrsPstatsHCTable(void);

#endif /* WRS_PSTATS_HC_TABLE_H */
