/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_rtud v2.0
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: RTU Filtering Database Exported API.
 *              Definition of structs used by mini-ipc fdb server and proxy
 *
 * Fixes:
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


#ifndef __WHITERABBIT_RTU_FD_EXPORT_H
#define __WHITERABBIT_RTU_FD_EXPORT_H

#include "rtu_fd.h"

struct rtu_fdb_get_max_vid_argdata {
    // void
};

struct rtu_fdb_get_max_vid_retdata {
    uint16_t retval;
};


struct rtu_fdb_get_max_supported_vlans_argdata {
    // void
};

struct rtu_fdb_get_max_supported_vlans_retdata {
    uint16_t retval;
};


struct rtu_fdb_get_num_vlans_argdata {
    // void
};

struct rtu_fdb_get_num_vlans_retdata {
    uint16_t retval;
};


struct rtu_fdb_get_num_dynamic_entries_argdata {
    uint8_t fid;
};

struct rtu_fdb_get_num_dynamic_entries_retdata {
    uint16_t retval;
};


struct rtu_fdb_get_num_learned_entry_discards_argdata {
    uint8_t fid;
};

struct rtu_fdb_get_num_learned_entry_discards_retdata {
    uint32_t retval;
};


struct rtu_fdb_get_aging_time_argdata {
    uint8_t fid;
};

struct rtu_fdb_get_aging_time_retdata {
    unsigned long retval;
};


struct rtu_fdb_set_aging_time_argdata {
    uint8_t fid;
    unsigned long t;
};

struct rtu_fdb_set_aging_time_retdata {
    int retval;
};


struct rtu_fdb_get_num_vlan_deletes_argdata {
    // void
};

struct rtu_fdb_get_num_vlan_deletes_retdata {
    uint64_t retval;
};


struct rtu_fdb_read_entry_argdata {
    uint8_t mac[ETH_ALEN];
    uint8_t fid;
};

struct rtu_fdb_read_entry_retdata {
    int retval;
    uint32_t port_map;
    int entry_type;
};


struct rtu_fdb_read_next_entry_argdata {
    uint8_t mac[ETH_ALEN];
    uint8_t fid;
};

struct rtu_fdb_read_next_entry_retdata {
    int retval;
    uint8_t mac[ETH_ALEN];
    uint8_t fid;
    uint32_t port_map;
    int entry_type;
};


struct rtu_fdb_create_static_entry_argdata {
    uint8_t mac[ETH_ALEN];
    uint16_t vid;
    enum filtering_control port_map[NUM_PORTS];
    enum storage_type type;
    int active;
};

struct rtu_fdb_create_static_entry_retdata {
    int retval;
};

struct rtu_fdb_read_static_entry_argdata {
    uint8_t mac[ETH_ALEN];
    uint16_t vid;
};

struct rtu_fdb_read_static_entry_retdata {
    int retval;
    enum filtering_control port_map[NUM_PORTS];
    enum storage_type type;
    int active;
};


struct rtu_fdb_read_next_static_entry_argdata {
    uint8_t mac[ETH_ALEN];
    uint16_t vid;
};

struct rtu_fdb_read_next_static_entry_retdata {
    int retval;
    uint8_t mac[ETH_ALEN];
    uint16_t vid;
    enum filtering_control port_map[NUM_PORTS];
    enum storage_type type;
    int active;
};


struct rtu_fdb_delete_static_entry_argdata {
    uint8_t mac[ETH_ALEN];
    uint16_t vid;
};

struct rtu_fdb_delete_static_entry_retdata {
    int retval;
};

struct rtu_fdb_get_next_fid_argdata {
    uint8_t fid;
};

struct rtu_fdb_get_next_fid_retdata {
    uint16_t retval;
};

struct rtu_fdb_create_static_vlan_entry_argdata {
    uint16_t vid;
    uint8_t fid;
    enum registrar_control member_set[NUM_PORTS];
    uint32_t untagged_set;
};

struct rtu_fdb_create_static_vlan_entry_retdata {
    int retval;
};


struct rtu_fdb_delete_static_vlan_entry_argdata {
    uint16_t vid;
};

struct rtu_fdb_delete_static_vlan_entry_retdata {
    int retval;
};


struct rtu_fdb_read_static_vlan_entry_argdata {
    uint16_t vid;
};

struct rtu_fdb_read_static_vlan_entry_retdata {
    int retval;
    enum registrar_control member_set[NUM_PORTS];
    uint32_t untagged_set;
};


struct rtu_fdb_read_next_static_vlan_entry_argdata {
    uint16_t vid;
};

struct rtu_fdb_read_next_static_vlan_entry_retdata {
    int retval;
    uint16_t vid;
    enum registrar_control member_set[NUM_PORTS];
    uint32_t untagged_set;
};


struct rtu_fdb_read_vlan_entry_argdata {
    uint16_t vid;
};

struct rtu_fdb_read_vlan_entry_retdata {
    int retval;
    uint8_t fid;
    int entry_type;
    enum registrar_control member_set[NUM_PORTS];
    uint32_t untagged_set;
    unsigned long creation_t;
};


struct rtu_fdb_read_next_vlan_entry_argdata {
    uint16_t vid;
};

struct rtu_fdb_read_next_vlan_entry_retdata {
    int retval;
    uint16_t vid;
    uint8_t fid;
    int entry_type;
    enum registrar_control member_set[NUM_PORTS];
    uint32_t untagged_set;
    unsigned long creation_t;
};

#endif /*__WHITERABBIT_RTU_FD_EXPORT_H*/
