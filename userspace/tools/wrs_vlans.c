/*
 * Copyright (c) 2014, CERN
 *
 * Author: Grzegorz Daniluk <grzegorz.daniluk@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <minipc.h>
#include <rtud_exports.h>
#include "regs/endpoint-regs.h"

#include <libwr/switch_hw.h>
#include "fpga_io.h"
#include "wrs_vlans.h"

#include <libwr/shmem.h>
#include <libwr/rtu_shmem.h>

static int debug = 0;
static struct minipc_ch *rtud_ch;
static struct rtu_vlans_t *rtu_vlans = NULL;
static char *prgname;

/* runtime options */
static struct option ropts[] = {
	{"help", 0, NULL, OPT_HELP},
	{"debug", 0, &debug, 1},
	{"clear", 0, NULL, OPT_CLEAR},
	{"list", 0, NULL, OPT_LIST},
	{"ep", 1, NULL, OPT_EP_PORT},
	{"emode", 1, NULL, OPT_EP_QMODE},
	{"evid", 1, NULL, OPT_EP_VID},
	{"eprio", 1, NULL, OPT_EP_PRIO},
	{"eumask", 1, NULL, OPT_EP_UMASK},
	{"elist", 0, NULL, OPT_EP_LIST},
	{"rvid", 1, NULL, OPT_RTU_VID},
	{"rfid", 1, NULL, OPT_RTU_FID},
	{"rmask", 1, NULL, OPT_RTU_PMASK},
	{"rdrop", 1, NULL, OPT_RTU_DROP},
	{"rprio", 1, NULL, OPT_RTU_PRIO},
	{"del", 0, NULL, OPT_RTU_DEL},
	{0,}};
/*******************/

static struct s_port_vlans vlans[NPORTS];

static unsigned long portmask;

static int print_help(char *prgname);
static void print_config(struct s_port_vlans *vlans);
static int apply_settings(struct s_port_vlans *vlans);
static int clear_all(void);
static int set_rtu_vlan(int vid, int fid, int pmask, int drop, int prio,
			int del, int flags);
static void free_rtu_vlans(struct rtu_vlans_t *ptr);
static void list_rtu_vlans(void);
static void list_ep_vlans(void);
static int rtu_find_vlan(struct rtu_vlan_table_entry *rtu_vlan_entry, int vid,
					 int fid);
static int config_rtud(void);

struct rtu_vlan_table_entry *vlan_tab_shm;
struct wrs_shm_head *rtu_port_shmem;

static inline int nextport(int i, unsigned long pmask) /* helper for for_each_port() below */
{
	while (++i < NPORTS)
		if (pmask & (1 << i))
			return i;
	return -1;
}

#define iterate_ports(i, pmask) \
	for (i = -1; (i = nextport(i, pmask)) >= 0;)

#define for_each_port(i) \
	iterate_ports(i, portmask)

static int parse_mask(char *arg, unsigned long *pmask)
{
	int p1, p2;
	char c, *newarg, *s;

	newarg = strdup(arg);
	while ( (s = strtok(newarg, ",")) ) {
		newarg = NULL; /* for next iteration */
		switch (sscanf(s, "%i-%i%c", &p1, &p2, &c)) {
		case 1:
			p2 = p1;
		case 2:
			break;
		default:
			return -1;
		}
		if ((p1 > p2) || (p1 < 0) || (p2 >= NPORTS))
			return -1;
		for (; p1 <= p2; p1++) {
			*pmask |= (1 << p1);
			portmask |= (1 << p1);
		}
	}
	if (!debug)
		return 0;

	fprintf(stderr, "%s: working on ports:", prgname);
	iterate_ports(p1, *pmask)
		printf(" %i", p1);
	printf("\n");
	return 0;
}

int main(int argc, char *argv[])
{
	int c, i, arg;
	unsigned long conf_pmask = 0;	//current '--ep' port mask
	struct rtu_shmem_header *rtu_hdr;
	int n_wait = 0;
	int ret;

	prgname = argv[0];

	if (NPORTS > 8 * sizeof(portmask)) {
		/* build error: too big maxports */
		static __attribute__((used)) int
			array[8 * sizeof(portmask) - NPORTS];
	}

	if (argc == 1) {
		print_help(prgname);
		exit(0);
	}
	rtud_ch = minipc_client_create("rtud", 0);
	if(!rtud_ch) {
		fprintf(stderr, "%s: Can't connect to RTUd mini-rpc server\n",
			prgname);
		exit(1);
	}

	/* open rtu shm */
	while ((ret = wrs_shm_get_and_check(wrs_shm_rtu, &rtu_port_shmem)) != 0) {
		n_wait++;
		if (n_wait > 10) {
			if (ret == 1) {
				fprintf(stderr, "%s: Unable to open RTUd's "
					"shmem!\n", prgname);
			}
			if (ret == 2) {
				fprintf(stderr, "%s: Unable to read RTUd's "
					"version!\n", prgname);
			}
			exit(1);
		}
		sleep(1);
	}

	/* check rtu shm version */
	if (rtu_port_shmem->version != RTU_SHMEM_VERSION) {
		fprintf(stderr, "%s: unknown version %i (known is %i)\n",
			prgname, rtu_port_shmem->version, RTU_SHMEM_VERSION);
		exit(1);
	}

	/* get vlans array */
	rtu_hdr = (void *)rtu_port_shmem + rtu_port_shmem->data_off;
	vlan_tab_shm = wrs_shm_follow(rtu_port_shmem, rtu_hdr->vlans);

	if (!vlan_tab_shm) {
		fprintf(stderr, "%s: cannot follow pointer to vlans in "
			"RTU's shmem\n", prgname);
		exit(1);
	}

	if (shw_fpga_mmap_init() < 0) {
		fprintf(stderr, "%s: Can't access device memory\n", prgname);
		exit(1);
	}

	/*parse parameters*/
	while( (c = getopt_long(argc, argv, "h", ropts, NULL)) != -1) {
		switch(c) {
			case OPT_EP_PORT:
				//port number
				conf_pmask = 0;
				if (parse_mask(optarg, &conf_pmask) < 0) {
					fprintf(stderr, "%s: wrong port mask "
						"\"%s\"\n", prgname, optarg);
					exit(1);
				}

				break;
			case OPT_EP_QMODE:

				//qmode for port
				arg = atoi(optarg);
				if (arg < 0 || arg > 3) {
					fprintf(stderr, "%s: invalid qmode %i (\"%s\")\n",
						prgname, arg, optarg);
					exit(1);
				}
				iterate_ports(i, conf_pmask) {
					vlans[i].qmode = arg;
					vlans[i].valid_mask |= VALID_QMODE;
					/* untag is all-or-nothing: default untag if access mode */
					if ((vlans[i].valid_mask & VALID_UNTAG) == 0)
						vlans[i].untag_mask = (arg == 0);
				}
				break;
			case OPT_EP_PRIO:
				//priority value for port, forces fix_prio=1
				iterate_ports(i, conf_pmask) {
					vlans[i].prio_val = atoi(optarg);
					vlans[i].fix_prio = 1;
					vlans[i].valid_mask |= VALID_PRIO;
				}
				break;
			case OPT_EP_VID:
				//VID for port
				iterate_ports(i, conf_pmask) {
					vlans[i].vid = atoi(optarg);
					vlans[i].valid_mask |= VALID_VID;
				}
				break;
			case OPT_EP_UMASK:
				//untag mask -- currently 0 or 1. Overrides default set in QMODE above
				arg = atoi(optarg);
				if (arg < 0 || arg > 1) {
					fprintf(stderr, "%s: invalid unmask bit %i (\"%s\")\n",
						prgname, arg, optarg);
					exit(1);
				}
				iterate_ports(i, conf_pmask) {
					vlans[i].untag_mask = arg;
					vlans[i].valid_mask |= VALID_UNTAG;
				}
				break;
			case OPT_EP_LIST:
				// list endpoint stuff
				list_ep_vlans();
				break;

		  /****************************************************/
			/* RTU settings */
			case OPT_RTU_VID:
				set_rtu_vlan(atoi(optarg), -1, -1, 0, -1, 0, VALID_VID);
				break;
			case OPT_RTU_FID:
				set_rtu_vlan(-1, atoi(optarg), -1, 0, -1, 0, VALID_FID);
				break;
			case OPT_RTU_PMASK:
				set_rtu_vlan(-1, -1, (int) strtol(optarg, NULL, 16), 0, -1, 0, VALID_PMASK);
				break;
			case OPT_RTU_DROP:
				set_rtu_vlan(-1, -1, -1, atoi(optarg), -1, 0, VALID_DROP);
				break;
			case OPT_RTU_PRIO:
				set_rtu_vlan(-1, -1, -1, 0, atoi(optarg), 0, VALID_PRIO);
				break;
			case OPT_RTU_DEL:
				set_rtu_vlan(-1, -1, -1, 0, -1, 1, 0);
				break;

		  /****************************************************/
			/* Other settings */
			case OPT_CLEAR:
				if (clear_all())
					exit(1); /* message already printed */
				break;
			case OPT_DEBUG:
			case 0:
				break;
			case OPT_LIST:
				if (debug)
					minipc_set_logfile(rtud_ch,stderr);
				list_rtu_vlans();
				break;
			case '?':
			case OPT_HELP:
			default:
				print_help(prgname);
		}
	}

	if(debug) {
		minipc_set_logfile(rtud_ch,stderr);
		print_config(vlans);
	}
	apply_settings(vlans);
	free_rtu_vlans(rtu_vlans);

	return 0;
}

static int print_help(char *prgname)
{
	fprintf(stderr, "Use: %s [--ep <port number> <EP options> --ep <port number> "
			"<EP options> ...] [--rvid <vid> --rfid <fid> --rmask <mask> --rdrop "
			"--rprio <prio> --rvid <vid>...] [--debug]\n", prgname);

	fprintf(stderr,
			"Endpoint options:\n"
			"\t --emode <mode No.>  sets qmode for a port, possible values:\n"
			"\t \t 0: ACCESS           - tags untagged frames, drops tagged frames not belinging to configured VLAN\n"
			"\t \t 1: TRUNK            - passes only tagged frames, drops all untagged frames\n"
			"\t \t 2: VLANs disabled   - passess all frames as is\n"
			"\t \t 3: Unqualified port - passess all frames regardless VLAN config\n"
			"\t --eprio <priority>  sets priority for retagging\n"
			"\t --evid  <vid>       sets VLAN Id for port\n"
			"\t --eumask <hex mask> sets untag mask for port\n"
			"\t --elist             lists current EP configuration\n"
			"RTU options:\n"
			"\t --rvid <vid>        configure VLAN <vid> in rtud\n"
			"\t --del               delete selected VLAN from rtud\n"
			"\t --rfid <fid>        assign <fid> to configured VLAN\n"
			"\t --rmask <hex mask>  ports belonging to configured VLAN\n"
			"\t --rdrop <1/0>       drop/don't drop frames on VLAN\n"
			"\t --rprio <prio>      force priority for VLAN (-1 cancels priority override\n"
			"Other options:\n"
			"\t --clear  clears RTUd VLAN table\n"
			"\t --list   prints the content of RTUd VLAN table\n"
			"\t --debug  debug mode, prints VLAN config before writing Endpoint registers\n"
			"\t --help   prints this help message\n");
	return 0;
}

static void print_config(struct s_port_vlans *vlans)
{
	int i;

	for_each_port(i) {
		printf("port: %d, qmode: %d, qmode_valid: %d, fix_prio: %d, prio_val: %d, "
		       "prio_valid: %d, vid: %d, vid_valid: %d, untag_mask: 0x%X, untag_valid: %d\n",
		       i, vlans[i].qmode, ((vlans[i].valid_mask & VALID_QMODE) != 0), vlans[i].fix_prio,
		       vlans[i].prio_val, ((vlans[i].valid_mask & VALID_PRIO) != 0),vlans[i].vid,
		       ((vlans[i].valid_mask & VALID_VID) != 0), vlans[i].untag_mask,
		       ((vlans[i].valid_mask & VALID_UNTAG) != 0) );
	}
}

static uint32_t ep_read(int ep, int offset)
{
	return _fpga_readl(0x30000 + ep * 0x400 + offset);
}

static void ep_write(int ep, int offset, uint32_t value)
{
	_fpga_writel(0x30000 + ep * 0x400 + offset, value);
}

static int apply_settings(struct s_port_vlans *vlans)
{
	int ep;
	uint32_t v, r;

	for_each_port(ep) {
		/* VCR0 */
		r = offsetof(struct EP_WB, VCR0);
		v = ep_read(ep, r);
		if (vlans[ep].valid_mask & VALID_QMODE)
			v = (v & ~EP_VCR0_QMODE_MASK) | EP_VCR0_QMODE_W(vlans[ep].qmode);
		if (vlans[ep].valid_mask & VALID_PRIO) {
			v |= EP_VCR0_FIX_PRIO;
			v = (v & ~EP_VCR0_PRIO_VAL_MASK) | EP_VCR0_PRIO_VAL_W(vlans[ep].prio_val);
		}
		if (vlans[ep].valid_mask & VALID_VID)
			v = (v & ~EP_VCR0_PVID_MASK) | EP_VCR0_PVID_W(vlans[ep].vid);
		ep_write(ep, r, v);
		/* VCR1: loop over the whole bitmask */
		if (vlans[ep].untag_mask) {
			int i;

			r = offsetof(struct EP_WB, VCR1);
			for (i = 0;i < 4096/16; i++) {
				if (vlans[ep].untag_mask)
					ep_write(ep, r, (0xffff << 10) | i);
				else
					ep_write(ep, r, (0x0000 << 10) | i);
			}
		}
	}
	config_rtud();

	return 0;
}

static int config_rtud(void)
{
	struct rtu_vlans_t *cur;
	struct rtu_vlan_table_entry rtu_vlan_entry;
	int ret, val;

	cur = rtu_vlans;
	while(cur) {
		if (rtu_find_vlan(&rtu_vlan_entry, cur->vid,
				  (cur->flags & VALID_FID) ? cur->fid : -1)) {

		/*preserve previous settings if not overwritten*/
			if (!(cur->flags & VALID_FID))
				cur->fid = rtu_vlan_entry.fid;
			if (!(cur->flags & VALID_PMASK))
				cur->pmask = rtu_vlan_entry.port_mask;
			if (!(cur->flags & VALID_DROP))
				cur->drop = rtu_vlan_entry.drop;
			if (!(cur->flags & VALID_PRIO)) {
				cur->prio = rtu_vlan_entry.prio;
				cur->has_prio = rtu_vlan_entry.has_prio;
				cur->prio_override =
						rtu_vlan_entry.prio_override;
			}
		}
		ret = minipc_call(rtud_ch, MINIPC_TIMEOUT, &rtud_export_vlan_entry, &val,
				cur->vid, cur->fid, cur->pmask, cur->drop, cur->prio, cur->has_prio,
				cur->prio_override);
		cur = cur->next;
	}

	return 0;
}

static void list_rtu_vlans(void)
{
	unsigned ii;
	unsigned retries = 0;
	static struct rtu_vlan_table_entry vlan_tab_local[NUM_VLANS];

	/* read data, with the sequential lock to have all data consistent */
	while (1) {
		ii = wrs_shm_seqbegin(rtu_port_shmem);
		memcpy(&vlan_tab_local, vlan_tab_shm,
		       NUM_VLANS * sizeof(*vlan_tab_shm));
		retries++;
		if (retries > 100) {
			fprintf(stderr, "%s: couldn't read consistent data "
					"from RTU's shmem. Use inconsistent\n",
					prgname);
			break; /* use inconsistent data */
			}
		if (!wrs_shm_seqretry(rtu_port_shmem, ii))
			break; /* consistent read */
		usleep(1000);
	}

	printf("# VID    FID       MASK       DROP    PRIO    PRIO_OVERRIDE\n");
	printf("#----------------------------------------------------------\n");

	for (ii = 0; ii < NUM_VLANS; ii++) {
		/* ignore entires that are not active */
		if ((vlan_tab_local[ii].drop != 0)
		    && (vlan_tab_local[ii].port_mask == 0x0))
			continue;
		printf("%4d   %4d      0x%08x    ", ii, vlan_tab_local[ii].fid,
		       vlan_tab_local[ii].port_mask);

		if (vlan_tab_local[ii].drop == 0)
			printf("NO ");
		else
			printf("YES");

		if (vlan_tab_local[ii].has_prio == 0)
			printf("     --    ");
		else
			printf("     %1d    ", vlan_tab_local[ii].prio);

		if (vlan_tab_local[ii].prio_override == 0)
			printf("     NO ");
		else
			printf("     YES ");

		printf("\n");
	}
	printf("\n");

}

static void list_ep_vlans(void)
{
	uint32_t v, r;
	int ep;
	static char *names[] = {"ACCESS", "TRUNK", "disabled", "unqualified"};

	printf("#      QMODE    FIX_PRIO  PRIO    PVID     MAC\n");
	printf("#---------------------------------------------\n");
	for (ep = 0; ep < NPORTS; ep++) {
		r = offsetof(struct EP_WB, VCR0);
		v = ep_read(ep, r);
		printf(" %2i    %i %6.6s     %i      %i     %4i    %04x%08x\n",
		       ep, v & 3, names[v & 3],
		       v & EP_VCR0_FIX_PRIO ? 1 : 0,
		       EP_VCR0_PRIO_VAL_R(v),
		       EP_VCR0_PVID_R(v),
		       (int)ep_read(ep, offsetof(struct EP_WB, MACH)),
		       (int)ep_read(ep, offsetof(struct EP_WB, MACL)));
	}
	return;
}

static int clear_all(void)
{
	uint32_t r;
	int val, i;
	int ep;

	/* cancel all rtu-administered vlans */
	minipc_call(rtud_ch, MINIPC_TIMEOUT, &rtud_export_vlan_entry,
		    &val, 0, 0, 0xffffffff, 0, 0, 0, 0);

	for (i = 1; i < NUM_VLANS; i++) {
		if ((vlan_tab_shm[i].drop != 0)
		    && (vlan_tab_shm[i].port_mask == 0x0))
			continue;
		minipc_call(rtud_ch, MINIPC_TIMEOUT,
				    &rtud_export_vlan_entry, &val, i,
				    vlan_tab_shm[i].fid, 0, 1, 0, 0, 0);
		}

	/* cancel tagging/untagging in all endpoints*/
	for (ep = 0; ep < NPORTS; ep++) {
		r = offsetof(struct EP_WB, VCR0);
		ep_write(ep, r, 0x3 /* QMODE */);

		r = offsetof(struct EP_WB, VCR1);
		for (i = 0;i < 4096/16; i++) {
			ep_write(ep, r, (0x0000 << 10) | i); /* no untag */
		}
	}
	return 0;
}

static int set_rtu_vlan(int vid, int fid, int pmask, int drop, int prio,
			int del, int flags)
{
	struct rtu_vlans_t *cur = rtu_vlans;;

	if (!rtu_vlans && vid <= 0) {
		fprintf(stderr, "%s: missing \"--rvid <vid>\" before rtu cmd\n",
			prgname);
		return -1;
	}

	if (vid > 0) {
		cur = calloc(1, sizeof(*cur));
		if (!cur) {
			fprintf(stderr, "%s: %s\n", prgname, strerror(errno));
			return -1;
		}
		cur->vid = vid;
		cur->fid = vid;
		cur->flags |= VALID_VID;

		/* link to the list, next time head is "cur" */
		cur->next = rtu_vlans;
		rtu_vlans = cur;
	}
	if(flags & VALID_FID)
		cur->fid = fid;
	if(flags & VALID_PMASK)
		cur->pmask = pmask;
	if(flags & VALID_DROP)
		cur->drop = drop;
	if(flags & VALID_PRIO && prio >= 0) {
		cur->prio = prio;
		cur->has_prio = 1;
		cur->prio_override = 1;
	}
	else if(flags & VALID_PRIO) {
		cur->prio = prio;
		cur->has_prio = 0;
		cur->prio_override = 0;
	}
	if(del) {
		/*delete this vlan*/
		cur->pmask = 0;
		cur->drop = 1;
		cur->prio = 0;
		cur->has_prio = 0;
		cur->prio_override = 0;
		flags |= (VALID_PMASK | VALID_DROP | VALID_PRIO);
	}

	cur->flags |= flags;

	return 0;
}

static void free_rtu_vlans(struct rtu_vlans_t *ptr)
{
	struct rtu_vlans_t *next = NULL;

	while(ptr) {
		next = ptr->next;
		free(ptr);
		ptr = next;
	}
}

static int rtu_find_vlan(struct rtu_vlan_table_entry *rtu_vlan_entry, int vid,
					 int fid)
{
	unsigned ii;
	unsigned retries = 0;

	/* copy data no mater if it will be used later, with the sequential
	 * lock to have all data consistent */
	while (1) {
		ii = wrs_shm_seqbegin(rtu_port_shmem);
		memcpy(rtu_vlan_entry, &vlan_tab_shm[vid],
			sizeof(*rtu_vlan_entry));
		retries++;
		if (retries > 100) {
			fprintf(stderr, "%s: couldn't read consistent "
				"data from RTU's shmem. "
				"Use inconsistent\n", prgname);
			break; /* use inconsistent data */
			}
		if (!wrs_shm_seqretry(rtu_port_shmem, ii))
			break; /* consistent read */
		usleep(1000);
	}

	/* Ignore entires that are not active */
	if ((rtu_vlan_entry->drop != 0)
	    && (rtu_vlan_entry->port_mask == 0x0))
		return 0;

	if ((fid == rtu_vlan_entry->fid) || (fid == -1))
		return 1;
	return 0;
}
