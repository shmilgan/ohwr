#ifndef __WRS_HIST_H__
#define __WRS_HIST_H__

#include <time.h>
#include <inttypes.h>
#include <libwr/hal_shmem.h>

#define DAYS_IN_WEEK	7
#define HOURS_IN_DAY	24
#define MINUTES_IN_HOUR	60
#define SECONDS_IN_HOUR	60

/* periods in seconds */
#define NAND_UPDATE_PERIOD 5 /* MINUTES_IN_HOUR * SECONDS_IN_HOUR */
#define SPI_UPDATE_PERIOD 20 /* DAYS_IN_WEEK * HOURS_IN_DAY * MINUTES_IN_HOUR
			       * SECONDS_IN_HOUR */
#define SFP_UPDATE_PERIOD 20 /* MINUTES_IN_HOUR * SECONDS_IN_HOUR */

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
void hist_sfp_nand_exit(void);
void hist_sfp_insert(char *vn, char *pn, char *sn);
void hist_sfp_remove(char *vn, char *pn, char *sn);
void hist_sfp_nand_save(void);

/* hist_up.c */
int hist_up_nand_init(void);
void hist_up_nand_exit(void);
time_t hist_up_lifetime_get(void);
void hist_up_nand_save(void);

/* hist_spi.c */
int hist_up_spi_init(void);
void hist_up_spi_exit(void);
void hist_up_spi_save(void);


/* hist_hal.c */
void hal_shm_init(void);
struct hal_port_state * hal_shmem_read_ports(void);
int hal_shmem_read_temp(struct hal_temp_sensors * temp);

#endif /* __WRS_HIST_H__ */
