#ifndef WRS_MEMORY_GROUP_H
#define WRS_MEMORY_GROUP_H

#define WRSMEMORY_CACHE_TIMEOUT 5
#define WRSMEMORY_OID WRS_OID, 7, 1, 4

struct wrsMemory_s {
	int wrsMemoryTotal;
	int wrsMemoryUsed;
	int wrsMemoryUsedPerc;
	int wrsMemoryFree;
};

extern struct wrsMemory_s wrsMemory_s;
time_t wrsMemory_data_fill(void);

void init_wrsMemoryGroup(void);
#endif /* WRS_MEMORY_GROUP_H */
