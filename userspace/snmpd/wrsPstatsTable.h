#ifndef WRS_PSTATS_TABLE_H
#define WRS_PSTATS_TABLE_H

#define WRSPSTATSTABLE_CACHE_TIMEOUT 5
#define WRSPSTATSTABLE_OID WRS_OID, 6, 4

#define PSTATS_MAX_N_COUNTERS 39 /* maximum number of counters */
#define PSTATS_SYSCTL_PATH "/proc/sys/pstats/" /* Path to sysclt entries */
#define PSTATS_SYSCTL_INFO_FILE "info" /* file with version of pstats counters
					* and number of counters */


struct wrsPstatsTable_s {
	uint32_t index;		/* not reported, index fields has to be marked
				 * as not-accessible in MIB */
	char port_name[12];	/* port name of counters */
	uint32_t TXUnderrun;
	uint32_t RXOverrun;
	uint32_t RXInvalidCode;
	uint32_t RXSyncLost;
	uint32_t RXPauseFrames;
	uint32_t RXPfilterDropped;
	uint32_t RXPCSErrors;
	uint32_t RXGiantFrames;
	uint32_t RXRuntFrames;
	uint32_t RXCRCErrors;
	uint32_t RXPclass0;
	uint32_t RXPclass1;
	uint32_t RXPclass2;
	uint32_t RXPclass3;
	uint32_t RXPclass4;
	uint32_t RXPclass5;
	uint32_t RXPclass6;
	uint32_t RXPclass7;
	uint32_t TXFrames;
	uint32_t RXFrames;
	uint32_t RXDropRTUFull;
	uint32_t RXPrio0;
	uint32_t RXPrio1;
	uint32_t RXPrio2;
	uint32_t RXPrio3;
	uint32_t RXPrio4;
	uint32_t RXPrio5;
	uint32_t RXPrio6;
	uint32_t RXPrio7;
	uint32_t RTUValid;
	uint32_t RTUResponses;
	uint32_t RTUDropped;
	uint32_t FastMatchPriority;
	uint32_t FastMatchFastForward;
	uint32_t FastMatchNonForward;
	uint32_t FastMatchRespValid;
	uint32_t FullMatchRespValid;
	uint32_t Forwarded;
	uint32_t TRURespValid;
};

extern struct wrsPstatsTable_s pstats_array[WRS_N_PORTS];
time_t wrsPstatsTable_data_fill(unsigned int *rows);
void init_wrsPstatsTable(void);

#endif /* WRS_PSTATS_TABLE_H */
