#include <string.h>

#include <libwr/hist_shmem.h>
#include <libwr/wrs-msg.h>

#include "wrs_hist.h"
#define SFP_STR_LEN 16
static struct wrs_hist_sfp_entry * hist_sfp_find(char *vn, char *pn, char *sn)
{
	int i;
	for (i = 0; i < WRS_HIST_MAX_SFPS; i++) {
		if (!hist_shmem->hist_sfp_nand.sfps[i].sfp_uptime) {
			/* Skip entries never used */
			continue;
		}
		if (!strncmp(hist_shmem->hist_sfp_nand.sfps[i].vn, vn, SFP_STR_LEN)
		    && !strncmp(hist_shmem->hist_sfp_nand.sfps[i].pn, pn, SFP_STR_LEN)
		    && !strncmp(hist_shmem->hist_sfp_nand.sfps[i].sn, sn, SFP_STR_LEN)) {
			/* sfp found */
			pr_debug("Sfp vn:%.*s, pn:%.*s, sn: %.*s found in index %d\n",
				 SFP_STR_LEN, vn, SFP_STR_LEN, pn, SFP_STR_LEN, sn, i);
			return &hist_shmem->hist_sfp_nand.sfps[i];
		}
	}
	return NULL;
}

static struct wrs_hist_sfp_entry * hist_sfp_find_empty(void)
{
	int i;

	for (i = 0; i < WRS_HIST_MAX_SFPS; i++) {
		if (!hist_shmem->hist_sfp_nand.sfps[i].sfp_uptime) {
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

	for (i = 1; i < WRS_HIST_MAX_SFPS; i++) {
		if (oldest_sfp->last_seen > hist_shmem->hist_sfp_nand.sfps[i].last_seen) {
			/* An older entry found */
			oldest_sfp = &hist_shmem->hist_sfp_nand.sfps[i];
		}
	}
	return oldest_sfp;
}

static struct wrs_hist_sfp_entry * hist_sfp_get_entry(void)
{
	struct wrs_hist_sfp_entry *sfp;

	sfp = hist_sfp_find_empty();
	if (!sfp) {
		sfp = hist_sfp_find_oldest();
	}

	return sfp;
}


void hist_sfp_insert(char *vn, char *pn, char *sn)
{
	struct wrs_hist_sfp_entry *sfp;
	sfp = hist_sfp_find(vn, pn, sn);
	if (!sfp) {
		sfp = hist_sfp_get_entry();
		pr_info("removing sfp vn:%.*s, pn:%.*s, sn: %.*s\n",
			SFP_STR_LEN, sfp->vn, SFP_STR_LEN, sfp->pn,
			SFP_STR_LEN, sfp->sn);
		strncpy(sfp->vn, vn, SFP_STR_LEN);
		strncpy(sfp->pn, pn, SFP_STR_LEN);
		strncpy(sfp->sn, sn, SFP_STR_LEN);
		sfp->last_seen = 0;
		sfp->sfp_uptime = 0;
	}

	/* update last seen */
	sfp->last_seen += 1;
	sfp->sfp_uptime += 1;
}

void hist_sfp_update(char *vn, char *pn, char *sn)
{
	/* update last_seen */
	/* update sfp_uptime */
	
	struct wrs_hist_sfp_entry *sfp;
	sfp = hist_sfp_find(vn, pn, sn);
	if (!sfp) {
		/* TODO: if not available in the database then maybe add it? */
		pr_info("Sfp vn:%.*s, pn:%.*s, sn: %.*s removed but not "
			"present in the database\n",
			 SFP_STR_LEN, vn, SFP_STR_LEN, pn, SFP_STR_LEN, sn);

		return;
	}
	sfp->sfp_uptime += 1;
}

void hist_sfp_remove(char *vn, char *pn, char *sn)
{
	/* do the same as for update */
	hist_sfp_update(vn, pn, sn);
}
