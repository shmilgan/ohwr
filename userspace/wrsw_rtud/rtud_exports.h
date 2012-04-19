/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_rtud v1.0
 *
 * Authors:     Tomasz Wlostowski (tomasz.wlostowski@cern.ch)
 *
 * Description: Dump the filtering database.
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


#ifndef __RTUD_EXPORTS_H
#define __RTUD_EXPORTS_H

#include <stdint.h>
#include <minipc.h>

typedef struct
{
		uint8_t mac[8];
		uint32_t dpm;
		uint32_t spm;
		uint8_t priority;
		int dynamic;
} rtudexp_fd_entry_t;

typedef struct  {
	rtudexp_fd_entry_t list[8];

	int num_rules;
	int next;
} rtudexp_fd_list_t;

/* Export this function: it returns a structure */
struct minipc_pd rtud_export_get_fd_list = {
	.name = "get_fd_list",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
				    rtudexp_fd_list_t),
	.args = {
		MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
		MINIPC_ARG_END,
	},
};

#endif
