/*
 * rtu_stat:
 *
 * Give detail on the Routing-Table-Unit (RTU) and permit to modify it
 *
 *  Modified on: Oct 30, 2012
 *  Authors:
 *  	- Tomasz Wlostowski (tomasz.wlostowski@cern.ch)
 * 		- Benoit RAT (benoit<AT>sevensols.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License...
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libwr/util.h>

#include <libwr/shmem.h>
#include <libwr/hal_shmem.h>

#include <minipc.h>
#include <rtud_exports.h>
#include <mac.h>

#define MINIPC_TIMEOUT 200

static struct minipc_ch *rtud_ch;

// forwarding entries
void rtudexp_get_fd_list(rtudexp_fd_list_t *list, int start_from)
{
	minipc_call(rtud_ch, MINIPC_TIMEOUT, &rtud_export_get_fd_list, list,
			start_from);
}

// vlan entries
void rtudexp_get_vd_list(rtudexp_vd_list_t *list, int current)
{
	minipc_call(rtud_ch, MINIPC_TIMEOUT, &rtud_export_get_vd_list, list,
			current);
}

int rtudexp_clear_entries(int netif, int force)
{
	int val, ret;
	ret = minipc_call(rtud_ch, MINIPC_TIMEOUT, &rtud_export_clear_entries,&val,netif,force);
	return (ret<0)?ret:val;
}

int rtudexp_add_entry(const char *eha, int port, int mode)
{
	int val, ret;
	ret = minipc_call(rtud_ch, MINIPC_TIMEOUT, &rtud_export_add_entry,&val,eha,port,mode);
	return (ret<0)?ret:val;
}

int rtudexp_vlan_entry(int vid, int fid, const char *ch_mask, int drop, int prio, int has_prio,
		        int prio_override)
{
	int val, ret;
	int mask;
	sscanf(ch_mask,"%x", &mask);
	ret = minipc_call(rtud_ch, MINIPC_TIMEOUT, &rtud_export_vlan_entry,&val,vid,fid,mask,
			    drop,prio,has_prio,prio_override);
	return (ret<0)?ret:val;
}

#define RTU_MAX_ENTRIES 8192
#define NUM_VLANS       4096

void fetch_rtu_fd(rtudexp_fd_entry_t *d, int *n_entries)
{
	int start = 0, n = 0;
	rtudexp_fd_list_t list;

	do {
		rtudexp_get_fd_list(&list, start);
		//	printf("num_rules %d\n", list.num_rules);

		memcpy( d+n, list.list, sizeof(rtudexp_fd_entry_t) * list.num_rules);
		start=list.next;
		n+=list.num_rules;
	} while(start > 0);

	//	printf("%d rules \n", n);
	*n_entries = n;
}

int fetch_rtu_vd(rtudexp_vd_entry_t *d, int *n_entries)
{
	int start = 0, n = 0;
	rtudexp_vd_list_t list;

	do {
		rtudexp_get_vd_list(&list, start);
		memcpy( d+n, list.list, sizeof(rtudexp_vd_entry_t) * list.num_entries);
		start=list.next;
		n+=list.num_entries;
	} while(start > 0);
	*n_entries = n;
	return 0;
}

/**
 * \brief Write mac address into a buffer to avoid concurrent access on static variable.
 */
//TODO: already defined in wrsw_rtud lib but we do not link to it. 3 opts: make it inline, move to libwr or link to the rtud lib?
char *mac_to_buffer(uint8_t mac[ETH_ALEN],char buffer[ETH_ALEN_STR])
{
 	if(mac && buffer)
 		snprintf(buffer, ETH_ALEN_STR, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    return buffer;
}


static int cmp_entries(const void *p1, const void *p2)
{
	rtudexp_fd_entry_t *e1 = (rtudexp_fd_entry_t *)p1;
	rtudexp_fd_entry_t *e2 = (rtudexp_fd_entry_t *)p2;


	return memcmp(e1->mac, e2->mac, 6);
	//           return strcmp(* (char * const *) p1, * (char * const *) p2);
}

char *decode_ports(int dpm, int nports)
{
	static char str[80],str2[20];
	int i;

	if ((dpm & ((1<<nports)-1)) == ((1<<nports)-1))
	{
		strcpy(str,"ALL");
		return str;
	}
	strcpy(str,"");

	for (i = 0; i < nports; i++)
	{
		sprintf(str2,"%d ", i);
		if(dpm&(1<<i)) strcat(str,str2);
	}

	if (dpm & (1<<nports))
		strcat(str, "CPU");

	return str;
}

void show_help(char *prgname)
{
	fprintf(stderr, "usage: %s <command> <values>\n", prgname);
	fprintf(stderr,""
			"   help:             Show this message\n"
			"   list:             List the routing table (same as empty command)\n"
			"   remove <ifnum> [<force>]: Remove all dynamic entries for one interface\n"
			"   add    <mac (XX:XX:XX:XX:XX)> <ifnum> [<mode>]: Add entry for a specific \n"
			"                     MAC address (mode={1=dynamic,0=static}\n"
			"   vlan   <vid> <fid> <hex mask> [<drop>, <prio>, <has_prio>, <prio_override>]: \n"
			"                    Add VLAN entry with vid, fid, mask and drop flag (Write mask=0x0 \n"
			"                    and drop=1 to remove the VLAN)\n");

	exit(1);
}

int get_nports_from_hal(void)
{
	struct hal_shmem_header *h;
	struct wrs_shm_head *hal_head;
	int hal_nports_local; /* local copy of number of ports */

	hal_head = wrs_shm_get(wrs_shm_hal, "", WRS_SHM_READ);
	if (!hal_head) {
		fprintf(stderr, "unable to open shm for HAL!\n");
		exit(-1);
	}
	h = (void *)hal_head + hal_head->data_off;
	/* Assume number of ports does not change in runtime */
	hal_nports_local = h->nports;
	if (hal_nports_local > HAL_MAX_PORTS) {
		fprintf(stderr, "Too many ports reported by HAL. "
			"%d vs %d supported\n",
			hal_nports_local, HAL_MAX_PORTS);
		exit(-1);
	}
	return hal_nports_local;
}

int main(int argc, char **argv)
{

	rtudexp_fd_entry_t fd_list[RTU_MAX_ENTRIES];
	rtudexp_vd_entry_t vd_list[NUM_VLANS];

	int n_fd_entries, n_vd_entries;
	int i, isok;
	int nports;

	nports = get_nports_from_hal();

	rtud_ch = minipc_client_create("rtud", 0);

	if(!rtud_ch)
	{
		printf("Can't connect to RTUd mini-rpc server\n");
		return -1;
	}
	minipc_set_logfile(rtud_ch,stderr);
	isok=0;
	if(argc>1)
	{
		if(strcmp(argv[1], "remove")==0)
		{
			i=atoidef(argv[2],-1);
			if((0 <= i && i < 18) && (rtudexp_clear_entries(i,atoidef(argv[3],0))==0))	isok=1;
			else printf("Could not %s entry for wr%d\n",argv[1],i);
		}
		else if(strcmp(argv[1], "add")==0)
		{
			if((argc > 3) && (rtudexp_add_entry(argv[2],atoi(argv[3]),atoidef(argv[4],0))==0)) isok=1;
			else printf("Could not %s entry for %s\n",argv[2],argv[3]);
		}
		else if(strcmp(argv[1], "vlan")==0)
		{
			if((argc > 3 ) && (rtudexp_vlan_entry(atoi(argv[2]),atoi(argv[3]),argv[4],
					    atoidef(argv[5],0),atoidef(argv[6],0),atoidef(argv[7],0),
					    atoidef(argv[8],0))==0))  isok=1;
			else printf("Could not %s entry for %s\n",argv[2],argv[3]);
			exit(1);
		}
		else if(strcmp(argv[1], "list")==0) isok=1;

		//Does not continue
		if(!isok) show_help(argv[0]);


	}

	fetch_rtu_fd(fd_list, &n_fd_entries);

	qsort(fd_list, n_fd_entries,  sizeof(rtudexp_fd_entry_t), cmp_entries);

	printf("RTU Filtering Database Dump: %d rules\n", n_fd_entries);
	printf("\n");
	printf("MAC                     Dst.ports      FID          Type               Age [s]\n");
	printf("----------------------------------------------------------------------------------\n");

	char mac_buf[ETH_ALEN_STR];

	for(i=0;i<n_fd_entries;i++)
	{
		printf("%-25s %-12s %2d          %s (hash %03x:%x)   ",
			mac_to_buffer(fd_list[i].mac,mac_buf),
			decode_ports(fd_list[i].dpm, nports),
			fd_list[i].fid,
			fd_list[i].dynamic ? "DYNAMIC":"STATIC ",
			fd_list[i].hash,
			fd_list[i].bucket);
		if(fd_list[i].dynamic)
			printf("%d\n", fd_list[i].age);
		else
			printf("-\n");
	}
	printf("\n");

	fetch_rtu_vd(vd_list, &n_vd_entries);

		printf("RTU VLAN Table Dump: %d active VIDs defined\n", n_vd_entries);
	printf("\n");
	printf("  VID    FID       MASK       DROP    PRIO    PRIO_OVERRIDE\n");
	printf("-----------------------------------------------------------\n");

	for(i=0;i<n_vd_entries;i++)
	{
		printf("%4d   %4d      0x%8x    ", vd_list[i].vid, vd_list[i].fid, vd_list[i].port_mask);
		if(vd_list[i].drop == 0)     printf("NO ");
		else                         printf("YES");
		if(vd_list[i].has_prio == 0) printf("     --    ");
		else                         printf("     %1d    ",vd_list[i].prio);

		if(vd_list[i].prio_override == 0) printf("     NO ");
		else                              printf("     YES ");
		printf("\n");
	}
	printf("\n");
	return 0;
}
