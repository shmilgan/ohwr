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
#include <libwr/config.h>

static int debug = 0;
static struct minipc_ch *rtud_ch;
static struct rtu_vlans_t *rtu_vlans = NULL;
static char *prgname;

/* runtime options */
static struct option ropts[] = {
	{"help", 0, NULL, OPT_HELP},
	{"debug", optional_argument, NULL, OPT_DEBUG},
	{"clear", 0, NULL, OPT_CLEAR},
	{"list", 0, NULL, OPT_LIST},
	{"port", 1, NULL, OPT_P_PORT},
	{"pmode", 1, NULL, OPT_P_QMODE},
	{"pvid", 1, NULL, OPT_P_VID},
	{"pprio", 1, NULL, OPT_P_PRIO},
	{"pumask", 1, NULL, OPT_P_UMASK},
	{"plist", 0, NULL, OPT_P_LIST},
	{"rvid", 1, NULL, OPT_RTU_VID},
	{"rfid", 1, NULL, OPT_RTU_FID},
	{"rmask", 1, NULL, OPT_RTU_PMASK},
	{"rdrop", 1, NULL, OPT_RTU_DROP},
	{"rprio", 1, NULL, OPT_RTU_PRIO},
	{"del", 0, NULL, OPT_RTU_DEL},
	{"file", 1, NULL, OPT_FILE_READ},
	{0,}};
/*******************/
static struct vlan_sets dot_config_vlan_sets[] = {
	{"VLANS_ENABLE_SET1", 0, 22},
	{"VLANS_ENABLE_SET2", 23, 100},
	{"VLANS_ENABLE_SET3", 101, 4095},
	{NULL, 0, 0}
};
static struct s_port_vlans vlans[NPORTS];

static unsigned long portmask;

static void set_p_qmode(int ep, int arg_mode);
static void set_p_vid(int ep, char *arg_vid);
static void set_p_prio(int ep, char *arg_prio);
static void set_p_umask(int ep, int arg_umask);
static int check_rtu(char *name, char *arg_val, int min, int max);
static int print_help(char *prgname);
static void print_config_rtu(struct s_port_vlans *vlans);
static void print_config_vlan(void);
static int apply_settings(struct s_port_vlans *vlans);
static int clear_all(void);
static int set_rtu_vlan(int vid, int fid, int pmask, int drop, int prio,
			int del, int flags);
static void free_rtu_vlans(struct rtu_vlans_t *ptr);
static void list_rtu_vlans(void);
static void list_p_vlans(void);
static int rtu_find_vlan(struct rtu_vlan_table_entry *rtu_vlan_entry, int vid,
					 int fid);
static int config_rtud(void);
static int read_dot_config(char *dot_config_file);
static void read_dot_config_vlans(int vlan_min, int vlan_max);

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
		/* parameter --port should be from the range 1..18,
		 * but internally we use 0..17 */
		p1--;
		p2--;
		if ((p1 > p2) || (p1 < 0) || (p2 >= NPORTS))
			return -1;
		for (; p1 <= p2; p1++) {
			*pmask |= (1 << p1);
			portmask |= (1 << p1);
		}
	}
	if (!debug)
		return 0;

	printf("%s: working on ports:\n", prgname);
	iterate_ports(p1, *pmask)
		printf(" %i", p1 + 1);
	printf("\n");
	return 0;
}

int main(int argc, char *argv[])
{
	int c, i, arg;
	unsigned long conf_pmask = 0; /* current '--port' port mask */
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

	n_wait = 0;
	/* connect to the RTUd mini-rpc */
	while((rtud_ch = minipc_client_create("rtud", 0)) == 0) {
		n_wait++;
		if (n_wait > 10) {
			fprintf(stderr, "%s: Can't connect to RTUd mini-rpc "
					"server\n", prgname);
			exit(1);
		}
		sleep(1);
	}

	n_wait = 0;
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
	while ((c = getopt_long(argc, argv, "hf:", ropts, NULL)) != -1) {
		switch(c) {
		case OPT_P_PORT:
			/* port number */
			conf_pmask = 0;
			if (parse_mask(optarg, &conf_pmask) < 0) {
				fprintf(stderr, "%s: wrong port mask "
					"\"%s\"\n", prgname, optarg);
				exit(1);
			}
			break;

		case OPT_P_QMODE:
			/* qmode for port */
			arg = atoi(optarg);
			if (arg < 0 || arg > 3) {
				fprintf(stderr, "%s: invalid qmode %i (\"%s\""
					")\n",
					prgname, arg, optarg);
				exit(1);
			}
			iterate_ports(i, conf_pmask) {
				set_p_qmode(i, arg);
			}
			break;

		case OPT_P_PRIO:
			/* priority value for port, forces fix_prio=1 */
			iterate_ports(i, conf_pmask) {
				set_p_prio(i, optarg);
			}
			break;

		case OPT_P_VID:
			/* VID for port */
			iterate_ports(i, conf_pmask) {
				set_p_vid(i, optarg);
			}
			break;

		case OPT_P_UMASK:
			/* untag mask -- currently 0 or 1. Overrides default
			 * set in QMODE above */
			arg = atoi(optarg);
			if (arg < 0 || arg > 1) {
				fprintf(stderr, "%s: invalid unmask bit %i "
					"(\"%s\")\n",
					prgname, arg, optarg);
				exit(1);
			}
			iterate_ports(i, conf_pmask) {
				set_p_umask(i, arg);
			}
			break;

		case OPT_P_LIST:
			/* list endpoint stuff */
			list_p_vlans();
			break;

	  /****************************************************/
		/* RTU settings */
		case OPT_RTU_VID:
			ret = check_rtu("rtu vid", optarg, RTU_VID_MIN,
					RTU_VID_MAX);
			if (ret < 0)
				exit(1);
			set_rtu_vlan(ret, -1, -1, 0, -1, 0, VALID_VID);
			break;
		case OPT_RTU_FID:
			ret = check_rtu("rtu fid", optarg, RTU_FID_MIN,
					RTU_FID_MAX);
			if (ret < 0)
				exit(1);
			set_rtu_vlan(-1, ret, -1, 0, -1, 0, VALID_FID);
			break;
		case OPT_RTU_PMASK:
			set_rtu_vlan(-1, -1, (int) strtol(optarg, NULL, 16), 0,
				     -1, 0, VALID_PMASK);
			break;
		case OPT_RTU_DROP:
			ret = check_rtu("rtu drop", optarg, 0, 1);
			if (ret < 0)
				exit(1);
			set_rtu_vlan(-1, -1, -1, ret, -1, 0, VALID_DROP);
			break;
		case OPT_RTU_PRIO:
			ret = check_rtu("rtu prio", optarg, RTU_PRIO_MIN,
					RTU_PRIO_MAX);
			if (ret < 0)
				exit(1);
			set_rtu_vlan(-1, -1, -1, 0, ret, 0, VALID_PRIO);
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
			if (optarg)
				debug = atoi(optarg);
			else
				debug = 1;

			printf("Set debug to %d\n", debug);
			break;
		case 0:
			break;
		case OPT_LIST:
			if (debug)
				minipc_set_logfile(rtud_ch, stderr);
			list_rtu_vlans();
			break;
		case OPT_FILE_READ:
			if (debug)
				printf("Using file %s as dot-config\n",
				       optarg);
			/* read dot-config */
			ret = read_dot_config(optarg);
			if (ret < 0)
				exit(-ret);

			break;
		case '?':
		case OPT_HELP:
		default:
			print_help(prgname);
			exit(0);
		}
	}

	if (debug > 1)
		minipc_set_logfile(rtud_ch, stderr);

	if (debug) {
		print_config_rtu(vlans);
		print_config_vlan();
	}
	/* apply vlans and rtu_vlans */
	apply_settings(vlans);
	free_rtu_vlans(rtu_vlans);

	return 0;
}

static void set_p_qmode(int ep, int arg_mode)
{
	vlans[ep].qmode = arg_mode;
	vlans[ep].valid_mask |= VALID_QMODE;
	/* untag is all-or-nothing: default untag if access mode */
	if ((vlans[ep].valid_mask & VALID_UNTAG) == 0)
		vlans[ep].untag_mask = (arg_mode == 0);
}

static void set_p_vid(int ep, char *arg_vid)
{
	int vid;

	vid = atoi(arg_vid);
	if (vid < PORT_VID_MIN || vid > PORT_VID_MAX) {
		fprintf(stderr, "%s: invalid vid %i (\"%s\") for port %d\n",
			prgname, vid, arg_vid, ep + 1);
		exit(1);
	}
	vlans[ep].vid = atoi(arg_vid);
	vlans[ep].valid_mask |= VALID_VID;
}

static void set_p_prio(int ep, char *arg_prio)
{
	int prio;

	prio = atoi(arg_prio);
	if (prio < PORT_PRIO_MIN || prio > PORT_PRIO_MAX) {
		fprintf(stderr, "%s: invalid priority %i (\"%s\") for port %d"
			"\n", prgname, prio, arg_prio, ep + 1);
		exit(1);
	}
	vlans[ep].prio_val = prio;
	vlans[ep].fix_prio = 1;
	vlans[ep].valid_mask |= VALID_PRIO;
}

static void set_p_umask(int ep, int arg_umask)
{
	vlans[ep].untag_mask = arg_umask;
	vlans[ep].valid_mask |= VALID_UNTAG;
}

static int check_rtu(char *name, char *arg_val, int min, int max)
{
	int val;

	val = atoi(arg_val);
	if (val < min || val > max) {
		fprintf(stderr, "%s: invalid %s %i (\"%s\")"
			"\n", prgname, name, val, arg_val);
		return -1;
	}
	return val;
}

static int print_help(char *prgname)
{
	fprintf(stderr, "Use: %s [--debug|--debug=<level>] "
			"[--port <port number 1..18> <port options> "
			"--port <port number> <port options> ...] "
			"[--rvid <vid> --rfid <fid> --rmask <mask> --rdrop "
			"--rprio <prio> --rvid <vid>...]  "
			"[-f|--file <dot-config>]\n", prgname);

	fprintf(stderr,
			"Port options:\n"
			"\t --pmode <mode No.>  sets qmode for a port, possible values:\n"
			"\t \t 0: ACCESS           - tags untagged frames, drops tagged frames not belonging to configured VLAN\n"
			"\t \t 1: TRUNK            - passes only tagged frames, drops all untagged frames\n"
			"\t \t 2: VLANs disabled   - passes all frames as is\n"
			"\t \t 3: Unqualified port - passes all frames regardless of VLAN config\n"
			"\t --pprio <priority>  sets priority for retagging\n"
			"\t --pvid  <vid>       sets VLAN Id for port\n"
			"\t --pumask <hex mask> sets untag mask for port\n"
			"\t --plist             lists current ports configuration\n"
			"RTU options:\n"
			"\t --rvid <vid>        configure VLAN <vid> in rtud\n"
			"\t --del               delete selected VLAN from rtud\n"
			"\t --rfid <fid>        assign <fid> to configured VLAN\n"
			"\t --rmask <hex mask>  ports belonging to configured VLAN\n"
			"\t --rdrop <1/0>       drop/don't drop frames on VLAN\n"
			"\t --rprio <prio>      force priority for VLAN (-1 cancels priority override)\n"
			"Other options:\n"
			"\t --clear                  clears RTUd VLAN table\n"
			"\t --list                   prints the content of RTUd VLAN table\n"
			"\t -f|--file <file>         reads configuration from the provided dot-config file\n"
			"\t --debug|--debug=<level>  set the debug level:\n"
			"\t \t 0 - no debug output (default)\n"
			"\t \t 1 - prints config before applaying it (--debug)\n"
			"\t \t 2 - prints extra information about parameters read from the dot-config\n"
			"\t \t     note: --debug=2 should be placed before -f|--file parameter\n"
			"\t --help                   prints this help message\n");
	return 0;
}

static void print_config_rtu(struct s_port_vlans *vlans)
{
	int i;

	for_each_port(i) {
		printf("port: %2d, qmode: %d, qmode_valid: %d, fix_prio: %d, "
		       "prio_val: %d, prio_valid: %d, vid: %2d, vid_valid: %d,"
		       " untag_mask: 0x%X, untag_valid: %d\n",
		       i + 1,
		       vlans[i].qmode,
		       ((vlans[i].valid_mask & VALID_QMODE) != 0),
		       vlans[i].fix_prio,
		       vlans[i].prio_val,
		       ((vlans[i].valid_mask & VALID_PRIO) != 0),
		       vlans[i].vid,
		       ((vlans[i].valid_mask & VALID_VID) != 0),
		       vlans[i].untag_mask,
		       ((vlans[i].valid_mask & VALID_UNTAG) != 0)
		       );
	}
}

static void print_config_vlan(void)
{
	struct rtu_vlans_t *cur;
	cur = rtu_vlans;
	while (cur) {
		printf("vid: ");
		if (cur->flags & VALID_VID)
			printf("%4d ", cur->vid);
		else
			printf("     ");

		printf("fid: ");
		if (cur->flags & VALID_FID)
			printf("%4d ", cur->fid);
		else
			printf("     ");

		printf("port_mask: ");
		if (cur->flags & VALID_PMASK)
			printf("0x%05x ", cur->pmask);
		else
			printf("        ");

		printf("drop: ");
		if (cur->flags & VALID_DROP)
			printf("%1d ", cur->drop);
		else
			printf("  ");

		printf("prio: ");
		if (cur->flags & VALID_PRIO)
			printf("%1d", cur->prio);

		printf("\n");
		cur = cur->next;
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
	int val;

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

		minipc_call(rtud_ch, MINIPC_TIMEOUT, &rtud_export_vlan_entry, &val,
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

static void list_p_vlans(void)
{
	uint32_t v, r;
	int ep;
	static char *names[] = {"ACCESS", "TRUNK", "disabled", "unqualified"};

	printf("#        QMODE    FIX_PRIO  PRIO    PVID     MAC\n");
	printf("#-----------------------------------------------\n");
	for (ep = 0; ep < NPORTS; ep++) {
		r = offsetof(struct EP_WB, VCR0);
		v = ep_read(ep, r);
		printf("wri%-2i    %i %6.6s     %i      %i     %4i    %04x%08x\n",
		       ep + 1, v & 3, names[v & 3],
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

	if (!rtu_vlans && vid < 0) {
		fprintf(stderr, "%s: missing \"--rvid <vid>\" before rtu cmd\n",
			prgname);
		return -1;
	}

	if (vid >= 0) {
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

static int read_dot_config(char *dot_config_file)
{
	int line;
	int port;
	char *ret;
	char buff[60];
	char *val_ch;
	int i;

	if (access(dot_config_file, R_OK)) {
		fprintf(stderr, "Unable to read dot-config file %s\n",
			dot_config_file);
		return -1;
	}

	line = libwr_cfg_read_file(dot_config_file);

	if (line == -1) {
		fprintf(stderr, "Unable to read dot-config file %s or error in"
			" line 1\n", dot_config_file);
		return -1;
	} else if (line) {
		fprintf(stderr, "Error in dot-config file %s, error in line "
			"%d\n",
			 dot_config_file, -line);
		return -1;
	}

	ret = libwr_cfg_get("VLANS_ENABLE");
	if (!ret || strcmp(ret, "y")) {
		if (debug > 1)
			printf("VLANS not enabled\n");
		return -2;
	}

	for (port = 1; port <= NPORTS; port++) {
		portmask = portmask | (1 << (port - 1));

		sprintf(buff, "VLANS_PORT%02d_MODE_ACCESS", port);
		ret = libwr_cfg_get(buff);
		if (ret && !strcmp(ret, "y")) {
			if (debug > 1)
				printf("Found %s\n", buff);
			set_p_qmode(port - 1, 0);
		}
		sprintf(buff, "VLANS_PORT%02d_MODE_TRUNK", port);
		ret = libwr_cfg_get(buff);
		if (ret && !strcmp(ret, "y")) {
			if (debug > 1)
				printf("Found %s\n", buff);
			set_p_qmode(port - 1, 1);
		}
		sprintf(buff, "VLANS_PORT%02d_MODE_DISABLED", port);
		ret = libwr_cfg_get(buff);
		if (ret && !strcmp(ret, "y")) {
			if (debug > 1)
				printf("Found %s\n", buff);
			set_p_qmode(port - 1, 2);
		}
		sprintf(buff, "VLANS_PORT%02d_MODE_UNQUALIFIED", port);
		ret = libwr_cfg_get(buff);
		if (ret && !strcmp(ret, "y")) {
			if (debug > 1)
				printf("Found %s\n", buff);
			set_p_qmode(port - 1, 3);
		}
		sprintf(buff, "VLANS_PORT%02d_UMASK_ALL", port);
		ret = libwr_cfg_get(buff);
		if (ret && !strcmp(ret, "y")) {
			if (debug > 1)
				printf("Found %s\n", buff);
			set_p_umask(port - 1, 1);
		}
		sprintf(buff, "VLANS_PORT%02d_UMASK_NONE", port);
		ret = libwr_cfg_get(buff);
		if (ret && !strcmp(ret, "y")) {
			if (debug > 1)
				printf("Found %s\n", buff);
			set_p_umask(port - 1, 0);
		}
		sprintf(buff, "VLANS_PORT%02d_PRIO", port);
		val_ch = libwr_cfg_get(buff);
		if (val_ch) {
			if (debug > 1)
				printf("Found %s=%s\n", buff, val_ch);
			set_p_prio(port - 1, val_ch);
		}
		sprintf(buff, "VLANS_PORT%02d_VID", port);
		val_ch = libwr_cfg_get(buff);
		if (val_ch) {
			if (debug > 1)
				printf("Found %s=%s\n", buff, val_ch);
			set_p_vid(port - 1, val_ch);
		}
	}

	for (i = 0; dot_config_vlan_sets[i].name; i++) {
		ret = libwr_cfg_get(dot_config_vlan_sets[i].name);
		if (ret && !strcmp(ret, "y")) {
			read_dot_config_vlans(dot_config_vlan_sets[i].min,
					      dot_config_vlan_sets[i].max);
		}
	}
	return 0;
}

static void read_dot_config_vlans(int vlan_min, int vlan_max)
{
	int fid, prio, drop, pmask;
	int vlan_flags;
	int vlan;
	char buff[60];
	char *beg; /* beginning char of a port number */
	char *end; /* end char of a port number */
	int port;

	for (vlan = vlan_min; vlan <= vlan_max; vlan++) {
		vlan_flags = 0;
		fid = -1;
		prio = -1;
		drop = -1;
		pmask = 0;
		if (!libwr_cfg_convert2("VLANS_VLAN%04d", "fid", LIBWR_STRING,
					buff, vlan)) {
			fid = check_rtu("rtu vid", buff, RTU_FID_MIN,
					RTU_FID_MAX);
			if (fid < 0)
				exit(1);
			if (debug > 1)
				printf("Vlan %4d: Found fid=%d\n",
				       vlan, fid);
			vlan_flags |= VALID_FID;
		}

		if (!libwr_cfg_convert2("VLANS_VLAN%04d", "prio", LIBWR_STRING,
					buff, vlan)) {
			prio = check_rtu("rtu prio", buff, RTU_PRIO_MIN,
					 RTU_PRIO_MAX);
			if (prio < 0)
				exit(1);
			vlan_flags |= VALID_PRIO;
			if (debug > 1)
				printf("Vlan %4d: Found prio=%d\n",
				       vlan, prio);
		}
		if (!libwr_cfg_convert2("VLANS_VLAN%04d", "drop", LIBWR_STRING,
					buff, vlan)) {
			if (!strcmp(buff, "y"))
				sprintf(buff, "1");
			else if (!strcmp(buff, "n"))
				sprintf(buff, "0");
			else {
				fprintf(stderr, "%s: invalid drop parameter"
					"\"%s\" in VLANS_VLAN%04d\n", prgname,
					buff, vlan);
				exit(1);
			}
			drop = check_rtu("rtu drop", buff, 0, 1);
			if (drop < 0)
				exit(1);
			if (debug > 1)
				printf("Vlan %4d: Found drop=%d\n",
				       vlan, drop);
			vlan_flags |= VALID_DROP;
		}
		if (!libwr_cfg_convert2("VLANS_VLAN%04d", "ports",
					LIBWR_STRING, buff, vlan)) {
			beg = buff;
			while (1) {
				port = strtol(beg, &end, 0);
				if (end == beg) {
					fprintf(stderr, "%s: invalid ports "
						"parameter\"%s\" in "
						"VLANS_VLAN%04d\n", prgname,
						buff, vlan);
					exit(1);
				}
				if (port < 1 || port > NPORTS) {
					fprintf(stderr, "%s: invalid port %d "
						"(\"%s\") for vlan %4d\n",
						prgname, port, buff, vlan);
					exit(1);
				}
				if (debug > 1)
					printf("Vlan %4d: Found port %d\n",
					       vlan, port);
				pmask |= 1 << (port - 1);
				if (*end == '\0') {
					/* end of ports' string */
					break;
				}
				if (*end == ':') {
					/* check next port */
					beg = end;
					beg++; /* skip ":" */
					continue;
				}
			}

			if (pmask < RTU_PMASK_MIN || pmask > RTU_PMASK_MAX) {
				fprintf(stderr, "%s: invalid port mask 0x%x "
					"(\"%s\") for vlan %4d\n", prgname,
					pmask, buff, vlan);
				exit(1);
			}
			if (debug > 1)
				printf("Vlan %4d: Port mask 0x%05x\n",
				       vlan, pmask);
			vlan_flags |= VALID_PMASK;
		}
		if (vlan_flags) {
			/* at least one parameter is present, so trigger
			 * the fill */
			vlan_flags |= VALID_VID;
			set_rtu_vlan(vlan, fid, pmask, drop, prio, 0,
				     vlan_flags);
		}
	}
}
