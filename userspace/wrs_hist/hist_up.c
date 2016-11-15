#include <stdio.h>
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
#include "hist_crc.h"

#define HIST_UP_NAND_FILENAME "/update/lifetime_stats.bin"

static time_t init_time_monotonic;
static time_t uptime_stored;
/* Array translating temperature's value into the index of histogram's array */
static uint8_t temp_descr_tab[256];

static int nand_read(time_t *uptime_stored,
	uint16_t temp_hist[WRS_HIST_TEMP_SENSORS_N][WRS_HIST_TEMP_ENTRIES]);
static void nand_write(struct wrs_hist_up_nand *data);
static void get_temp(int8_t temp[WRS_HIST_TEMP_SENSORS_N]);
static void nand_update(void);

int hist_up_nand_init(void)
{
	int i;
	int j;

	/* Fill array translating temperature's value into the index of
	 * histogram's array */
	for (i = 0; i < WRS_HIST_TEMP_ENTRIES; i++) {
		/* add a 127 bias to the temperature's value */
		for (j = h_descr[i].from + 127; j <= h_descr[i].to + 127; j++) {
			temp_descr_tab[j] = i;
		}
	}
	init_time_monotonic = get_monotonic_sec();
	nand_read(&uptime_stored, hist_shmem->temp);

	return 0;
}

time_t hist_up_lifetime_get(void)
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

static int nand_read(time_t *uptime_stored,
	uint16_t temp_hist[WRS_HIST_TEMP_SENSORS_N][WRS_HIST_TEMP_ENTRIES])
{
	int fd;
	struct wrs_hist_up_nand data_up_nand;
	int ret;
	uint32_t lifetime = 0;
	int32_t entries_ok = 0; /* number of valid entries read so far */
	int32_t entries_index = 0; /* number of entries read so far */
	uint8_t crc_saved;
	uint8_t crc_calc;

	/* use O_NOATIME to avoid update of last access time */
	fd = open(HIST_UP_NAND_FILENAME, O_RDONLY | O_NOATIME);
	if (fd < 0) {
		pr_error("Unable to read the file %s\n",
			 HIST_UP_NAND_FILENAME);
		return -1;
	}

	while (1) {
		ret = read(fd, &data_up_nand, sizeof(data_up_nand));
		if (ret == 0) {
			/* End of file */
			break;
		} else if (ret < 0) {
			pr_error("Read error from the file %s, ret %d, "
				 "error(%d) %s:\n",
				 HIST_UP_NAND_FILENAME, ret, errno,
				 strerror(errno));
			break;
		} else if (ret != sizeof(struct wrs_hist_up_nand)) {
			pr_error("Unable to read all data from the file %s\n",
				 HIST_UP_NAND_FILENAME);
			break;
		}
		entries_index++;
		if (data_up_nand.magic != WRS_HIST_UP_NAND_MAGIC) {
			pr_error("Wrong magic in saved uptime entry (index %d)"
				 " in the nand (file %s)! is 0x%04x, "
				 "expected 0x%04x\n",
				 entries_index, HIST_UP_NAND_FILENAME,
				 data_up_nand.magic, WRS_HIST_UP_NAND_MAGIC);
			continue;
		}
		if (data_up_nand.ver != WRS_HIST_UP_NAND_MAGIC_VER) {
			pr_error("Wrong version number in saved uptime entry "
				 "(index %d) in the nand (file %s)! is 0x%04x,"
				 " expected 0x%04x\n",
				 entries_index, HIST_UP_NAND_FILENAME,
				 data_up_nand.ver,
				 WRS_HIST_UP_NAND_MAGIC_VER);
			continue;
		}
		/* save old, clear old, compute new and compare CRCs */
		crc_saved = data_up_nand.crc;
		data_up_nand.crc = 0;
		crc_calc = crc_fast((uint8_t *)&data_up_nand,
				    sizeof(struct wrs_hist_up_nand));
		if (crc_saved != crc_calc) {
			/* avoid to many printouts about non-valid entries */
			if (entries_index - entries_ok > 30) {
				/* but if debug is enabled print them all */
				pr_debug("Wrong CRC in saved uptime entry "
					 "(index %d) in the nand (file %s)! "
					 "Expected crc 0x%02x, calculated "
					 "0x%02x\n", entries_index,
					 HIST_UP_NAND_FILENAME, crc_saved,
					 crc_calc);
				continue;
			}
			pr_error("Wrong CRC in saved uptime entry (index %d) "
				 "in the nand (file %s)! Expected crc 0x%02x, "
				 "calculated 0x%02x\n", entries_index,
				 HIST_UP_NAND_FILENAME, crc_saved, crc_calc);
			continue;
		}

		lifetime = data_up_nand.lifetime;
		if (hist_shmem->hist_up_spi.lifetime < lifetime) {
			/* Update temp histogram, with only newer entries than
			 * the last histogram dump in the SPI */
			update_temp_histogram(temp_hist, data_up_nand.temp);
		}
		entries_ok++;
	}

	close(fd);
	*uptime_stored = lifetime;
	pr_debug("Read %d lifetime entries from the flash\n", entries_ok);

	if (entries_ok != entries_index)
		pr_error("Read %d non-valid lifetime entries from the flash\n",
			 entries_index - entries_ok);
	return 0;
}

static void nand_update(void)
{
	struct wrs_hist_up_nand *data_up_nand;

	/* lock shmem */
	wrs_shm_write(hist_shmem_hdr, WRS_SHM_WRITE_BEGIN);

	data_up_nand = &hist_shmem->hist_up_nand;
	data_up_nand->magic = WRS_HIST_UP_NAND_MAGIC;
	data_up_nand->ver = WRS_HIST_UP_NAND_MAGIC_VER;
	get_temp(data_up_nand->temp);
	data_up_nand->lifetime = hist_up_lifetime_get();
	data_up_nand->timestamp = time(NULL);
	update_temp_histogram(hist_shmem->temp, data_up_nand->temp);

	/* unlock shmem */
	wrs_shm_write(hist_shmem_hdr, WRS_SHM_WRITE_END);
}

void hist_up_nand_save(void)
{
	nand_update();
	pr_debug("Saving lifetime data to the nand\n");
	nand_write(&hist_shmem->hist_up_nand);
}

static void nand_write(struct wrs_hist_up_nand *data)
{
	int fd;
	int ret;
	uint8_t crc_calc;

	/* use O_NOATIME to avoid update of last access time */
	/* O_SYNC to reduce caching problem */
	fd = open(HIST_UP_NAND_FILENAME,
		  O_WRONLY| O_APPEND | O_CREAT | O_NOATIME | O_SYNC, 0644);
	if (fd < 0) {
		pr_error("Unable to write to the file %s\n",
			 HIST_UP_NAND_FILENAME);
		exit(1);
	}

	/* clear old, compute and write CRC */
	data->crc = 0;
	crc_calc = crc_fast((uint8_t *)data, sizeof(struct wrs_hist_up_nand));
	data->crc = crc_calc;

	ret = write(fd, data, sizeof(struct wrs_hist_up_nand));
	if (ret < 0) {
		pr_error("Write error to the file %s, ret %d, error(%d) %s:\n",
			 HIST_UP_NAND_FILENAME, ret, errno, strerror(errno));
	} else if (ret != sizeof(struct wrs_hist_up_nand)) {
		pr_error("Unable to write all data to the file %s\n",
			 HIST_UP_NAND_FILENAME);
	}
	fsync(fd);
	close(fd);
}

static void get_temp(int8_t temp[WRS_HIST_TEMP_SENSORS_N])
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

/* Function to be called at the exit of wrs_hist */
void hist_up_nand_exit(void)
{
	/* nothing to do there */
}
