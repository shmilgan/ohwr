#ifndef WRS_VERSION_GROUP_H
#define WRS_VERSION_GROUP_H

#define WRSVERSION_OID WRS_OID, 6, 3

struct wrsVersion_s {
	char wrsVersions[13][32];	/* array of version strings */
};

extern struct wrsVersion_s wrsVersion_s;
time_t wrsVersion_data_fill(void);

void init_wrsVersionGroup(void);
#endif /* WRS_VERSION_GROUP_H */
