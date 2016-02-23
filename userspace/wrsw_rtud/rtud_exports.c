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

#include <libwr/wrs-msg.h>

#include "minipc.h"
#include "rtu.h"
#include "rtu_fd.h"
#include "rtud_exports.h"
#include "mac.h"

/* The channel */
static struct minipc_ch *rtud_ch;

/* Define shortcut */
#define MINIPC_EXP_FUNC(stru,func) stru.f= func; \
		if (minipc_export(rtud_ch, &stru) < 0) { \
			pr_error("Could not export %s (rtu_ch=%p)\n",stru.name,rtud_ch); }

int rtudexp_clear_entries(const struct minipc_pd *pd,
			  uint32_t * args, void *ret)
{
	int iface_num = (int)args[0];
	int *p_ret = (int *)ret;	//force pointed to int type

	pr_debug("Removing dynamic entries on interface %d (wri%d)\n",
		 iface_num + 1, iface_num + 1);

	rtu_fd_clear_entries_for_port(iface_num);
	*p_ret = 0;
	return *p_ret;
}

int rtudexp_add_entry(const struct minipc_pd *pd, uint32_t * args, void *ret)
{
	uint8_t mac_tmp[ETH_ALEN] = { 0 };

	char *strEHA;
	int port, mode;
	int *p_ret = (int *)ret;	//force pointed to int type

	strEHA = (char *)args;
	args = minipc_get_next_arg(args, pd->args[0]);
	port = (int)args[0];
	mode = (int)args[1];

	//pr_info("iface=%s, port=%d, dynamic=%d\n",strEHA,port,mode);

	if (mac_from_str(mac_tmp, strEHA) != ETH_ALEN)
		pr_error(
		      "%s is an invalid MAC format (XX:XX:XX:XX:XX:XX)\n",
		      strEHA);

	pr_info("Create entry for (MAC=%s) port %x (wri%d), mode:%s\n",
	      mac_to_string(mac_tmp), 1 << port, port + 1,
	      (mode) ? "DYNAMIC" : "STATIC");
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

	pr_debug("wripc server created [fd %d]\n",
	      minipc_fileno(rtud_ch));
	if (getenv("RTUD_MINIPC_DEBUG"))
		minipc_set_logfile(rtud_ch, stderr);

	MINIPC_EXP_FUNC(rtud_export_clear_entries, rtudexp_clear_entries);
	MINIPC_EXP_FUNC(rtud_export_add_entry, rtudexp_add_entry);
	MINIPC_EXP_FUNC(rtud_export_vlan_entry, rtudexp_vlan_entry);

	return 0;
}

void rtud_handle_wripc()
{
	minipc_server_action(rtud_ch, 100);
}
