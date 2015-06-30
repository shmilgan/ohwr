#ifndef WRS_CPU_LOAD_GROUP_H
#define WRS_CPU_LOAD_GROUP_H

#define WRSCPULOAD_CACHE_TIMEOUT 5
#define WRSCPULOAD_OID WRS_OID, 7, 1, 5

struct wrsCpuLoad_s {
	int wrsCPULoadAvg1min;
	int wrsCPULoadAvg5min;
	int wrsCPULoadAvg15min;
};

extern struct wrsCpuLoad_s wrsCpuLoad_s;
time_t wrsCpuLoad_data_fill(void);

void init_wrsCpuLoadGroup(void);
#endif /* WRS_CPU_LOAD_GROUP_H */
