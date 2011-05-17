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

#include <wr_ipc.h>
#include <hw/trace.h>

#include "rtu.h"
#include "rtu_fd.h"
#include "rtud_exports.h"
#include "mac.h"

static int rtud_ipc;




int rtud_init_exports()
{
	rtud_ipc = wripc_create_server("rtud");

	if(rtud_ipc < 0)
		return rtud_ipc;

	TRACE(TRACE_INFO,"wripc server created [fd %d]", rtud_ipc);
	

	wripc_export(rtud_ipc, T_STRUCT(rtudexp_fd_list_t), "rtudexp_get_fd_list", rtudexp_get_fd_list, 1, T_INT32);

	return 0;
}

void rtudexp_get_fd_list(rtudexp_fd_list_t *list, int start_from)
{
	int i;
	
	//TRACE(TRACE_INFO,"GetFDList start=%d",start_from);
	
	for(i=0;i<8;i++)
	{
		struct filtering_entry *ent = rtu_fd_lookup_htab_entry(start_from + i);
		if(!ent) break;
		

		memcpy(list->list[i].mac, ent->mac, sizeof(ent->mac));
//		printf("Ent: %s %x\n", mac_to_string(ent->mac), ent->port_mask_dst);

		list->list[i].dpm = ent->port_mask_dst;
		list->list[i].spm = ent->port_mask_src;
		list->list[i].priority = 0;
		list->list[i].dynamic = ent->dynamic;
	}

	list->num_rules = i;
	list->next = (i < 8 ? 0 : start_from+i);
}

void rtud_handle_wripc()
{
	wripc_process(rtud_ipc);
}
