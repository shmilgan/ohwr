#ifndef WRS_CURRENT_TIME_GROUP_H
#define WRS_CURRENT_TIME_GROUP_H

#define WRSCURRENTTIME_CACHE_TIMEOUT 5
#define WRSCURRENTTIME_OID WRS_OID, 7, 1, 1

struct wrsCurrentTime_s {
	uint64_t wrsDateTAI;		/* current time in TAI */
	char wrsDateTAIString[32];	/* current time in TAI as string */
};

extern struct wrsCurrentTime_s wrsCurrentTime_s;
time_t wrsCurrentTime_data_fill(void);

void init_wrsCurrentTimeGroup(void);
#endif /* WRS_CURRENT_TIME_GROUP_H */
