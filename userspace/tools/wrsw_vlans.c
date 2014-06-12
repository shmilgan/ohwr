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
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <minipc.h>
#include <rtud_exports.h>
#include "wrsw_vlans.h"

int debug = 0;
struct minipc_ch *rtud_ch;
struct rtu_vlans_t *rtu_vlans = NULL;
char *prgname;

/* runtime options */
struct option ropts[] = {
	{"help", 0, NULL, OPT_HELP},
	{"debug", 0, &debug, 1},
	{"clear", 0, NULL, OPT_CLEAR},
	{"list", 0, NULL, OPT_LIST},
	{"ep", 1, NULL, OPT_EP_PORT},
	{"emode", 1, NULL, OPT_EP_QMODE},
	{"evid", 1, NULL, OPT_EP_VID},
	{"eprio", 1, NULL, OPT_EP_PRIO},
	{"eumask", 1, NULL, OPT_EP_UMASK},
	{"rvid", 1, NULL, OPT_RTU_VID},
	{"rfid", 1, NULL, OPT_RTU_FID},
	{"rmask", 1, NULL, OPT_RTU_PMASK},
	{"rdrop", 1, NULL, OPT_RTU_DROP},
	{"rprio", 1, NULL, OPT_RTU_PRIO},
	{"del", 0, NULL, OPT_RTU_DEL},
	{0,}};
/*******************/

struct s_port_vlans vlans[NPORTS];

unsigned long portmask;

static inline int nextport(int i) /* helper for for_each_port() below */
{
	while (++i < NPORTS)
		if (portmask & (1 << i))
			return i;
	return -1;
}

#define for_each_port(i) \
	for (i = -1; (i = nextport(i)) >= 0;)

static int parse_mask(char *arg)
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
		for (; p1 <= p2; p1++)
			portmask |= (1 << p1);
	}
	if (!debug)
		return 0;

	fprintf(stderr, "%s: working on ports:", prgname);
	for_each_port(p1)
		printf(" %i", p1);
	printf("\n");
	return 0;
}

static void exit_mask(int present)
{
	if (present)
		fprintf(stderr, "%s: can't set mask twice from cmdline\n",
			prgname);
	else
		fprintf(stderr, "%s: please set port mask before config\n",
			prgname);
	exit(1);
}


int main(int argc, char *argv[])
{
	int c, i, mask_ok = 0;

	prgname = argv[0];

	if (NPORTS > 8 * sizeof(portmask)) {
		/* build error: too big maxports */
		static __attribute__((used)) int
			array[8 * sizeof(portmask) - NPORTS];
	}


	rtud_ch = minipc_client_create("rtud", 0);
	if(!rtud_ch) {
		fprintf(stderr, "%s: Can't connect to RTUd mini-rpc server\n",
			prgname);
		return -1;
	}

	/*parse parameters*/
	while( (c = getopt_long(argc, argv, "h", ropts, NULL)) != -1) {
		switch(c) {
			case OPT_EP_PORT:
				//port number
				if (mask_ok)
					exit_mask(mask_ok);
				if (parse_mask(optarg) < 0) {
					fprintf(stderr, "%s: wrong port mask "
						"\"%s\"\n", prgname, optarg);
					exit(1);
				}
				mask_ok = 1;

				break;
			case OPT_EP_QMODE:
				if (!mask_ok)
					exit_mask(mask_ok);

				//qmode for port
				for_each_port(i) {
					vlans[i].qmode = atoi(optarg);
					vlans[i].valid_mask |= VALID_QMODE;
				}
				break;
			case OPT_EP_PRIO:
				//priority value for port, forces fix_prio=1
				for_each_port(i) {
					vlans[i].prio_val = atoi(optarg);
					vlans[i].fix_prio = 1;
					vlans[i].valid_mask |= VALID_PRIO;
				}
				break;
			case OPT_EP_VID:
				//VID for port
				for_each_port(i) {
					vlans[i].vid = atoi(optarg);
					vlans[i].valid_mask |= VALID_VID;
				}
				break;
			case OPT_EP_UMASK:
				//untag mask
				for_each_port(i) {
					vlans[i].untag_mask = (int) strtol(optarg, NULL, 16);
					vlans[i].valid_mask |= VALID_UNTAG;
				}
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

int print_help(char *prgname)
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

void print_config(struct s_port_vlans *vlans)
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

int apply_settings(struct s_port_vlans *vlans)
{
	int i;

	for_each_port(i) {
		printf("port %i\n", i);
		//TODO: call apropriate ioctls to configure tagging/untagging
	}

	config_rtud();

	return 0;
}

int config_rtud(void)
{
	struct rtu_vlans_t *cur;
	struct rtu_vlans_t *old_list, *old_entry;
	int ret, val;

	old_list = rtu_retrieve_config();

	cur = rtu_vlans;
	while(cur) {
		old_entry = rtu_find_vlan(old_list, cur->vid, cur->flags & VALID_FID?cur->fid:-1);
		/*preserve previous settings if not overwritten*/
		if(old_entry!=NULL && !(cur->flags & VALID_FID))
			cur->fid = old_entry->fid;
		if(old_entry!=NULL && !(cur->flags & VALID_PMASK))
			cur->pmask = old_entry->pmask;
		if(old_entry!=NULL && !(cur->flags & VALID_DROP))
			cur->drop = old_entry->drop;
		if(old_entry!=NULL && !(cur->flags & VALID_PRIO)) {
			cur->prio = old_entry->prio;
			cur->has_prio = old_entry->has_prio;
			cur->prio_override = old_entry->prio_override;
		}

		ret = minipc_call(rtud_ch, MINIPC_TIMEOUT, &rtud_export_vlan_entry, &val,
				cur->vid, cur->fid, cur->pmask, cur->drop, cur->prio, cur->has_prio,
				cur->prio_override);
		cur = cur->next;
	}

	free_rtu_vlans(old_list);

	return 0;
}

void list_rtu_vlans(void)
{
	rtudexp_vd_list_t vlist;
	rtudexp_vd_entry_t *ventry;
	int idx = 0, i;

	printf("  VID    FID       MASK       DROP    PRIO    PRIO_OVERRIDE\n");
	printf("-----------------------------------------------------------\n");
	do {
		if (minipc_call(rtud_ch, MINIPC_TIMEOUT,
				&rtud_export_get_vd_list, &vlist, idx) < 0) {
			fprintf(stderr, "%s: minipc_call: %s\n", prgname, strerror(errno));
			return;
		}
		for(i=0; i<vlist.num_entries; ++i) {
			ventry = &vlist.list[i];
			printf("%4d   %4d      0x%8x    ", ventry->vid, ventry->fid, ventry->port_mask);
			if(ventry->drop == 0)     printf("NO ");
			else                      printf("YES");
			if(ventry->has_prio == 0) printf("     --    ");
			else                      printf("     %1d    ", ventry->prio);

			if(ventry->prio_override == 0) printf("     NO ");
			else                           printf("     YES ");
			printf("\n");
		}
		idx = vlist.next;
	} while(idx>0);
	printf("\n");
}

int clear_all()
{
	rtudexp_vd_list_t vlist;
	rtudexp_vd_entry_t *ventry;
	int idx = 0, i, val;

	do {
		if (minipc_call(rtud_ch, MINIPC_TIMEOUT,
				&rtud_export_get_vd_list, &vlist, idx) < 0) {
			/* Duplicated from above */
			fprintf(stderr, "%s: minipc_call: %s\n", prgname, strerror(errno));
			return -1;
		}

		/*remove vlans from the list*/
		for(i=0; i<vlist.num_entries; ++i) {
			ventry = &vlist.list[i];
			if(ventry->vid != 0)
				minipc_call(rtud_ch, MINIPC_TIMEOUT, &rtud_export_vlan_entry, &val, ventry->vid,
						ventry->fid, 0, 1, 0, 0, 0);
			else
				minipc_call(rtud_ch, MINIPC_TIMEOUT, &rtud_export_vlan_entry, &val, 0, 0,
						0xffffffff, 0, 0, 0, 0);
		}
		idx = vlist.next;
	} while(idx>0);

	/*TODO: cancel tagging/untagging in all endpoints*/

	return 0;
}

int set_rtu_vlan(int vid, int fid, int pmask, int drop, int prio, int del, int flags)
{
	static struct rtu_vlans_t *cur;

	if ( vid > 0 && rtu_vlans == NULL) {
		/* allocate first element of the list */
		rtu_vlans = (struct rtu_vlans_t*) malloc( sizeof(struct rtu_vlans_t) );
		if (rtu_vlans == NULL) {
			fprintf(stderr, "Could not allocate rtu_vlans\n");
			return -1;
		}
		cur = rtu_vlans;
	}
	else if( vid > 0 ) {
		/* allocate next element of the list */
		cur->next = (struct rtu_vlans_t*) malloc( sizeof(struct rtu_vlans_t) );
		if (cur->next == NULL) {
			fprintf(stderr, "Could not allocate next rtu_vlans\n");
			return -1;
		}
		cur = cur->next;
	}

	if(vid > 0) {
		bzero(cur, sizeof(struct rtu_vlans_t));
		cur->vid = vid;
		cur->fid = vid;
		cur->flags |= VALID_VID;
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

void free_rtu_vlans(struct rtu_vlans_t *ptr)
{
	struct rtu_vlans_t *next = NULL;

	while(ptr) {
		next = ptr->next;
		free(ptr);
		ptr = next;
	}
}

struct rtu_vlans_t* rtu_retrieve_config(void)
{
	rtudexp_vd_list_t vlist;
	rtudexp_vd_entry_t *ventry;
	int idx = 0, i;
	struct rtu_vlans_t *config = NULL, *cur = NULL, *ptr;

	do {
		if (minipc_call(rtud_ch, MINIPC_TIMEOUT,
				&rtud_export_get_vd_list, &vlist, idx) < 0) {
			/* Duplicated from above */
			fprintf(stderr, "%s: minipc_call: %s\n", prgname, strerror(errno));
			return config; /* maybe partly good */
		}

		for(i=0; i<vlist.num_entries; ++i) {
			ptr = malloc(sizeof(struct rtu_vlans_t));
			if (!ptr) {
				fprintf(stderr, "%s: reading RTUd table: %s\n",
					prgname, strerror(errno));
				return config; /* may be partly good */
			}

			ventry = &vlist.list[i];

			if(config == NULL) { /* first item */
				config = cur = ptr;
			} else {
				cur->next = ptr;
				cur = ptr;
			}
			cur->vid = ventry->vid;
			cur->fid = ventry->fid;
			cur->pmask = ventry->port_mask;
			cur->drop  = ventry->drop;
			cur->prio  = ventry->prio;
			cur->has_prio = ventry->has_prio;
			cur->prio_override = ventry->prio_override;
		}
		idx = vlist.next;
	} while(idx>0);

	return config;
}

struct rtu_vlans_t* rtu_find_vlan(struct rtu_vlans_t *conf, int vid, int fid)
{
	struct rtu_vlans_t *cur;

	cur = conf;
	while(cur) {
		if((cur->vid == vid && cur->fid == fid) || (cur->vid == vid && fid == -1))
			break;
		cur = cur->next;
	}

	return cur;
}
