#ifndef WRS_PSTATS_HC_TABLE_H
#define WRS_PSTATS_HC_TABLE_H

#define WRSPSTATSHCTABLE_CACHE_TIMEOUT 5
#define WRSPSTATSHCTABLE_OID WRS_OID, 7, 7

#define PSTATS_MAX_N_COUNTERS 39 /* maximum number of counters */
#define PSTATS_SYSCTL_PATH "/proc/sys/pstats/" /* Path to sysclt entries */
#define PSTATS_SYSCTL_INFO_FILE "info" /* file with version of pstats counters
					* and number of counters */


struct wrsPstatsHCTable_s {
	uint32_t index;		/* not reported, index fields has to be marked
				 * as not-accessible in MIB */
	char port_name[12];	/* port name of counters */
	uint64_t TXUnderrun;
	uint64_t RXOverrun;
	uint64_t RXInvalidCode;
	uint64_t RXSyncLost;
	uint64_t RXPauseFrames;
	uint64_t RXPfilterDropped;
	uint64_t RXPCSErrors;
	uint64_t RXGiantFrames;
	uint64_t RXRuntFrames;
	uint64_t RXCRCErrors;
	uint64_t RXPclass0;
	uint64_t RXPclass1;
	uint64_t RXPclass2;
	uint64_t RXPclass3;
	uint64_t RXPclass4;
	uint64_t RXPclass5;
	uint64_t RXPclass6;
	uint64_t RXPclass7;
	uint64_t TXFrames;
	uint64_t RXFrames;
	uint64_t RXDropRTUFull;
	uint64_t RXPrio0;
	uint64_t RXPrio1;
	uint64_t RXPrio2;
	uint64_t RXPrio3;
	uint64_t RXPrio4;
	uint64_t RXPrio5;
	uint64_t RXPrio6;
	uint64_t RXPrio7;
	uint64_t RTUValid;
	uint64_t RTUResponses;
	uint64_t RTUDropped;
	uint64_t FastMatchPriority;
	uint64_t FastMatchFastForward;
	uint64_t FastMatchNonForward;
	uint64_t FastMatchRespValid;
	uint64_t FullMatchRespValid;
	uint64_t Forwarded;
	uint64_t TRURespValid;
};

extern struct wrsPstatsHCTable_s pstats_array[WRS_N_PORTS];
time_t wrsPstatsHCTable_data_fill(unsigned int *rows);
void init_wrsPstatsHCTable(void);

#endif /* WRS_PSTATS_HC_TABLE_H */
