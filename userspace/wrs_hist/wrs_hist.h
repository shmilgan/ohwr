#ifndef __WRS_HIST_H__
#define __WRS_HIST_H__

#include <time.h>
#include <inttypes.h>
#include <libwr/hal_shmem.h>

/* periods in seconds */
#define NAND_UPDATE_PERIOD 5
#define SPI_UPDATE_PERIOD 5
#define SFP_UPDATE_PERIOD 20

extern struct hist_shmem_data *hist_shmem;
extern struct wrs_shm_head *hist_head;

extern struct hist_shmem_data *hist_shmem;
extern struct wrs_shm_head *hist_shmem_hdr;
extern char *hist_sfp_nand_filename;
extern char *hist_sfp_nand_filename_backup;

int hist_shmem_init(void);

/* hist_exports.c */
int hist_wripc_init(void);
int hist_wripc_update(int ms_timeout);
int hist_check_running(void);

/* hist_sfp.c */
int hist_sfp_init(void);
void hist_sfp_insert(char *vn, char *pn, char *sn);
void hist_sfp_remove(char *vn, char *pn, char *sn);
void hist_sfp_nand_save(void);

/* hist_uptime.c */
int hist_uptime_init(void);
time_t hist_uptime_lifetime_get(void);
void hist_uptime_nand_save(void);
void hist_uptime_spi_save(void);

/* hist_hal.c */
void hal_shm_init(void);
struct hal_port_state * hal_shmem_read_ports(void);
int hal_shmem_read_temp(struct hal_temp_sensors * temp);

#endif /* __WRS_HIST_H__ */
