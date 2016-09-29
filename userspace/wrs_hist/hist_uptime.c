#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#define __USE_GNU /* for O_NOATIME flag */
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <libwr/util.h>
#include <libwr/shmem.h>
#include <libwr/hist_shmem.h>
#include <libwr/wrs-msg.h>

#include "wrs_hist.h"

#define HIST_RUN_NAND_FILENAME "/update/lifetime_stats.bin"

static time_t init_time_monotonic;
static time_t uptime_stored;

static time_t hist_uptime_nand_read(void);
static void hist_uptime_nand_write(struct wrs_hist_run_nand *data);
static void hist_uptime_get_temp(uint8_t temp[4]);
static void hist_uptime_nand_update(void);

int hist_uptime_init(void)
{
	init_time_monotonic = get_monotonic_sec();
	uptime_stored = hist_uptime_nand_read();
	hist_uptime_nand_update();
	return 0;
}

time_t hist_uptime_lifetime_get(void)
{
	time_t uptime;
	uptime = get_monotonic_sec() - init_time_monotonic;
	return uptime_stored + uptime;
}


static time_t hist_uptime_nand_read(void)
{
	int fd;
	struct wrs_hist_run_nand data_run_nand;
	int ret;
	uint32_t lifetime = 0;

	/* use O_NOATIME to avoid update of last access time */
	fd = open(HIST_RUN_NAND_FILENAME, O_RDONLY | O_NOATIME);
	if (fd < 0) {
		pr_error("Unable to read the file %s\n",
			 HIST_RUN_NAND_FILENAME);
		return 0;
	}
	while (1) {
		ret = read(fd, &data_run_nand, sizeof(data_run_nand));
		if (ret == 0) {
			/* End of file */
			break;
		} else if (ret < 0) {
			pr_error("Read error from the file %s, ret %d, "
				 "error(%d) %s:\n",
				 HIST_RUN_NAND_FILENAME, ret, errno,
				 strerror(errno));
			break;
		} else if (ret != sizeof(struct wrs_hist_run_nand)) {
			pr_error("Unable to read all data from the file %s\n",
				 HIST_RUN_NAND_FILENAME);
			break;
		}
		if (data_run_nand.magic != (WRS_HIST_RUN_NAND_MAGIC | WRS_HIST_RUN_NAND_MAGIC_VER)) {
			pr_error("Wrong magic number in the file %s, is 0x%x, "
				 "expected 0x%x\n",
				 HIST_RUN_NAND_FILENAME, data_run_nand.magic,
				 WRS_HIST_RUN_NAND_MAGIC | WRS_HIST_RUN_NAND_MAGIC_VER);
			continue;
		}

		lifetime = data_run_nand.lifetime;
		/* update temp histogram */
	}
	close(fd);
	return lifetime;
}

static void hist_uptime_nand_update(void)
{
	struct wrs_hist_run_nand *data_run_nand;

	/* lock shmem */
	wrs_shm_write(hist_shmem_hdr, WRS_SHM_WRITE_BEGIN);

	data_run_nand = &hist_shmem->hist_run_nand;
	data_run_nand->magic =
			WRS_HIST_RUN_NAND_MAGIC | WRS_HIST_RUN_NAND_MAGIC_VER;
	hist_uptime_get_temp(data_run_nand->temp);
	data_run_nand->lifetime = hist_uptime_lifetime_get();
	data_run_nand->timestamp = time(NULL);

	/* unlock shmem */
	wrs_shm_write(hist_shmem_hdr, WRS_SHM_WRITE_END);
}

void hist_uptime_nand_save(void)
{
	hist_uptime_nand_update();
	pr_debug("Saving lifetime data to the nand\n");
	hist_uptime_nand_write(&hist_shmem->hist_run_nand);
}

static void hist_uptime_nand_write(struct wrs_hist_run_nand *data)
{
	int fd;
	int ret;
	/* use O_NOATIME to avoid update of last access time */
	/* O_SYNC to reduce caching problem */
	fd = open(HIST_RUN_NAND_FILENAME,
		  O_WRONLY| O_APPEND | O_CREAT | O_NOATIME | O_SYNC, 0644);
	if (fd < 0) {
		pr_error("Unable to write to the file %s\n",
			 HIST_RUN_NAND_FILENAME);
		exit(1);
	}
	ret = write(fd, data, sizeof(struct wrs_hist_run_nand));
	if (ret < 0) {
		pr_error("Write error to the file %s, ret %d, error(%d) %s:\n",
			 HIST_RUN_NAND_FILENAME, ret, errno, strerror(errno));
	} else if (ret != sizeof(struct wrs_hist_run_nand)) {
		pr_error("Unable to write all data to the file %s\n",
			 HIST_RUN_NAND_FILENAME);
	}
	fsync(fd);
	close(fd);
}

static void hist_uptime_get_temp(uint8_t temp[4])
{
	temp[0] += 1;
	temp[1] += 2;
	temp[2] += 3;
	temp[3] += 4;
}

void hist_uptime_spi_save(void)
{
}
