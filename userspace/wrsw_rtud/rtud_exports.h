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

typedef struct {
	uint8_t mac[8];
	uint32_t dpm;
	uint32_t spm;
	uint8_t priority;
	int dynamic;
	uint16_t hash;
	int bucket;
	int age;
	int fid;
} rtudexp_fd_entry_t;

typedef struct {
	rtudexp_fd_entry_t list[8];

	int num_rules;
	int next;
} rtudexp_fd_list_t;

///// VLAN export
typedef struct {
	int vid;
	uint32_t port_mask;
	uint8_t fid;
	uint8_t prio;
	int has_prio;
	int prio_override;
	int drop;
} rtudexp_vd_entry_t;

typedef struct {
	rtudexp_vd_entry_t list[8];

	int num_entries;
	int next;
} rtudexp_vd_list_t;

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
/* Export this function: it returns a structure */
struct minipc_pd rtud_export_get_vd_list = {
	.name = "get_vd_list",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
				    rtudexp_vd_list_t),
	.args = {
		 MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
		 MINIPC_ARG_END,
		 },
};

/* Export of a function to set remove entry in rtu */
struct minipc_pd rtud_export_clear_entries = {
	.name = "clear_entries",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
	.args = {
		 MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
		 MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
		 MINIPC_ARG_END,
		 },
};

/* Export of a function to add entry in rtu */
struct minipc_pd rtud_export_add_entry = {
	.name = "add_entry",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
	.args = {
		 MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRING, char *),
		 MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
		 MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
		 MINIPC_ARG_END,
		 },
};

/* Export of a function to add vlan entry in rtu */
struct minipc_pd rtud_export_vlan_entry = {
	.name = "vlan_entry",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
	.args = {
		 MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),	// 0: vid
		 MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),	// 1: fid
		 MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),	// 2: mask
		 MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),	// 3: drop
		 MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),	// 4: prio
		 MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),	// 5: has_prio
		 MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),	// 6: prio_override
		 MINIPC_ARG_END,
		 },
};

#endif
