#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include <sys/stat.h>
#define __USE_GNU /* for O_NOATIME flag */
#include <fcntl.h>

#include <libwr/wrs-msg.h>
#include <libwr/util.h>
#include <libwr/shmem.h>
#include <libwr/hist_shmem.h>
#include "wrs_hist.h"
#include "hist_crc.h"

#define SFP_STR_LEN 16
#define SFP_INSERT_NEW_ENTRY 1

#define VERIFY_OK		0
#define VERIFY_WRONG_MAGIC	1
#define VERIFY_WRONG_MAGIC_SEC	2
#define VERIFY_WRONG_CRC	3
#define VERIFY_WRONG_CRC_SEC	4
#define VERIFY_WRONG_ENTRY_CRC	5

char *hist_sfp_nand_filename = "/update/lifetime_sfp_stats.bin";
char *hist_sfp_nand_filename_backup = "/update/lifetime_sfp_stats_b.bin";

static int hist_sfp_nand_read(struct wrs_hist_sfp_nand *sfp_data, char *file);
static int hist_sfp_nand_read_verify(struct wrs_hist_sfp_nand *sfp_data,
				     char *file);
static void hist_sfp_nand_copy(struct wrs_hist_sfp_nand *to,
			       struct wrs_hist_sfp_nand *from);
static struct wrs_hist_sfp_nand *hist_sfp_nand_data_verify(
					     struct wrs_hist_sfp_nand *data_a,
					     struct wrs_hist_sfp_nand *data_b);
static int hist_sfp_nand_validate_sfp_entries(struct wrs_hist_sfp_nand *data,
					      char *file);
static void hist_sfp_nand_merge_sfp_db(struct wrs_hist_sfp_nand *data_a,
				       struct wrs_hist_sfp_nand *data_b);
#define VALID 1
#define NOT_VALID 0

int hist_sfp_init(void)
{
	struct wrs_hist_sfp_nand data_a; /* sfp data read from the file */
	struct wrs_hist_sfp_nand data_b; /* sfp data read from the backup
					   * file */
	struct wrs_hist_sfp_nand *ret_data;

	ret_data = hist_sfp_nand_data_verify(&data_a, &data_b);
	wrs_shm_write(hist_shmem_hdr, WRS_SHM_WRITE_BEGIN);
	if (ret_data) {
		/* copy data from malloced memory */
		hist_sfp_nand_copy(&hist_shmem->hist_sfp_nand, ret_data);
	}
	/* Whatever is the state of DB recreate magic etc. No need to
	  * calculate crc at this moment. */
	hist_shmem->hist_sfp_nand.magic = WRS_HIST_SFP_MAGIC;
	hist_shmem->hist_sfp_nand.ver = WRS_HIST_SFP_MAGIC_VER;
	hist_shmem->hist_sfp_nand.saved_swlifetime =
					hist_uptime_lifetime_get();
	hist_shmem->hist_sfp_nand.saved_timestamp = time(NULL);
	hist_shmem->hist_sfp_nand.end_magic = WRS_HIST_SFP_MAGIC;
	hist_shmem->hist_sfp_nand.end_ver = WRS_HIST_SFP_MAGIC_VER;

	wrs_shm_write(hist_shmem_hdr, WRS_SHM_WRITE_END);

	return 0;
}

static void hist_sfp_nand_copy(struct wrs_hist_sfp_nand *to,
			       struct wrs_hist_sfp_nand *from)
{
	*to = *from;
}

static struct wrs_hist_sfp_nand *hist_sfp_nand_data_verify(
					      struct wrs_hist_sfp_nand *data_a,
					      struct wrs_hist_sfp_nand *data_b)
{
	int ret_a;
	int ret_b;
	int valid_a = VALID;
	int valid_b = VALID;
	int n_wrong_sfp_a = 0;
	int n_wrong_sfp_b = 0;

	/*
	 * 1. Read both files
	 * 2. If both main CRC's are correct, take the one which is newer
	 *    (based on the lifetime stamp)
	 * 3. If CRC of one is wrong take all data from the second
	 * 4. If both main CRC's are not valid, take entries with the bigger
	 *    lifetime (TODO: take into the account that for some reason
	 *    entries may be in the different order)
	 */

	ret_a = hist_sfp_nand_read(data_a, hist_sfp_nand_filename);
	if (ret_a < 0)
		valid_a = NOT_VALID;

	ret_b = hist_sfp_nand_read(data_b, hist_sfp_nand_filename_backup);
	if (ret_b < 0)
		valid_b = NOT_VALID;

	if (valid_a == NOT_VALID && valid_b == NOT_VALID) {
		/* none is valid, return null */
		pr_debug("None of files were read successfully\n");
		return NULL;
	}

	if (valid_a == VALID && valid_b == NOT_VALID) {
		pr_debug("failed to read %s, using %s\n",
			 hist_sfp_nand_filename_backup,
			 hist_sfp_nand_filename);
		hist_sfp_nand_read_verify(data_a, hist_sfp_nand_filename);

		/* check SFP entries, clear not valid */
		hist_sfp_nand_validate_sfp_entries(data_a,
						      hist_sfp_nand_filename);

		/* return whatever we managed to read */
		return data_a;
	}

	if (valid_a == NOT_VALID && valid_b == VALID) {
		pr_debug("failed to read %s, using %s\n",
			 hist_sfp_nand_filename,
			 hist_sfp_nand_filename_backup);
		hist_sfp_nand_read_verify(data_b,
						hist_sfp_nand_filename_backup);

		/* check SFP entries, clear not valid */
		hist_sfp_nand_validate_sfp_entries(data_b,
						hist_sfp_nand_filename_backup);

		/* return whatever we managed to read */
		return data_b;
	}

	/* At this point we know that both files were read successfully */

	ret_a = hist_sfp_nand_read_verify(data_a, hist_sfp_nand_filename);
	ret_b = hist_sfp_nand_read_verify(data_b,
					  hist_sfp_nand_filename_backup);

	n_wrong_sfp_a = hist_sfp_nand_validate_sfp_entries(data_a,
						hist_sfp_nand_filename);
	n_wrong_sfp_b = hist_sfp_nand_validate_sfp_entries(data_b,
						hist_sfp_nand_filename_backup);

	if (ret_a == VERIFY_OK && n_wrong_sfp_a == 0
	    && ret_b == VERIFY_OK && n_wrong_sfp_b == 0) {
		/* both are ok, take latest, or data_a when the same age */
		if (data_a->saved_swlifetime >= data_b->saved_swlifetime) {
			pr_debug("Using %s\n", hist_sfp_nand_filename);
			return data_a;
		} else {
			pr_debug("Using %s\n", hist_sfp_nand_filename_backup);
			return data_b;
		}
	}

	/* now only one is ok */
	if (ret_a == VERIFY_OK && n_wrong_sfp_a == 0) {
		pr_debug("Using %s\n", hist_sfp_nand_filename);
		return data_a;
	}

	if (ret_b == VERIFY_OK && n_wrong_sfp_b == 0) {
		pr_debug("Using %s\n", hist_sfp_nand_filename_backup);
		return data_b;
	}

	/* Now none of files are ok, try to recover as much SFPs as possible */
	/* at this point all erroneous SFP's entries were cleared */
	/* Merge SFP databases into the data_a */
	hist_sfp_nand_merge_sfp_db(data_a, data_b);
	pr_debug("Using merge of %s and %s\n", 
		 hist_sfp_nand_filename, hist_sfp_nand_filename_backup);
	return data_a;
}

static void hist_sfp_nand_merge_sfp_db(struct wrs_hist_sfp_nand *data_a,
				       struct wrs_hist_sfp_nand *data_b)
{
	int i;
	struct wrs_hist_sfp_entry *s_a = data_a->sfps;
	struct wrs_hist_sfp_entry *s_b = data_b->sfps;

	/* Assume that all entries have valid CRC, these with wrong CRC's were
	 * cleared before */
	/* For simplicity compare entries only with the same indexes */
	for (i = 0; i < WRS_HIST_MAX_SFPS; i++) {
		if (s_b[i].vn[0] == '\0'
			&& s_b[i].pn[0] == '\0'
			&& s_b[i].sn[0] == '\0'
		   ) {
			/* If entry in data_b is empty go to next entry;
			 * If entry in data_a is not empty we will keep it
			 * anyway */
			continue;
		}
		if (s_a[i].vn[0] == '\0'
			&& s_a[i].pn[0] == '\0'
			&& s_a[i].sn[0] == '\0'
		   ) {
			/* Entry in data_a is empty, so copy entry from data_b
			 * to the data_a */
			s_a[i] = s_b[i];
			continue;
		}
		/* There are entries at this index in the data_a and the data_b
		 * compare last seen */
		if (s_b[i].lastseen_swlifetime > s_a[i].lastseen_swlifetime) {
			/* entry in the data_b is newer, copy it */
			s_a[i] = s_b[i];
			continue;
		}
	}
}

static int hist_sfp_nand_validate_sfp_entries(struct wrs_hist_sfp_nand *data,
					      char *file)
{
	int i;
	uint8_t crc_calc;
	uint8_t crc_old;
	int entries_nok = 0;
	int invalid_entry = 0;
	struct wrs_hist_sfp_entry sfp_entry;

	sfp_entry = data->sfps[0];
	for (i = 0; i < WRS_HIST_MAX_SFPS; i++) {
		/* skip empty entries */
		if (data->sfps[i].vn[0] == '\0'
			&& data->sfps[i].pn[0] == '\0'
			&& data->sfps[i].sn[0] == '\0'
		   ) {
			/* empty entry skip it */
			continue;
		}
		invalid_entry = 0;

		/* copy sfp data, calculation of CRC requires CRC to be 0 */
		sfp_entry = data->sfps[i];
		if (sfp_entry.mag != WRS_HIST_SFP_EMAGIC) {
			pr_error("Wrong magic for SFP entry %d in file %s "
				 "(vn:%.*s, pn:%.*s, sn:%.*s). "
				 "read 0x%02x, expected 0x%02x\n",
				 i, file,  SFP_STR_LEN, sfp_entry.vn,
				 SFP_STR_LEN, sfp_entry.pn, SFP_STR_LEN,
				 sfp_entry.sn, sfp_entry.mag,
				 WRS_HIST_SFP_EMAGIC);
			if (!invalid_entry)
				entries_nok++;
			invalid_entry = 1;
		}
		if (sfp_entry.ver != WRS_HIST_SFP_EMAGIC_VER) {
			pr_error("Wrong version for SFP entry %d in file %s "
				 "(vn:%.*s, pn:%.*s, sn:%.*s). "
				 "read 0x%02x, expected 0x%02x\n",
				 i, file,  SFP_STR_LEN, sfp_entry.vn,
				 SFP_STR_LEN, sfp_entry.pn, SFP_STR_LEN,
				 sfp_entry.sn, sfp_entry.ver,
				 WRS_HIST_SFP_EMAGIC_VER);
			if (!invalid_entry)
				entries_nok++;
			invalid_entry = 1;
		}
		crc_old = sfp_entry.crc;
		sfp_entry.crc = 0;
		crc_calc = crc_fast((uint8_t *)&sfp_entry,
				    sizeof(struct wrs_hist_sfp_entry));
		if (crc_calc != crc_old) {
			pr_error("Wrong CRC for SFP entry %d in file %s "
				 "(vn:%.*s, pn:%.*s, sn:%.*s). "
				 "Calculated 0x%02x, expected 0x%02x\n",
				 i, file,  SFP_STR_LEN, sfp_entry.vn,
				 SFP_STR_LEN, sfp_entry.pn, SFP_STR_LEN,
				 sfp_entry.sn, crc_calc, crc_old);
			/* clear entries with wrong CRC, we cannot do more with
			 * them */
			memset(&data->sfps[i], 0,
			       sizeof(struct wrs_hist_sfp_entry));
			if (!invalid_entry)
				entries_nok++;
			continue;
		}
		if (invalid_entry) {
			/* fix magic and version */
			data->sfps[i].mag = WRS_HIST_SFP_EMAGIC;
			data->sfps[i].ver = WRS_HIST_SFP_EMAGIC_VER;
			/* Recalculate SFP */
			data->sfps[i].crc = 0;
			data->sfps[i].crc = crc_fast((uint8_t *)&data->sfps[i],
					    sizeof(struct wrs_hist_sfp_entry));
		}
	}
	if (entries_nok != 0)
		pr_error("Found %d not valid SFP entries in the file %s\n",
			  entries_nok, file);
	return entries_nok;
}

static struct wrs_hist_sfp_entry * hist_sfp_find(char *vn, char *pn, char *sn)
{
	int i;
	struct wrs_hist_sfp_nand *hist_sfp_nand_loc;
	hist_sfp_nand_loc = &hist_shmem->hist_sfp_nand;
	for (i = 0; i < WRS_HIST_MAX_SFPS; i++) {
		if (!hist_sfp_nand_loc->sfps[i].mag) {
			/* Skip entries never used */
			continue;
		}
		if (!strncmp(hist_sfp_nand_loc->sfps[i].vn, vn, SFP_STR_LEN)
		    && !strncmp(hist_sfp_nand_loc->sfps[i].pn, pn, SFP_STR_LEN)
		    && !strncmp(hist_sfp_nand_loc->sfps[i].sn, sn, SFP_STR_LEN)
		   ) {
			/* sfp found */
			pr_debug("Sfp vn:%.*s, pn:%.*s, sn: %.*s found in "
				 "index %d\n", SFP_STR_LEN, vn, SFP_STR_LEN,
				 pn, SFP_STR_LEN, sn, i);
			return &hist_sfp_nand_loc->sfps[i];
		}
	}
	return NULL;
}

static struct wrs_hist_sfp_entry * hist_sfp_find_empty(void)
{
	int i;

	for (i = 0; i < WRS_HIST_MAX_SFPS; i++) {
		if (!hist_shmem->hist_sfp_nand.sfps[i].mag) {
			/* Return an entry never used */
			return &hist_shmem->hist_sfp_nand.sfps[i];
		}
	}
	return NULL;
}

static struct wrs_hist_sfp_entry * hist_sfp_find_oldest(void)
{
	struct wrs_hist_sfp_entry *oldest_sfp;
	int i;

	/* start with first one as the oldest */
	oldest_sfp = &hist_shmem->hist_sfp_nand.sfps[0];

	/* search for the latest seen, not with the biggest lifetime */
	for (i = 1; i < WRS_HIST_MAX_SFPS; i++) {
		if (oldest_sfp->lastseen_timestamp >
			hist_shmem->hist_sfp_nand.sfps[i].lastseen_timestamp
		    ) {
			/* An older entry found */
			oldest_sfp = &hist_shmem->hist_sfp_nand.sfps[i];
		}
	}
	return oldest_sfp;
}

static int hist_sfp_get_entry(char *vn, char *pn, char *sn,
		    struct wrs_hist_sfp_entry **sfp_entry)
{
	struct wrs_hist_sfp_entry *sfp;
	int new_entry = 0;

	sfp = hist_sfp_find(vn, pn, sn);
	if (!sfp) {
		/* Entry not found, find empty */
		sfp = hist_sfp_find_empty();
		if (!sfp) {
			/* Empty not found, find the oldest to be replaced */
			sfp = hist_sfp_find_oldest();
		}
		if (sfp->vn[0] != '\0'
			|| sfp->pn[0] != '\0'
			|| sfp->sn[0] != '\0'
		   ) {
			pr_info("removing sfp vn:%.*s, pn:%.*s, sn: %.*s\n",
				SFP_STR_LEN, sfp->vn, SFP_STR_LEN, sfp->pn,
				SFP_STR_LEN, sfp->sn);
		}
		strncpy(sfp->vn, vn, SFP_STR_LEN);
		strncpy(sfp->pn, pn, SFP_STR_LEN);
		strncpy(sfp->sn, sn, SFP_STR_LEN);
		sfp->lastseen_swlifetime = 0;
		sfp->lastseen_timestamp = 0;
		sfp->sfp_lifetime = 0;
		new_entry = SFP_INSERT_NEW_ENTRY;
	}

	if (sfp_entry) {
		/* if sfp_entry present fill it */
		*sfp_entry = sfp;
	}
	return new_entry;
}

static void hist_sfp_calc_sfp_crc(struct wrs_hist_sfp_entry *sfp)
{
	/* clear crc */
	sfp->crc = 0;
	sfp->crc = crc_fast((uint8_t *) sfp, sizeof(*sfp));
}

void hist_sfp_insert(char *vn, char *pn, char *sn)
{
	struct wrs_hist_sfp_entry *sfp;

	hist_sfp_get_entry(vn, pn, sn, &sfp);
	sfp->mag = WRS_HIST_SFP_EMAGIC;
	sfp->ver = WRS_HIST_SFP_EMAGIC_VER;
	sfp->flags |= WRS_HIST_SFP_PRESENT;
	/* update last seen */
	sfp->lastseen_swlifetime = hist_uptime_lifetime_get();
	sfp->lastseen_timestamp = time(NULL);
	hist_sfp_calc_sfp_crc(sfp);

	/* Update all other SFPs and write new information to the nand.
	 * We don't want to loose the information that particullar SFP was
	 * inserted, so we want to update nand. Since we will update the nand
	 * anyway update the entire database. */
	hist_sfp_nand_save();

	return;
}

void hist_sfp_remove(char *vn, char *pn, char *sn)
{
	struct wrs_hist_sfp_entry *sfp;
	int new_entry;
	uint32_t lifetime_of_remove;
	
	/* avoid multipe calls of hist_uptime_lifetime_get() to have consistent
	 * time */
	lifetime_of_remove = hist_uptime_lifetime_get();

	new_entry = hist_sfp_get_entry(vn, pn, sn, &sfp);
	if (new_entry & SFP_INSERT_NEW_ENTRY) {
		pr_warning("New SFP entry added at SFP removal! It should "
			   "never happen! Sfp vn:%.*s, pn:%.*s, sn: %.*s\n",
			   SFP_STR_LEN, vn, SFP_STR_LEN, pn, SFP_STR_LEN, sn);
	}
	if (!(sfp->flags & WRS_HIST_SFP_PRESENT)) {
		pr_warning("Removed not present SFP! It should "
			   "never happen! Sfp vn:%.*s, pn:%.*s, sn: %.*s\n",
			   SFP_STR_LEN, vn, SFP_STR_LEN, pn, SFP_STR_LEN, sn);
	}

	sfp->sfp_lifetime += lifetime_of_remove - sfp->lastseen_swlifetime;
	sfp->flags &= ~WRS_HIST_SFP_PRESENT;
	sfp->mag = WRS_HIST_SFP_EMAGIC;
	sfp->ver = WRS_HIST_SFP_EMAGIC_VER;
	sfp->lastseen_swlifetime = lifetime_of_remove;
	sfp->lastseen_timestamp = time(NULL);
	hist_sfp_calc_sfp_crc(sfp);

	/* Update all other SFPs and write new information to the nand.
	 * We don't want to loose the information that particullar SFP was
	 * inserted, so we want to update nand. Since we will update the nand
	 * anyway update the entire database. */
	hist_sfp_nand_save();

	return;
}

static void hist_sfp_update(char *vn, char *pn, char *sn,
			    uint32_t lifetime_of_update)
{
	struct wrs_hist_sfp_entry *sfp;
	int new_entry;

	new_entry = hist_sfp_get_entry(vn, pn, sn, &sfp);

	if (new_entry & SFP_INSERT_NEW_ENTRY) {
		/* This might be printed at the startup, then it is normal */
		pr_info("Sfp vn:%.*s, pn:%.*s, sn: %.*s updated but was not "
			"present in the database\n",
			 SFP_STR_LEN, vn, SFP_STR_LEN, pn, SFP_STR_LEN, sn);
	} else if (sfp->flags & WRS_HIST_SFP_PRESENT) {
		/* SFP was present before, update it, we cannot use
		 * SFP_UPDATE_PERIOD, because usually insertion does not happen
		 * in sync with the update. We don't want to loose time shorter
		 * than SFP_UPDATE_PERIOD */
		sfp->sfp_lifetime +=
				lifetime_of_update - sfp->lastseen_swlifetime;
	}
	sfp->mag = WRS_HIST_SFP_EMAGIC;
	sfp->ver = WRS_HIST_SFP_EMAGIC_VER;
	sfp->lastseen_swlifetime = lifetime_of_update;
	sfp->lastseen_timestamp = time(NULL);
	sfp->flags |= WRS_HIST_SFP_PRESENT;
	hist_sfp_calc_sfp_crc(sfp);

	return;
}

static void hist_sfp_update_all(void)
{
	struct hal_port_state *ports;
	int i;
	uint32_t lifetime_of_update;

	lifetime_of_update = hist_uptime_lifetime_get();
	pr_debug("lifetime_of_update %d, hist_uptime_lifetime_get %ld\n",
		 lifetime_of_update, hist_uptime_lifetime_get());
	ports = hal_shmem_read_ports();
	if (!ports) {
		pr_error("Unable to get ports stats from the HAL!!!\n");
		return;
	}
	for (i = 0; i < HAL_MAX_PORTS; i++) {
		if (ports->state != HAL_PORT_STATE_DISABLED
		    && (ports->calib.sfp.vendor_name[0] != '\0'
			|| ports->calib.sfp.part_num[0] != '\0'
			|| ports->calib.sfp.vendor_serial[0] != '\0')
		) {
			hist_sfp_update(ports->calib.sfp.vendor_name,
					ports->calib.sfp.part_num,
					ports->calib.sfp.vendor_serial,
					lifetime_of_update);
		}
		/* go to next port */
		ports++;
	}
}


static int hist_sfp_nand_read(struct wrs_hist_sfp_nand *sfp_data, char *file)
{
	int fd;
	int ret;

	/* use O_NOATIME to avoid update of last access time */
	fd = open(file, O_RDONLY | O_NOATIME);
	if (fd < 0) {
		pr_error("Unable to read the file %s\n",
			 file);
		return -1;
	}
	/* clear data */
	memset(sfp_data, 0, sizeof(struct wrs_hist_sfp_nand));

	ret = read(fd, sfp_data, sizeof(struct wrs_hist_sfp_nand));
	close(fd);
	if (ret < 0) {
		pr_error("Read error from the file %s, ret %d, error(%d) "
			  "%s:\n",
			  file, ret, errno, strerror(errno));
		ret = -1;
	} else if (ret != sizeof(struct wrs_hist_sfp_nand)) {
		pr_error("Unable to read all data from the file %s, read %d, "
			 "expected %zu\n",
			 file, ret, sizeof(struct wrs_hist_sfp_nand));
		/* not fatal */
		ret = 1;
	}

	return ret;
}

static int hist_sfp_nand_read_verify(struct wrs_hist_sfp_nand *sfp_data,
				     char *file)
{
	uint8_t crc_saved;
	uint8_t crc_saved_end;
	uint8_t crc_calc;

	/* Check the first magic */
	if (sfp_data->magic != WRS_HIST_SFP_MAGIC) {
		pr_error("Wrong magic number in the file %s, is 0x%x and 0x%x,"
			  " expected 0x%x\n",
			  file, sfp_data->magic,
			  sfp_data->end_magic, WRS_HIST_SFP_MAGIC);
		return VERIFY_WRONG_MAGIC;
	}
	/* Check the first version */
	if (sfp_data->ver != WRS_HIST_SFP_MAGIC_VER) {
		pr_error("Wrong version number in the file %s, is 0x%x and "
			 "0x%x, expected 0x%x\n",
			  file, sfp_data->ver,
			  sfp_data->end_ver, WRS_HIST_SFP_MAGIC_VER);
		return VERIFY_WRONG_MAGIC;
	}

	/* Check the second magic */
	if (sfp_data->end_magic != WRS_HIST_SFP_MAGIC) {
		pr_error("Wrong magic number in the file %s, is 0x%x and 0x%x,"
			  " expected 0x%x\n",
			  file, sfp_data->magic,
			  sfp_data->end_magic, WRS_HIST_SFP_MAGIC);
		return VERIFY_WRONG_MAGIC_SEC;
	}
	/* Check the second version */
	if (sfp_data->end_ver != WRS_HIST_SFP_MAGIC_VER) {
		pr_error("Wrong version number in the file %s, is 0x%x and "
			 "0x%x, expected 0x%x\n",
			  file, sfp_data->ver,
			  sfp_data->end_ver, WRS_HIST_SFP_MAGIC_VER);
		return VERIFY_WRONG_MAGIC_SEC;
	}

	/* save both CRCs read from file */
	crc_saved = sfp_data->crc;
	crc_saved_end = sfp_data->end_crc;
	/* clear both CRCs, since they affect the calculation of the new crc */
	sfp_data->crc = 0;
	sfp_data->end_crc = 0;
	crc_calc = crc_fast((uint8_t *)sfp_data,
			    sizeof(struct wrs_hist_sfp_nand));
	if (crc_saved != crc_calc) {
		pr_error("Wrong CRC at the beginnig of the file with SFPs data"
			 " in the nand (%s)! Expected crc 0x%x, Calculated "
			 "0x%x\n",
			 file, crc_saved, crc_calc);
		return VERIFY_WRONG_CRC;
	}
	if (crc_saved_end != crc_calc) {
		pr_error("Wrong CRC at the end of the file with SFPs data in "
			 "the nand (%s)! Expected crc 0x%x, Calculated 0x%x\n",
			 file, crc_saved_end, crc_calc);
		return VERIFY_WRONG_CRC_SEC;
	}
	return VERIFY_OK;
}

static void hist_sfp_nand_write(struct wrs_hist_sfp_nand *data, char *file)
{
	int fd;
	int ret;
	uint8_t crc_calc;

	/* use O_NOATIME to avoid update of last access time
	 * O_SYNC to reduce caching problem
	 * O_TRUNC to truncate to length 0 */
	fd = open(file, O_WRONLY | O_TRUNC | O_CREAT | O_NOATIME | O_SYNC,
		  0644);
	if (fd < 0) {
		pr_error("Unable to write to the file %s\n", file);
		exit(1);
	}
	/* Save a timestamp when the data was saved */
	data->saved_swlifetime = hist_uptime_lifetime_get();
	data->saved_timestamp = time(NULL);

	/* calculate CRC */
	data->crc = 0;
	data->end_crc = 0;
	crc_calc = crc_fast((uint8_t *)data, sizeof(struct wrs_hist_sfp_nand));
	data->crc = crc_calc;
	data->end_crc = crc_calc;
	ret = write(fd, data, sizeof(struct wrs_hist_sfp_nand));

	if (ret < 0) {
		pr_error("Write error to the file %s, ret %d, error(%d) %s:\n",
			file, ret, errno, strerror(errno));
	} else if (ret != sizeof(struct wrs_hist_sfp_nand)) {
		pr_error("Unable to write all data to the file %s\n", file);
	}
	/* TODO: write SFPs specific information like temperature histograms */

	fsync(fd);
	close(fd);
}

void hist_sfp_nand_save(void)
{
	hist_sfp_update_all();
	pr_debug("Saving SFP data to the nand\n");
	hist_sfp_nand_write(&hist_shmem->hist_sfp_nand,
			    hist_sfp_nand_filename);
	hist_sfp_nand_write(&hist_shmem->hist_sfp_nand,
			    hist_sfp_nand_filename_backup);
}
