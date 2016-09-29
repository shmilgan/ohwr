#ifndef __WRS_HIST_H__
#define __WRS_HIST_H__

#include <time.h>
#include <inttypes.h>

extern struct hist_shmem_data *hist_shmem;
extern struct wrs_shm_head *hist_shmem_hdr;
int hist_wripc_init(void);
int hist_wripc_update(int ms_timeout);
int hist_check_running(void);

/* hist_sfp.c */
void hist_sfp_insert(char *vn, char *pn, char *sn);
void hist_sfp_update(char *vn, char *pn, char *sn);
void hist_sfp_remove(char *vn, char *pn, char *sn);

/* hist_uptime.c */
int hist_uptime_init(void);
time_t hist_uptime_lifetime_get(void);
void hist_uptime_nand_save(void);
void hist_uptime_spi_save(void);

#endif /* __WRS_HIST_H__ */
