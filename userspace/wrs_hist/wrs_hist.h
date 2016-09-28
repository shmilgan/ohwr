#ifndef __WRS_HIST_H__
#define __WRS_HIST_H__

#include <inttypes.h>

extern struct hist_shmem_data *hist_shmem;
extern struct wrs_shm_head *hist_shmem_hdr;
int hist_wripc_init(void);
int hist_wripc_update(int ms_timeout);
int hist_check_running(void);

#endif /* __WRS_HIST_H__ */
