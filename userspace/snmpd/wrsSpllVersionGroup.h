#ifndef WRS_SPLL_VERSION_GROUP_H
#define WRS_SPLL_VERSION_GROUP_H

#define WRSSPLLVERSION_CACHE_TIMEOUT 5
#define WRSSPLLVERSION_OID WRS_OID, 7, 3, 1

struct wrsSpllVersion_s {
	char commit_id[32];
	char build_date[32];
	char build_by[32];
};

extern struct wrsSpllVersion_s wrsSpllVersion_s;
time_t wrsSpllVersion_data_fill(void);

void init_wrsSpllVersionGroup(void);
#endif /* WRS_SPLL_VERSION_GROUP_H */
