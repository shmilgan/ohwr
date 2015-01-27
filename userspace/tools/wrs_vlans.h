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

#ifndef __WR_VLANS_H__
#define __WR_VLANS_H__

#define NPORTS 18

#define QMODE_ACCESS 0
#define QMODE_TRUNK	 1
#define QMODE_DISABLED 2
#define QMODE_UNQ 3

#define MINIPC_TIMEOUT 200

#define VALID_CONFIG 1<<31
#define VALID_QMODE  1<<0
#define VALID_PRIO	 1<<1
#define VALID_VID	   1<<2
#define VALID_FID    1<<3
#define VALID_UNTAG  1<<4
#define VALID_PMASK  1<<5
#define VALID_DROP   1<<6

#define PORT_MASK(x) (1<<(x))

#define NOPTS 15
#define OPT_HELP 	'h'
#define OPT_DEBUG 'd'
#define OPT_CLEAR 3
#define OPT_LIST 4
#define OPT_EP_PORT  10
#define OPT_EP_QMODE 11
#define OPT_EP_VID   12
#define OPT_EP_PRIO  13
#define OPT_EP_UMASK 14
#define OPT_EP_LIST  15
#define OPT_RTU_VID  20
#define OPT_RTU_FID  21
#define OPT_RTU_PMASK 22
#define OPT_RTU_DROP  23
#define OPT_RTU_PRIO  24
#define OPT_RTU_DEL   25

struct s_port_vlans
{
	int  valid_mask;
	char qmode;
	char fix_prio;
	char prio_val;
	int  vid;
	int  untag_mask;
};

struct rtu_vlans_t
{
	int vid;
	int fid;
	int pmask;
	int drop;
	int prio;
	int has_prio;
	int prio_override;
	int flags;
	struct rtu_vlans_t *next;
};

#endif
