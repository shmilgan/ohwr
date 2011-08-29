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
 * Description: RTU Filtering database header.
 *              Filtering database management related operations and filtering
 *              database mirror. Note there is a single Filtering Database
 *              object per Bridge (See 802.1Q - 12.7.1)
 *
 * Fixes:
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


#ifndef __WHITERABBIT_RTU_FD_H
#define __WHITERABBIT_RTU_FD_H

#include "rtu.h"

// Static filtering entries may be active or inactive. Only active entries
// are used to compute the forward vector for a FDB entry.
#define NOT_IN_SERVICE  0
#define ACTIVE          1

int rtu_fdb_init(uint16_t poly, unsigned long aging)
        __attribute__((warn_unused_result));

// FDB

int rtu_fdb_create_dynamic_entry(
            uint8_t mac[ETH_ALEN],
            uint16_t vid,
            uint32_t port_map
     ) __attribute__((warn_unused_result));

int rtu_fdb_read_entry(
           uint8_t mac[ETH_ALEN],
           uint8_t fid,
           uint32_t *port_map,
           int *entry_type
     ) __attribute__((warn_unused_result));

int rtu_fdb_read_next_entry(
           uint8_t (*mac)[ETH_ALEN],                            // inout
           uint8_t *fid,                                        // inout
           uint32_t *port_map,                                  // out
           int *entry_type                                      // out
     ) __attribute__((warn_unused_result));

// Static FDB

int  rtu_fdb_create_static_entry(
            uint8_t mac[ETH_ALEN],
            uint16_t vid,
            enum filtering_control port_map[NUM_PORTS],
            enum storage_type type,
            int active
     ) __attribute__((warn_unused_result));

int rtu_fdb_delete_static_entry(
            uint8_t mac[ETH_ALEN],
            uint16_t vid
     ) __attribute__((warn_unused_result));

int rtu_fdb_read_static_entry(
            uint8_t mac[ETH_ALEN],
            uint16_t vid,
            enum filtering_control (*port_map)[NUM_PORTS],      // out
            enum storage_type *type,                            // out
            int *active                                         // out
     ) __attribute__((warn_unused_result));

int rtu_fdb_read_next_static_entry(
            uint8_t (*mac)[ETH_ALEN],                           // inout
            uint16_t *vid,                                      // inout
            enum filtering_control (*port_map)[NUM_PORTS],      // out
            enum storage_type *type,                            // out
            int *active                                         // out
     ) __attribute__((warn_unused_result));

// VLAN

int rtu_fdb_create_static_vlan_entry(
            uint16_t vid,
            uint8_t fid,
            enum registrar_control member_set[NUM_PORTS],
            uint32_t untagged_set
     ) __attribute__((warn_unused_result));

int rtu_fdb_delete_static_vlan_entry(
            uint16_t vid
     ) __attribute__((warn_unused_result));

int rtu_fdb_read_static_vlan_entry(
            uint16_t vid,
            enum registrar_control (*member_set)[NUM_PORTS],    // out
            uint32_t *untagged_set                              // out
     ) __attribute__((warn_unused_result));

int rtu_fdb_read_next_static_vlan_entry(
            uint16_t *vid,                                      // inout
            enum registrar_control (*member_set)[NUM_PORTS],    // out
            uint32_t *untagged_set                              // out
     ) __attribute__((warn_unused_result));

int rtu_fdb_read_vlan_entry(
            uint16_t vid,
            uint8_t *fid,                                       // out
            int *entry_type,                                    // out
            enum registrar_control (*member_set)[NUM_PORTS],    // out
            uint32_t *untagged_set,                             // out
            unsigned long *creation_t                           // out
     ) __attribute__((warn_unused_result));


int rtu_fdb_read_next_vlan_entry(
            uint16_t *vid,                                      // inout
            uint8_t *fid,                                       // out
            int *entry_type,                                    // out
            enum registrar_control (*member_set)[NUM_PORTS],    // out
            uint32_t *untagged_set,                             // out
            unsigned long *creation_t                           // out
     ) __attribute__((warn_unused_result));

// Config

void rtu_fdb_set_hash_poly(uint16_t poly);

void rtu_fdb_age_dynamic_entries(void);

int  rtu_fdb_set_aging_time(
            uint8_t fid,
            unsigned long t
    ) __attribute__((warn_unused_result));

unsigned long rtu_fdb_get_aging_time(uint8_t fid);

uint16_t rtu_fdb_get_num_vlans(void);
uint16_t rtu_fdb_get_max_supported_vlans(void);
uint16_t rtu_fdb_get_max_vid(void);
uint16_t rtu_fdb_get_next_fid(uint8_t fid);

// Statistics

uint16_t rtu_fdb_get_num_dynamic_entries(uint8_t fid);
uint32_t rtu_fdb_get_num_learned_entry_discards(uint8_t fid);
uint64_t rtu_fdb_get_num_vlan_deletes(void);

#endif /*__WHITERABBIT_RTU_FD_H*/
