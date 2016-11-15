#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#define __USE_GNU /* for O_NOATIME flag */
#include <fcntl.h>

#include <libwr/hist_shmem.h>
#include <libwr/wrs-msg.h>

#include "wrs_hist.h"
#include "hist_crc.h"

#define HIST_RUN_SPI_P "wrs_hist1"
#define HIST_RUN_SPI_B "wrs_hist2"

#define MTD_PREFIX "/dev/"

//#define MTD_INFO_FILE "mtd"
#define MTD_INFO_FILE "/proc/mtd"

#define READ_NOT_OK 0
#define READ_OK 1
#define READ_EMPTY_ENTRY 2

/* struct to be used to check whether record in the SPI flash is empty */
struct wrs_hist_up_spi empty_spi_entry;

char mtd_p[20];
char mtd_b[20];

int fd_p;
int fd_b;


static int find_partition(char *mtd_name, char *par_name)
{
	FILE *f;
	char mtd_str[10];
	char mtd_desc[50];
	char mtd_size[10];
	char mtd_erasesize[10];

	char par_name_q[52]; /* par_name with quotas */
	/*unsigned int mtd_size;
	unsigned int mtd_erasesize;*/
	int ret;
	
	f = fopen(MTD_INFO_FILE, "r");
	if (!f) {
		pr_error("Unable to read the file %s, unable to find where are"
			 " hist_up_spi partitions!\n",
			 MTD_INFO_FILE);
		return -1;
	}

	/* add quotas to the partition's name */
	sprintf(par_name_q, "\"%.*s\"", 52, par_name);
	while (1) {
		while (1) {
			/* read up to 10 chars without ":", then read ":",
			 * then up to 10 chars, again up to 10 chars,
			 * up to 50 chars (skip "s" in the last matching to
			 * support also white spaces int he partitions' name)
			 */
			ret = fscanf(f, "%10[^:]: %10s %10s %50[^\n]%*[\n]",
				     mtd_str, mtd_size, mtd_erasesize,
				     mtd_desc);
/*			pr_debug("ret %d\n", ret);
					pr_debug("%s, %s, %s, %s\n", mtd_str, mtd_size, mtd_erasesize,
		       mtd_desc);*/
			if (ret == 4)
				break;
			if (ret <= 0) {
				/* not found return from the function */
				fclose(f);
				return -1;
			}
		}
/*		pr_debug("%s, %s, %s, %s\n", mtd_str, mtd_size, mtd_erasesize,
		       mtd_desc);*/

		if (!strcmp(mtd_desc, par_name_q)){
			/* partition found! */
			snprintf(mtd_name, 20, MTD_PREFIX "%s", mtd_str);
			fclose(f);
			return 0;
		}
	}
	/* should be never reached */
	return -1;
}

static int open_partition(char *mtd_part, char *mtd_part_name)
{
	int ret;
	int fd = 0;

	ret = find_partition(mtd_part, mtd_part_name);
	if (ret < 0) {
		pr_debug("Parition %s not found!\n", mtd_part_name);
	} else {
		pr_debug("parition %s is %s\n", mtd_part_name, mtd_part);
		fd = open(mtd_part, O_RDWR | O_NOATIME | O_SYNC);
		if (fd < 0) {
			pr_error("Unable to read partition %s\n", mtd_part);
		}
	}

	return fd;
}


static int verify_read_data(int fd, struct wrs_hist_up_spi *valid_data, char *mtd_part_name, int *index)
{
	struct wrs_hist_up_spi data_local;
	int ret;
	uint8_t crc_old;
	uint8_t crc_calc;

	ret = read(fd, &data_local, sizeof(data_local));
	if (ret == 0) {
		/* End of file, no more space is SPI flash */
		pr_error("No more space in the flash partition %s\n",
			mtd_part_name);
		return READ_NOT_OK;
	} else if (ret < 0) {
		pr_error("Read error from the flash partition %s, "
			"ret %d, error(%d) %s:\n",
			mtd_part_name, ret, errno,
			strerror(errno));
		return READ_NOT_OK;
	} else if (ret != sizeof(data_local)) {
		pr_error("Unable to read all data from the flash "
			"partition %s\n",
			mtd_part_name);
		return READ_NOT_OK;
	}

	if (!memcmp(&data_local, &empty_spi_entry,
		    sizeof(empty_spi_entry))) {
		pr_debug("Found empty record for partition %s "
			"index %d\n",
			mtd_part_name, *index);
		/* rewind the file, by what was just read */
		lseek(fd, -sizeof(data_local), SEEK_CUR);
		return READ_EMPTY_ENTRY;
	}
	/* check the magic! */
	if (data_local.magic != WRS_HIST_UP_SPI_MAGIC) {
		pr_error("Wrong magic read in the entry %d from the partition "
			 "%s. Expected magic 0x%04x, read 0x%04x\n",
			 *index, mtd_part_name, WRS_HIST_UP_SPI_MAGIC,
			 data_local.magic);
		return READ_NOT_OK;
	}
	/* check the version */
	if (data_local.ver != WRS_HIST_UP_SPI_MAGIC_VER) {
		pr_error("Wrong version read in the entry %d from the "
			 "partition %s. Expected version 0x%02x, read 0x%02x\n",
			 *index, mtd_part_name, WRS_HIST_UP_SPI_MAGIC_VER,
			 data_local.ver);
		return READ_NOT_OK;
	}
		
	/* check the crc */
	crc_old = data_local.crc;
	data_local.crc = 0;
	crc_calc = crc_fast((uint8_t *)&data_local,
			    sizeof(data_local));
	if (crc_old != crc_calc) {
		/* crc mismatch */
		pr_error("CRC does not match for entry %d in flash partition %s"
			  " of spi flash. CRC "
			  "expected 0x%02x, calculated 0x%02x\n",
			  *index, mtd_part_name, crc_calc, crc_old);
		return READ_NOT_OK;
	}
	/* If the data is valid update "valid_data" */
	if (valid_data) {
		memcpy(valid_data, &data_local,
			sizeof(data_local));
	}
	return READ_OK;
}

int hist_up_spi_init(void)
{
	struct wrs_hist_up_spi data_p;
	struct wrs_hist_up_spi data_b;
	int index_p = 0, index_b = 0;
	int read_ok_p = READ_NOT_OK, read_ok_b = READ_NOT_OK;

	/* initialize empty entry with 0xff since SPI is a flash */
	memset(&empty_spi_entry, 0xff, sizeof(empty_spi_entry));
	/* clear data_p and data_b */
	memset(&data_p, 0, sizeof(data_p));
	memset(&data_b, 0, sizeof(data_b));

	/* find a proper mtd partition */
	fd_p = open_partition(mtd_p, HIST_RUN_SPI_P);
	fd_b = open_partition(mtd_b, HIST_RUN_SPI_B);
	
	if (fd_p < 0 && fd_b < 0) {
		/* no files opened successfully, nothing can be done */
		return 0;
	}

	while(1) {
		if (fd_p >= 0) {
			read_ok_p = verify_read_data(fd_p, &data_p,
						     HIST_RUN_SPI_P, &index_p);
			index_p++;
		}
		if (fd_b >= 0) {
			read_ok_b = verify_read_data(fd_b, &data_b,
						     HIST_RUN_SPI_B, &index_b);
			index_b++;
		}
		if (read_ok_p == READ_EMPTY_ENTRY
		    || read_ok_b == READ_EMPTY_ENTRY) {
			/* at least one empty entry found */
			break;
		}
	}

	/* If magic is ok, then we know that the data was read successful */
	/* If an entry with an index "i" in P is empty, and entry with the same
	 * index "i" is valid, then there will be a check with the i-1 entry
	 * of B. The newer valid will be used.
	 */
	if (data_p.magic == WRS_HIST_UP_SPI_MAGIC
	    && data_b.magic == WRS_HIST_UP_SPI_MAGIC) {
		/* data from both were read */
		if (data_p.lifetime >= data_b.lifetime)
			hist_shmem->hist_up_spi = data_p;
		else
			hist_shmem->hist_up_spi = data_b;
	} else if (data_p.magic == WRS_HIST_UP_SPI_MAGIC) {
		hist_shmem->hist_up_spi = data_p;
	} else if (data_b.magic == WRS_HIST_UP_SPI_MAGIC) {
		hist_shmem->hist_up_spi = data_b;
	}

	/* copy histogram read from the SPI to the live histogram */
	memcpy(hist_shmem->temp, hist_shmem->hist_up_spi.temp,
	       sizeof(hist_shmem->hist_up_spi.temp));
	return 0;
}

// struct wrs_hist_up_spi {
// 	uint16_t magic; /* 16bits magic */
// 	uint8_t ver; /* 8bits version */
// 	uint8_t crc; /* 8bits crc */
// 	uint32_t lifetime; /* in seconds, ~136 years */
// 	uint32_t timestamp; /* in seconds, ~136 years */
// 	/* histogram, tens of hours of particular temperature 512 bytes */
// 	uint16_t temp[WRS_HIST_TEMP_SENSORS_N][WRS_HIST_TEMP_ENTRIES];
// };

static void write_data(int fd, struct wrs_hist_up_spi *data, char *mtd_part_name)
{
	int ret;
	uint8_t crc_old;
	uint8_t crc_calc;
	struct wrs_hist_up_spi read_data;

	if (!fd) {
		/* File not opened */
		return;
	}
	
	ret = write(fd, data, sizeof(*data));
	if (ret < 0) {
		pr_error("Write error to the flash partition %s, "
			"ret %d, error(%d) %s:\n",
			mtd_part_name, ret, errno,
			strerror(errno));
		return;
	} else if (ret != sizeof(*data)) {
		pr_error("Written %d bytes, expected to be written %d to the flash "
			"partition %s\n",
			ret, sizeof(*data), mtd_part_name);
		return;
	}

	pr_debug("Write ok %s\n", mtd_part_name);

	/* verify written data */
	ret = lseek(fd, -sizeof(*data), SEEK_CUR);
	if (ret < 0) {
		pr_error("Unable to rewind the partition %s data to verify the"
			 " last write\n", mtd_part_name);
		return;
	}
	ret = read(fd, &read_data, sizeof(read_data));
	if (ret != sizeof(*data)) {
		pr_error("Unable to read data from partition %s to verify the "
			 "last write, read %d bytes, expected %d\n",
			 mtd_part_name, ret, sizeof(*data));
		return;
	}
	crc_old = data->crc;
	read_data.crc = 0;
	crc_calc = crc_fast((uint8_t *)&read_data, sizeof(read_data));
	if (crc_old != crc_calc) {
		/* crc mismatch */
		pr_error("CRC does not match for entry in flash partition %s"
			  " of spi flash. CRC "
			  "expected 0x%02x, calculated 0x%02x\n",
			  mtd_part_name, crc_calc, crc_old);
	}
}


void hist_up_spi_save(void)
{
	hist_shmem->hist_up_spi.magic = WRS_HIST_UP_SPI_MAGIC;
	hist_shmem->hist_up_spi.ver = WRS_HIST_UP_SPI_MAGIC_VER;
	hist_shmem->hist_up_spi.lifetime = hist_up_lifetime_get();
	hist_shmem->hist_up_spi.timestamp = time(NULL);

	memcpy(hist_shmem->hist_up_spi.temp, hist_shmem->temp,
	       sizeof(hist_shmem->hist_up_spi.temp));
	hist_shmem->hist_up_spi.crc = 0;
	hist_shmem->hist_up_spi.crc =
				crc_fast((uint8_t *)&hist_shmem->hist_up_spi,
					 sizeof(hist_shmem->hist_up_spi));

	/* save data */
	write_data(fd_p, &hist_shmem->hist_up_spi, HIST_RUN_SPI_P);
	write_data(fd_b, &hist_shmem->hist_up_spi, HIST_RUN_SPI_B);
}

/* on exit close file descriptors */
void hist_up_spi_exit(void)
{
	if (fd_p)
		close(fd_p);
	if (fd_b)
		close(fd_b);
}
