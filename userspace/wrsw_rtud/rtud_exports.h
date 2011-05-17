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

void rtudexp_get_fd_list(rtudexp_fd_list_t *list, int start_from);

#endif
