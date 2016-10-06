#include <string.h>

#include <libwr/util.h>
#include <libwr/hist_shmem.h>
#include <libwr/wrs-msg.h>

#include "wrs_hist.h"
#define SFP_STR_LEN 16
#define SFP_INSERT_NEW_ENTRY 1

static struct wrs_hist_sfp_entry * hist_sfp_find(char *vn, char *pn, char *sn)
{
	int i;
	struct wrs_hist_sfp_nand *hist_sfp_nand_loc;
	hist_sfp_nand_loc = &hist_shmem->hist_sfp_nand;
	for (i = 0; i < WRS_HIST_MAX_SFPS; i++) {
		if (!hist_sfp_nand_loc->sfps[i].sfp_lifetime) {
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
		if (!hist_shmem->hist_sfp_nand.sfps[i].sfp_lifetime) {
			/* Return an entry never used */
			return &hist_shmem->hist_sfp_nand.sfps[i];
		}
	}
	return NULL;
}

static void hist_sfp_copy(struct wrs_hist_sfp_entry *to,
			  struct wrs_hist_sfp_entry *from)
{
	*to = *from;
	/* TODO: copy sfp_temp if exist */
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

void hist_sfp_insert(char *vn, char *pn, char *sn)
{
	struct wrs_hist_sfp_entry *sfp;

	hist_sfp_get_entry(vn, pn, sn, &sfp);
	sfp->sfp_lifetime |= WRS_HIST_SFP_PRESENT;
	/* update last seen */
	sfp->lastseen_swlifetime = hist_uptime_lifetime_get();
	sfp->lastseen_timestamp = time(NULL);

}

void hist_sfp_remove(char *vn, char *pn, char *sn)
{
	struct wrs_hist_sfp_entry *sfp;
	int new_entry;
	uint32_t lifetime_of_remove;
	
	/* avoid multipe calls of hist_uptime_lifetime_get() to have consistent
	 * time */
	lifetime_of_remove =
			hist_uptime_lifetime_get() & ~WRS_HIST_SFP_PRESENT;

	new_entry = hist_sfp_get_entry(vn, pn, sn, &sfp);
	if (new_entry & SFP_INSERT_NEW_ENTRY) {
		pr_warning("New SFP entry added at SFP removal! It should "
			   "never happen! Sfp vn:%.*s, pn:%.*s, sn: %.*s\n",
			   SFP_STR_LEN, vn, SFP_STR_LEN, pn, SFP_STR_LEN, sn);
	}
	if (!(sfp->sfp_lifetime & WRS_HIST_SFP_PRESENT)) {
		pr_warning("Removed not present SFP! It should "
			   "never happen! Sfp vn:%.*s, pn:%.*s, sn: %.*s\n",
			   SFP_STR_LEN, vn, SFP_STR_LEN, pn, SFP_STR_LEN, sn);
	}

	sfp->sfp_lifetime += lifetime_of_remove - sfp->lastseen_swlifetime;
	sfp->sfp_lifetime &= ~WRS_HIST_SFP_PRESENT;
	sfp->lastseen_swlifetime = lifetime_of_remove;
	sfp->lastseen_timestamp = time(NULL);

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
	} else if (sfp->sfp_lifetime & WRS_HIST_SFP_PRESENT) {
		/* SFP was present before, update it, we cannot use
		 * SFP_UPDATE_PERIOD, because usually insertion does not happen
		 * in sync with the update. We don't want to loose time shorter
		 * than SFP_UPDATE_PERIOD */
		sfp->sfp_lifetime +=
				lifetime_of_update - sfp->lastseen_swlifetime;
	}
	sfp->lastseen_swlifetime = lifetime_of_update;
	sfp->lastseen_timestamp = time(NULL);
	sfp->sfp_lifetime |= WRS_HIST_SFP_PRESENT;

	return;
}

void hist_sfp_update_all(void)
{
	struct hal_port_state *ports;
	int i;
	uint32_t lifetime_of_update;

	lifetime_of_update =
			hist_uptime_lifetime_get() & ~WRS_HIST_SFP_PRESENT;
	pr_debug("lifetime_of_update %d, hist_uptime_lifetime_get %ld\n",
		 lifetime_of_update, hist_uptime_lifetime_get());
	ports = hal_shmem_read_ports();
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
