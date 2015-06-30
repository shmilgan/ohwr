#ifndef WRS_DISK_TABLE_H
#define WRS_DISK_TABLE_H

#define WRSDISKTABLE_CACHE_TIMEOUT 5
#define WRSDISKTABLE_OID WRS_OID, 7, 1, 6

#define WRS_MAX_N_DISKS 10

struct wrsDiskTable_s {
	uint32_t wrsDiskIndex;	/* not reported, index fields has to be marked
				 * as not-accessible in MIB */
	char wrsDiskMountPath[32];
	uint32_t wrsDiskSize;
	uint32_t wrsDiskUsed;
	uint32_t wrsDiskFree;
	uint32_t wrsDiskUseRate;
	char wrsDiskFilesystem[64];
};

extern struct wrsDiskTable_s wrsDiskTable_array[WRS_MAX_N_DISKS];

time_t wrsDiskTable_data_fill(unsigned int *rows);
void init_wrsDiskTable(void);

#endif /* WRS_DISK_TABLE_H */
