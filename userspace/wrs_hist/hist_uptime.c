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
#include <libwr/hal_shmem.h>
#include <libwr/wrs-msg.h>

#include "wrs_hist.h"

#define HIST_RUN_NAND_FILENAME "/update/lifetime_stats.bin"

static time_t init_time_monotonic;
static time_t uptime_stored;
/* Array translating temperature's value into the index of histogram's array */
static uint8_t temp_descr_tab[256];

static void hist_uptime_nand_read(time_t *uptime_stored,
	uint16_t temp_hist[WRS_HIST_TEMP_SENSORS_N][WRS_HIST_TEMP_ENTRIES]);
static void hist_uptime_nand_write(struct wrs_hist_run_nand *data);
static void hist_uptime_get_temp(int8_t temp[WRS_HIST_TEMP_SENSORS_N]);
static void hist_uptime_nand_update(void);

int hist_uptime_init(void)
{
	int i;
	int j;

	/* Fill array translating temperature's value into the index of
	 * histogram's array */
	for (i = 0; i < WRS_HIST_TEMP_ENTRIES; i++) {
		pr_debug("i = %d, from %d to %d %s\n", i, h_descr[i].from,
			 h_descr[i].to, h_descr[i].desc);
		/* add a 127 bias to the temperature's value */
		for (j = h_descr[i].from + 127; j <= h_descr[i].to + 127; j++) {
			pr_debug("i = %d j = %d\n", i, j);
			temp_descr_tab[j] = i;
		}
	}
	init_time_monotonic = get_monotonic_sec();
	hist_uptime_nand_read(&uptime_stored, hist_shmem->temp);
	hist_uptime_nand_update();
	return 0;
}

time_t hist_uptime_lifetime_get(void)
{
	time_t uptime;
	uptime = get_monotonic_sec() - init_time_monotonic;
	return uptime_stored + uptime;
}


static void update_temp_histogram(
	uint16_t temp_hist[WRS_HIST_TEMP_SENSORS_N][WRS_HIST_TEMP_ENTRIES],
	int8_t temp[WRS_HIST_TEMP_SENSORS_N])
{
	int i;
	int tt;

	for (i = 0; i < WRS_HIST_TEMP_SENSORS_N; i++) {
		tt = temp[i] + 127;
		temp_hist[i][temp_descr_tab[tt]]++;
	}
}

static void hist_uptime_nand_read(time_t *uptime_stored,
	uint16_t temp_hist[WRS_HIST_TEMP_SENSORS_N][WRS_HIST_TEMP_ENTRIES])
{
	int fd;
	struct wrs_hist_run_nand data_run_nand;
	int ret;
	uint32_t lifetime = 0;
	int lines_read = 0;
	uint32_t magic;

	/* use O_NOATIME to avoid update of last access time */
	fd = open(HIST_RUN_NAND_FILENAME, O_RDONLY | O_NOATIME);
	if (fd < 0) {
		pr_error("Unable to read the file %s\n",
			 HIST_RUN_NAND_FILENAME);
		return;
	}
	/* clear histogram stored in the shmem in case there was a restart of
	 * wrs_hist */
	memset(temp_hist, 0, sizeof(uint16_t) * WRS_HIST_TEMP_SENSORS_N *
			     WRS_HIST_TEMP_ENTRIES);
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
		magic = WRS_HIST_RUN_NAND_MAGIC | WRS_HIST_RUN_NAND_MAGIC_VER;
		if (data_run_nand.magic != magic) {
			pr_error("Wrong magic number in the file %s, is 0x%x, "
				 "expected 0x%x\n",
				 HIST_RUN_NAND_FILENAME, data_run_nand.magic,
				 magic);
			continue;
		}

		lifetime = data_run_nand.lifetime;
		/* update temp histogram */
		update_temp_histogram(temp_hist, data_run_nand.temp);
		lines_read++;
	}
	close(fd);
	*uptime_stored = lifetime;
	pr_debug("read %d lines from the flash\n", lines_read);
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
	update_temp_histogram(hist_shmem->temp, data_run_nand->temp);

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

static void hist_uptime_get_temp(int8_t temp[WRS_HIST_TEMP_SENSORS_N])
{
	static struct hal_temp_sensors temp_sensors;

	/* TODO: this should read or calculare average temperature not
	 * the current temperature */
	hal_shmem_read_temp(&temp_sensors);
	temp[WRS_HIST_TEMP_FPGA] = temp_sensors.fpga >> 8;
	temp[WRS_HIST_TEMP_PLL] = temp_sensors.pll >> 8;
	temp[WRS_HIST_TEMP_PSL] = temp_sensors.psl >> 8;
	temp[WRS_HIST_TEMP_PSR] = temp_sensors.psr >> 8;
}

void hist_uptime_spi_save(void)
{
}
