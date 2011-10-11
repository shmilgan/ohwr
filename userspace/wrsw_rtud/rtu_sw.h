/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_rtud v2.0
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *              Miguel Baizan   (miguel.baizan@integrasys.es)
 *              Maciej Lipinski (maciej.lipinski@cern.ch)
 *
 * Description: RTU Filtering Database Mirror.
 *              Filtering database management related operations and filtering
 *              database mirror. Note there is a single Filtering Database
 *              object per Bridge (See 802.1Q - 12.7.1)
 *
 * Fixes:
 *              Alessandro Rubini
 *              Tomasz Wlostowski
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

#ifndef __WHITERABBIT_RTU_SW_H
#define __WHITERABBIT_RTU_SW_H

#include "rtu.h"

int rtu_sw_find_entry(
        uint8_t mac[ETH_ALEN],
        uint8_t fid,
        struct filtering_entry **fe);

int rtu_sw_create_entry(
        struct filtering_entry **fe,
        uint8_t  mac[ETH_ALEN],
        uint16_t vid,
        uint32_t port_map,
        uint32_t use_dynamic,
        int dynamic);

void rtu_sw_update_entry(struct filtering_entry *fe);

void rtu_sw_delete_entry(struct filtering_entry *fe);

void rtu_sw_clean_fd(void);

struct vlan_table_entry *rtu_sw_find_vlan_entry(uint16_t vid);

struct vlan_table_entry *rtu_sw_find_next_ve(uint16_t *vid);

int rtu_sw_create_vlan_entry(
        uint16_t vid,
        uint8_t fid,
        uint32_t port_mask,
        uint32_t use_dynamic,
        uint32_t untagged_set,
        int dynamic);

int rtu_sw_delete_vlan_entry(uint16_t vid);

void rtu_sw_clean_vd(void);

int rtu_sw_get_num_vlans(void);

int rtu_sw_fid_shared(uint8_t fid);

void rtu_sw_clean_aging_map(void);
void rtu_sw_update_aging_map(void);
void rtu_sw_update_fd_age(void);

void rtu_sw_cache(void);
void rtu_sw_uncache(void);
void rtu_sw_commit(void);
void rtu_sw_rollback(void);

uint16_t rtu_sw_get_next_vid(uint16_t vid);
#endif /*__WHITERABBIT_RTU_SW_H*/
