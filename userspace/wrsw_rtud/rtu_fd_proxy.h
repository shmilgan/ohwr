/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_rtud v2.0
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: RTU Filtering Database Proxy.
 *              Provides IPC access to the Filtering Database.
 *              Based on Alessandro Rubini & Tomasz Wlostowski mini_ipc framework.
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


#ifndef __WHITERABBIT_RTU_FD_PROXY_H
#define __WHITERABBIT_RTU_FD_PROXY_H

#include <errno.h>

#include "minipc.h"
#include "rtu.h"

#define MILLISEC_TIMEOUT 1000

int  rtu_fdb_proxy_create_static_entry(
            uint8_t mac[ETH_ALEN],
            uint16_t vid,
            uint32_t egress_ports,
            uint32_t forbidden_ports,
            int type,
            int active,
            int is_bpdu
     ) __attribute__((warn_unused_result));

int rtu_fdb_proxy_delete_static_entry(
            uint8_t mac[ETH_ALEN],
            uint16_t vid
     ) __attribute__((warn_unused_result));

int rtu_fdb_proxy_read_entry(
           uint8_t mac[ETH_ALEN],
           uint8_t fid,
           uint32_t *port_map,
           uint32_t *use_dynamic,
           int *entry_type
     ) __attribute__((warn_unused_result));

int rtu_fdb_proxy_read_next_entry(
           uint8_t (*mac)[ETH_ALEN],                            // inout
           uint8_t *fid,                                        // inout
           uint32_t *port_map,                                  // out
           uint32_t *use_dynamic,                               // out
           int *entry_type                                      // out
     ) __attribute__((warn_unused_result));

int rtu_fdb_proxy_read_static_entry(
            uint8_t mac[ETH_ALEN],
            uint16_t vid,
            uint32_t *egress_ports,                             // out
            uint32_t *forbidden_ports,                          // out
            int *type,                                          // out
            int *active                                         // out
     ) __attribute__((warn_unused_result));

int rtu_fdb_proxy_read_next_static_entry(
            uint8_t (*mac)[ETH_ALEN],                           // inout
            uint16_t *vid,                                      // inout
            uint32_t *egress_ports,                             // out
            uint32_t *forbidden_ports,                          // out
            int *type,                                          // out
            int *active                                         // out
     ) __attribute__((warn_unused_result));

int rtu_fdb_proxy_create_static_vlan_entry(
            uint16_t vid,
            uint32_t egress_ports,
            uint32_t forbidden_ports,
            uint32_t untagged_set
     ) __attribute__((warn_unused_result));

int rtu_fdb_proxy_delete_static_vlan_entry(
            uint16_t vid
     ) __attribute__((warn_unused_result));

int rtu_fdb_proxy_read_static_vlan_entry(
            uint16_t vid,
            uint32_t *egress_ports,                             // out
            uint32_t *forbidden_ports,                          // out
            uint32_t *untagged_set                              // out
     ) __attribute__((warn_unused_result));

int rtu_fdb_proxy_read_next_static_vlan_entry(
            uint16_t *vid,                                      // inout
            uint32_t *egress_ports,                             // out
            uint32_t *forbidden_ports,                          // out
            uint32_t *untagged_set                              // out
     ) __attribute__((warn_unused_result));

int rtu_fdb_proxy_read_vlan_entry(
            uint16_t vid,
            uint8_t *fid,                                       // out
            int *entry_type,                                    // out
            uint32_t *port_mask,                                // out
            uint32_t *untagged_set,                             // out
            unsigned long *creation_t                           // out
     ) __attribute__((warn_unused_result));

int rtu_fdb_proxy_read_next_vlan_entry(
            uint16_t *vid,                                      // inout
            uint8_t *fid,                                       // out
            int *entry_type,                                    // out
            uint32_t *port_mask,                                // out
            uint32_t *untagged_set,                             // out
            unsigned long *creation_t                           // out
     ) __attribute__((warn_unused_result));

int  rtu_fdb_proxy_set_aging_time(
            uint8_t fid,
            unsigned long t
    ) __attribute__((warn_unused_result));

unsigned long rtu_fdb_proxy_get_aging_time(uint8_t fid);

uint16_t rtu_fdb_proxy_get_num_dynamic_entries(uint8_t fid);
uint32_t rtu_fdb_proxy_get_num_learned_entry_discards(uint8_t fid);
uint16_t rtu_fdb_proxy_get_num_vlans(void);
uint16_t rtu_fdb_proxy_get_max_supported_vlans(void);
uint16_t rtu_fdb_proxy_get_max_vid(void);
uint64_t rtu_fdb_proxy_get_num_vlan_deletes(void);
uint16_t rtu_fdb_proxy_get_next_fid(uint8_t fid);

int rtu_vfdb_proxy_forward_dynamic(int port, uint16_t vid);
int rtu_vfdb_proxy_filter_dynamic(int port, uint16_t vid);

void rtu_fdb_proxy_delete_dynamic_entries(int port, uint16_t vid);

int rtu_fdb_proxy_is_restricted_vlan_reg(int port);
int rtu_fdb_proxy_set_restricted_vlan_reg(int port);
int rtu_fdb_proxy_unset_restricted_vlan_reg(int port);

int rtu_fdb_proxy_get_size(void);
int rtu_fdb_proxy_get_num_all_static_entries(void);
int rtu_fdb_proxy_get_num_all_dynamic_entries(void);
int rtu_vfdb_proxy_get_num_all_static_entries(void);
int rtu_vfdb_proxy_get_num_all_dynamic_entries(void);

int rtu_fdb_proxy_create_lc(int sid, uint16_t vid, int lc_type);
int rtu_fdb_proxy_delete_lc(int sid, uint16_t vid);
int rtu_fdb_proxy_read_lc(uint16_t vid, int *lc_set);
int rtu_fdb_proxy_read_next_lc(uint16_t *vid, int *lc_set);
int rtu_fdb_proxy_read_lc_set_type(int sid, int *lc_type);

int rtu_fdb_proxy_set_default_lc(int sid);
void rtu_fdb_proxy_get_default_lc(int *sid, int *lc_type);

void rtu_fdb_proxy_read_fid(uint16_t vid, uint8_t *fid, int *fid_fixed);
int rtu_fdb_proxy_read_next_fid(uint16_t *vid, uint8_t *fid, int *fid_fixed);
int rtu_fdb_proxy_set_fid(uint16_t vid, uint8_t fid);
int rtu_fdb_proxy_delete_fid(uint16_t vid);

int rtu_fdb_proxy_set_default_lc_type(int lc_type);

void rtu_fdb_proxy_init(char *name);
int rtu_fdb_proxy_valid();
int rtu_fdb_proxy_connected();

#endif /*__WHITERABBIT_RTU_FD_PROXY_H*/
