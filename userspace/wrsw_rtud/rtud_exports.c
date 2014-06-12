/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_rtud v1.0
 *
 * Authors:     Tomasz Wlostowski (tomasz.wlostowski@cern.ch)
 *
 * Description: Dump the filtering database. Based on libwripc
 *
 * Fixes:
 * 		Benoit RAT
 *
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <trace.h>

#include "minipc.h"
#include "rtu.h"
#include "rtu_fd.h"
#include "rtud_exports.h"
#include "mac.h"

#include <hal_client.h>

/* The channel */
static struct minipc_ch *rtud_ch;

/* Define shortcut */
#define MINIPC_EXP_FUNC(stru,func) stru.f= func; \
		if (minipc_export(rtud_ch, &stru) < 0) { TRACE(TRACE_FATAL,"Could not export %s (rtu_ch=%d)",stru.name,rtud_ch); }

/* The exported function */
int rtudexp_get_fd_list(const struct minipc_pd *pd, uint32_t * args, void *ret)
{
	int i;
	rtudexp_fd_list_t *list = ret;
	int start_from = args[0];

	TRACE(TRACE_INFO, "GetFDList start=%d", start_from);

	for (i = 0; i < 8; i++) {
		struct filtering_entry *ent =
		    rtu_fd_lookup_htab_entry(start_from + i);
		if (!ent)
			break;

		memcpy(list->list[i].mac, ent->mac, sizeof(ent->mac));
		//printf("Ent: %s %x\n", mac_to_string(ent->mac), ent->port_mask_dst);

		list->list[i].dpm = ent->port_mask_dst;
		list->list[i].spm = ent->port_mask_src;
		list->list[i].priority = 0;
		list->list[i].dynamic = ent->dynamic;
		list->list[i].hash = ent->addr.hash;
		list->list[i].bucket = ent->addr.bucket;
		list->list[i].age = ent->age;
		list->list[i].fid = ent->fid;
	}

	list->num_rules = i;
	list->next = (i < 8 ? 0 : start_from + i);
	return 0;
}

/* The exported vlan */
int rtudexp_get_vd_list(const struct minipc_pd *pd, uint32_t * args, void *ret)
{
	int i = 0;
	rtudexp_vd_list_t *list = ret;
	int current = args[0];

	TRACE(TRACE_INFO, "GetVDList start=%d", current);

	do {
		struct vlan_table_entry *ent = rtu_vlan_entry_get(current);
		if (!ent)
			break;

		if (ent->drop == 0 || ent->port_mask != 0x0) {
			list->list[i].vid = current;
			list->list[i].port_mask = ent->port_mask;
			list->list[i].drop = ent->drop;
			list->list[i].fid = ent->fid;
			list->list[i].has_prio = ent->has_prio;
			list->list[i].prio_override = ent->prio_override;
			list->list[i].prio = ent->prio;
			TRACE(TRACE_INFO,
			      "vlan_entry_vd: vid %d, drop=%d, fid=%d, port_mask 0x%x",
			      list->list[i].vid, list->list[i].drop,
			      list->list[i].fid, list->list[i].port_mask);
			i++;
		}
		current++;

		if (current == NUM_VLANS)
			break;

	} while (i < 8);

	list->num_entries = i;
	list->next = (i < 8 ? 0 : current);
	return 0;
}

int rtudexp_clear_entries(const struct minipc_pd *pd,
			  uint32_t * args, void *ret)
{
	int iface_num = (int)args[0];
	int force = (int)args[1];
	int *p_ret = (int *)ret;	//force pointed to int type

	TRACE(TRACE_INFO, "iface=%d, force=%d", iface_num, force);

	//Do nothing
	if (force)
		TRACE(TRACE_INFO, "wr%d > force %d is not implemented",
		      iface_num, force);

	rtu_fd_clear_entries_for_port(iface_num);
	*p_ret = 0;
	return *p_ret;
}

int rtudexp_add_entry(const struct minipc_pd *pd, uint32_t * args, void *ret)
{
	uint8_t mac_tmp[ETH_ALEN] = { 0 };
	hexp_port_list_t ports;

	char *strEHA;
	int port, mode;
	int *p_ret = (int *)ret;	//force pointed to int type

	strEHA = (char *)args;
	args = minipc_get_next_arg(args, pd->args[0]);
	port = (int)args[0];
	mode = (int)args[1];

	//TRACE(TRACE_INFO,"iface=%s, port=%d, dynamic=%d",strEHA,port,mode);

	halexp_query_ports(&ports);

	if (mac_from_str(mac_tmp, strEHA) != ETH_ALEN)
		TRACE(TRACE_ERROR,
		      "%s is an invalid MAC format (XX:XX:XX:XX:XX:XX)",
		      strEHA);

	TRACE(TRACE_INFO, "Create entry for (MAC=%s) port %x, mode:%s",
	      mac_to_string(mac_tmp), 1 << port, (mode) ? "DYNAMIC" : "STATIC");
	*p_ret =
	    rtu_fd_create_entry(mac_tmp, 0, 1 << port, mode, OVERRIDE_EXISTING);
	return *p_ret;
}

int rtudexp_vlan_entry(const struct minipc_pd *pd, uint32_t * args, void *ret)
{
	int vid, fid, mask, drop, prio, has_prio, prio_override;
	int *p_ret = (int *)ret;	//force pointed to int type
	*p_ret = 0;

	vid = (int)args[0];
	fid = (int)args[1];
	mask = (int)args[2];
	drop = (int)args[3];
	prio = (int)args[4];
	has_prio = (int)args[5];
	prio_override = (int)args[6];
	rtu_fd_create_vlan_entry(vid, (uint32_t) mask, (uint8_t) fid,
				 (uint8_t) prio, has_prio, prio_override, drop);
	return *p_ret;
}

int rtud_init_exports()
{
	rtud_ch = minipc_server_create("rtud", 0);

	if (!rtud_ch < 0)
		return -1;

	TRACE(TRACE_INFO, "wripc server created [fd %d]",
	      minipc_fileno(rtud_ch));

	MINIPC_EXP_FUNC(rtud_export_get_fd_list, rtudexp_get_fd_list);
	MINIPC_EXP_FUNC(rtud_export_get_vd_list, rtudexp_get_vd_list);
	MINIPC_EXP_FUNC(rtud_export_clear_entries, rtudexp_clear_entries);
	MINIPC_EXP_FUNC(rtud_export_add_entry, rtudexp_add_entry);
	MINIPC_EXP_FUNC(rtud_export_vlan_entry, rtudexp_vlan_entry);

	return 0;
}

void rtud_handle_wripc()
{
	minipc_server_action(rtud_ch, 100);
}
