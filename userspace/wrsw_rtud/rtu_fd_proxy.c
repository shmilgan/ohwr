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
 *              Based on the mini_ipc framework.
 *              NOTE: errno used to inform of mini-ipc related errors
 *
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

#include <stdlib.h>
#include <errno.h>

#include "rtu_fd_proxy.h"
#include "rtu_fd_structs.h"

struct minipc_ch *client;

char name[MINIPC_MAX_NAME];

static struct minipc_ch *client_create()
{
    if (!client) {
    	client = minipc_client_create(name, 0);
        if (client)
    	    minipc_set_logfile(client, stderr);
    }
    return client;
}

static inline void check_conn(void)
{
    int err = errno;
    if (err == EPIPE || err == ENOTCONN || err == ECONNREFUSED) {
        minipc_close(client); 
        client = NULL;
        errno = err;
    }
}

int  rtu_fdb_proxy_create_static_entry(
            uint8_t mac[ETH_ALEN],
            uint16_t vid,
            uint32_t egress_ports,
            uint32_t forbidden_ports,
            int type,
            int active,
            int is_bpdu)
{
    struct rtu_fdb_create_static_entry_argdata in;
    int out;

    if (!client_create())
        return -1;

    in.vid              = vid;
    in.egress_ports     = egress_ports;
    in.forbidden_ports  = forbidden_ports;
    in.type             = type;
    in.active           = active;
    in.is_bpdu          = is_bpdu;
    mac_copy(in.mac, mac);

    errno = 0;
	if (minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_create_static_entry_struct, &out, &in) < 0)
        check_conn();    
    return out;
}

int rtu_fdb_proxy_delete_static_entry(
            uint8_t mac[ETH_ALEN],
            uint16_t vid)
{
    struct rtu_fdb_delete_static_entry_argdata in;
    int out;

    if (!client_create())
        return -1;

    in.vid = vid;
    mac_copy(in.mac, mac);

    errno = 0;
	if (minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_delete_static_entry_struct, &out, &in) < 0)
        check_conn();    
    return out;
}

int rtu_fdb_proxy_read_entry(
           uint8_t mac[ETH_ALEN],
           uint8_t fid,
           uint32_t *port_map,
           int *entry_type)
{
    struct rtu_fdb_read_entry_argdata in;
    struct rtu_fdb_read_entry_retdata out;

    if (!client_create())
        return -1;

    mac_copy(in.mac, mac);
    in.fid = fid;

    errno = 0;
	if (minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_fdb_read_entry_struct, &out, &in) == 0) {
        *port_map   = out.port_map;
        *entry_type = out.entry_type;
    } else {
        check_conn();    
    }
    return out.retval;
}

int rtu_fdb_proxy_read_next_entry(
           uint8_t (*mac)[ETH_ALEN],
           uint8_t *fid,
           uint32_t *port_map,
           int *entry_type)
{
    struct rtu_fdb_read_next_entry_argdata in;
    struct rtu_fdb_read_next_entry_retdata out;

    if (!client_create())
        return -1;

    in.fid = *fid;
    mac_copy(in.mac, *mac);

    errno = 0;
	if (minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_read_next_entry_struct, &out, &in) == 0) {
        mac_copy(*mac, out.mac);
        *fid        = out.fid;
        *port_map   = out.port_map;
        *entry_type = out.entry_type;
    } else {
        check_conn();    
    }
    return out.retval;
}

int rtu_fdb_proxy_read_static_entry(
            uint8_t mac[ETH_ALEN],
            uint16_t vid,
            uint32_t *egress_ports,
            uint32_t *forbidden_ports,
            int *type,
            int *active)
{
    struct rtu_fdb_read_static_entry_argdata in;
    struct rtu_fdb_read_static_entry_retdata out;

    if (!client_create())
        return -1;

    in.vid = vid;
    mac_copy(in.mac, mac);

    errno = 0;
	if (minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_read_static_entry_struct, &out, &in) == 0) {
        *egress_ports    = out.egress_ports;
        *forbidden_ports = out.forbidden_ports;
        *type            = out.type;
        *active          = out.active;
    } else {
        check_conn();    
    }
    return out.retval;
}

int rtu_fdb_proxy_read_next_static_entry(
            uint8_t (*mac)[ETH_ALEN],
            uint16_t *vid,
            uint32_t *egress_ports,
            uint32_t *forbidden_ports,
            int *type,
            int *active)
{
    struct rtu_fdb_read_next_static_entry_argdata in;
    struct rtu_fdb_read_next_static_entry_retdata out;

    if (!client_create())
        return -1;

    in.vid = *vid;
    mac_copy(in.mac, *mac);

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_read_next_static_entry_struct, &out, &in) == 0) {
        *vid             = out.vid;
        *egress_ports    = out.egress_ports;
        *forbidden_ports = out.forbidden_ports;
        *type            = out.type;
        *active          = out.active;
        mac_copy(*mac, out.mac);
    } else {
        check_conn();    
    }
    return out.retval;
}


int  rtu_fdb_proxy_set_aging_time(
            uint8_t fid,
            unsigned long t)
{
    struct rtu_fdb_set_aging_time_argdata in;
    int out;

    if (!client_create())
        return -1;

    in.fid = fid;
    in.t   = t;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_set_aging_time_struct, &out, &in) < 0)
        check_conn();    
    return out;
}

unsigned long rtu_fdb_proxy_get_aging_time(uint8_t fid)
{
    struct rtu_fdb_get_aging_time_argdata in;
    struct rtu_fdb_get_aging_time_retdata out;

    if (!client_create())
        return -1;

    in.fid = fid;

    if (minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_get_aging_time_struct, &out, &in) < 0)
        check_conn();    
    return out.retval;
}


uint16_t rtu_fdb_proxy_get_num_dynamic_entries(uint8_t fid)
{
    struct rtu_fdb_get_num_dynamic_entries_argdata in;
    struct rtu_fdb_get_num_dynamic_entries_retdata out;

    if (!client_create())
        return -1;

    in.fid = fid;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_get_num_dynamic_entries_struct, &out, &in) < 0)
        check_conn();    
    return out.retval;
}

uint32_t rtu_fdb_proxy_get_num_learned_entry_discards(uint8_t fid)
{
    struct rtu_fdb_get_num_learned_entry_discards_argdata in;
    struct rtu_fdb_get_num_learned_entry_discards_retdata out;

    if (!client_create())
        return -1;

    in.fid = fid;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_get_num_learned_entry_discards_struct, &out, &in) < 0)
        check_conn();    
    return out.retval;
}

uint16_t rtu_fdb_proxy_get_num_vlans(void)
{
    struct rtu_fdb_get_num_vlans_retdata out;

    if (!client_create())
        return -1;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_get_num_vlans_struct, &out) < 0)
        check_conn();    
    return out.retval;
}

uint16_t rtu_fdb_proxy_get_max_supported_vlans(void)
{
    struct rtu_fdb_get_max_supported_vlans_retdata out;

    if (!client_create())
        return -1;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_get_max_supported_vlans_struct, &out) < 0)
        check_conn();    
    return out.retval;
}

uint16_t rtu_fdb_proxy_get_max_vid(void)
{
    struct rtu_fdb_get_max_vid_retdata out;

    if (!client_create())
        return -1;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT, 
        &rtu_fdb_get_max_vid_struct, &out) < 0)
        check_conn();    
    return out.retval;
}

uint64_t rtu_fdb_proxy_get_num_vlan_deletes(void)
{
    struct rtu_fdb_get_num_vlan_deletes_retdata out;

    if (!client_create())
        return -1;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_get_num_vlan_deletes_struct, &out) < 0)
        check_conn();    
    return out.retval;
}

uint16_t rtu_fdb_proxy_get_next_fid(uint8_t fid)
{
    struct rtu_fdb_get_next_fid_argdata in;
    struct rtu_fdb_get_next_fid_retdata out;

    if (!client_create())
        return -1;

    in.fid = fid;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_get_next_fid_struct, &out, &in) < 0)
        check_conn();    
    return out.retval;
}

int rtu_fdb_proxy_create_static_vlan_entry(
            uint16_t vid,
            uint32_t egress_ports,
            uint32_t forbidden_ports,
            uint32_t untagged_set)
{
    struct rtu_fdb_create_static_vlan_entry_argdata in;
    int out;

    if (!client_create())
        return -1;

    in.vid              = vid;
    in.egress_ports     = egress_ports;
    in.forbidden_ports  = forbidden_ports;
    in.untagged_set     = untagged_set;

    errno = 0;
    if (minipc_call(client,  MILLISEC_TIMEOUT,
        &rtu_fdb_create_static_vlan_entry_struct, &out, &in) < 0)
        check_conn();    
    return out;
}

int rtu_fdb_proxy_delete_static_vlan_entry(uint16_t vid)
{
    struct rtu_fdb_delete_static_vlan_entry_argdata in;
    int out;

    if (!client_create())
        return -1;

    in.vid = vid;

    errno = 0;
    if (minipc_call(client,  MILLISEC_TIMEOUT,
        &rtu_fdb_delete_static_vlan_entry_struct, &out, &in) < 0)
        check_conn();    
    return out;
}

int rtu_fdb_proxy_read_static_vlan_entry(
            uint16_t vid,
            uint32_t *egress_ports,                             // out
            uint32_t *forbidden_ports,                          // out
            uint32_t *untagged_set)                             // out
{
    struct rtu_fdb_read_static_vlan_entry_argdata in;
    struct rtu_fdb_read_static_vlan_entry_retdata out;

    if (!client_create())
        return -1;

    in.vid = vid;

    errno = 0;
    if (minipc_call(client,  MILLISEC_TIMEOUT,
        &rtu_fdb_read_static_vlan_entry_struct, &out, &in) == 0) {
        *egress_ports    = out.egress_ports;
        *forbidden_ports = out.forbidden_ports;
        *untagged_set    = out.untagged_set;
    } else {
        check_conn();    
    }
    return out.retval;
}

int rtu_fdb_proxy_read_next_static_vlan_entry(
            uint16_t *vid,                                      // inout
            uint32_t *egress_ports,                             // out
            uint32_t *forbidden_ports,                          // out
            uint32_t *untagged_set)                             // out
{
    struct rtu_fdb_read_next_static_vlan_entry_argdata in;
    struct rtu_fdb_read_next_static_vlan_entry_retdata out;

    if (!client_create())
        return -1;

    in.vid = *vid;

    errno = 0;
    if (minipc_call(client,  MILLISEC_TIMEOUT,
        &rtu_fdb_read_next_static_vlan_entry_struct, &out, &in) == 0) {
        *vid             = out.vid;
        *egress_ports    = out.egress_ports;
        *forbidden_ports = out.forbidden_ports;
        *untagged_set    = out.untagged_set;
    } else {
        check_conn();    
    }
    return out.retval;
}

int rtu_fdb_proxy_read_vlan_entry(
            uint16_t vid,
            uint8_t *fid,                                       // out
            int *entry_type,                                    // out
            uint32_t *port_mask,                                // out
            uint32_t *untagged_set,                             // out
            unsigned long *creation_t)                          // out
{
    struct rtu_fdb_read_vlan_entry_argdata in;
    struct rtu_fdb_read_vlan_entry_retdata out;

    if (!client_create())
        return -1;

    in.vid = vid;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_fdb_read_vlan_entry_struct, &out, &in) == 0) {
        *fid            = out.fid;
        *entry_type     = out.entry_type;
        *port_mask      = out.port_mask;
        *untagged_set   = out.untagged_set;
        *creation_t     = out.creation_t;
    } else {
        check_conn();    
    }
    return out.retval;
}

int rtu_fdb_proxy_read_next_vlan_entry(
            uint16_t *vid,                                      // inout
            uint8_t *fid,                                       // out
            int *entry_type,                                    // out
            uint32_t *port_mask,                                // out
            uint32_t *untagged_set,                             // out
            unsigned long *creation_t)                          // out
{
    struct rtu_fdb_read_next_vlan_entry_argdata in;
    struct rtu_fdb_read_next_vlan_entry_retdata out;

    if (!client_create())
        return -1;

    in.vid = *vid;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_fdb_read_next_vlan_entry_struct, &out, &in) == 0) {
        *vid            = out.vid;
        *fid            = out.fid;
        *entry_type     = out.entry_type;
        *port_mask      = out.port_mask;
        *untagged_set   = out.untagged_set;
        *creation_t     = out.creation_t;
    } else {
        check_conn();    
    }
    return out.retval;
}

int rtu_vfdb_proxy_forward_dynamic(int port, uint16_t vid)
{
    struct rtu_vfdb_forward_dynamic_argdata in;
    int out;

    if (!client_create())
        return -1;

    in.vid = vid;
    in.port = port;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_vfdb_forward_dynamic_struct, &out, &in) < 0)
        check_conn();    
    return out;
}

int rtu_vfdb_proxy_filter_dynamic(int port, uint16_t vid)
{
    struct rtu_vfdb_filter_dynamic_argdata in;
    int out;

    if (!client_create())
        return -1;

    in.vid = vid;
    in.port = port;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_vfdb_filter_dynamic_struct, &out, &in) < 0)
        check_conn();    
    return out;
}

void rtu_fdb_proxy_delete_dynamic_entries(int port, uint16_t vid)
{
    struct rtu_fdb_delete_dynamic_entries_argdata in;
    int out;

    if (!client_create())
        return;

    in.vid = vid;
    in.port = port;
    
    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_fdb_delete_dynamic_entries_struct, &out, &in) < 0)
        check_conn();    
}

int rtu_fdb_proxy_is_restricted_vlan_reg(int port)
{
    struct rtu_fdb_is_restricted_vlan_reg_argdata in;
    int out;

    if (!client_create())
        return -1;

    in.port = port;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_fdb_is_restricted_vlan_reg_struct, &out, &in) < 0)
        check_conn();    
    return out;
}

int rtu_fdb_proxy_set_restricted_vlan_reg(int port)
{
    struct rtu_fdb_set_restricted_vlan_reg_argdata in;
    int out;

    if (!client_create())
        return -1;

    in.port = port;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_fdb_set_restricted_vlan_reg_struct, &out, &in) < 0)
        check_conn();    
    return out;
}

int rtu_fdb_proxy_unset_restricted_vlan_reg(int port)
{
    struct rtu_fdb_unset_restricted_vlan_reg_argdata in;
    int out;

    if (!client_create())
        return -1;

    in.port = port;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_fdb_unset_restricted_vlan_reg_struct, &out, &in) < 0)
        check_conn();    
    return out;
}

int rtu_fdb_proxy_get_size(void)
{
    int out;

    if (!client_create())
        return -1;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT, &rtu_fdb_get_size_struct, &out) < 0)
        check_conn();    
    return out;
}

int rtu_fdb_proxy_get_num_all_static_entries(void)
{
    int out;

    if (!client_create())
        return -1;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_fdb_get_num_all_static_entries_struct, &out) < 0)
        check_conn();    
    return out;
}

int rtu_fdb_proxy_get_num_all_dynamic_entries(void)
{
    int out;

    if (!client_create())
        return -1;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_fdb_get_num_all_dynamic_entries_struct, &out) < 0)
        check_conn();    
    return out;
}

int rtu_vfdb_proxy_get_num_all_static_entries(void)
{
    int out;

    if (!client_create())
        return -1;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_vfdb_get_num_all_static_entries_struct, &out) < 0)
        check_conn();    
    return out;
}

int rtu_vfdb_proxy_get_num_all_dynamic_entries(void)
{
    int out;

    if (!client_create())
        return -1;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_vfdb_get_num_all_dynamic_entries_struct, &out) < 0)
        check_conn();    
    return out;
}

int rtu_fdb_proxy_create_lc(int sid, uint16_t vid, int lc_type)
{
    struct rtu_fdb_create_lc_argdata in;
    int out;

    if (!client_create())
        return -1;

    in.sid = sid;
    in.vid = vid;
    in.lc_type = lc_type;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT, 
        &rtu_fdb_create_lc_struct, &out, &in) < 0)
        check_conn();    
    return out;
}

int rtu_fdb_proxy_delete_lc(int sid, uint16_t vid)
{
    struct rtu_fdb_delete_lc_argdata in;
    int out;

    if (!client_create())
        return -1;

    in.sid = sid;
    in.vid = vid;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT, 
        &rtu_fdb_delete_lc_struct, &out, &in) < 0)
        check_conn();    
    return out;
}

int rtu_fdb_proxy_read_lc(uint16_t vid, int *lc_set)
{
    struct rtu_fdb_read_lc_argdata in;
    struct rtu_fdb_read_lc_retdata out;

    if (!client_create())
        return -1;

    in.vid = vid;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT, 
        &rtu_fdb_read_lc_struct, &out, &in) == 0) {
        *lc_set = out.lc_set;
    } else {
        check_conn();    
    }
    return out.retval;
}

int rtu_fdb_proxy_read_next_lc(uint16_t *vid, int *lc_set)
{
    struct rtu_fdb_read_next_lc_argdata in;
    struct rtu_fdb_read_next_lc_retdata out;

    if (!client_create())
        return -1;

    in.vid = *vid;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT, 
        &rtu_fdb_read_next_lc_struct, &out, &in) == 0) {
        *vid = out.vid;
        *lc_set = out.lc_set;
    } else {
        check_conn();    
    }
    return out.retval;
}

int rtu_fdb_proxy_read_lc_set_type(int sid, int *lc_type)
{
    struct rtu_fdb_read_lc_set_type_retdata out;

    if (!client_create())
        return -1;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_fdb_read_lc_set_type_struct, &out, sid) == 0) {
        *lc_type = out.lc_type;
    } else {
        check_conn();    
    }
    return out.retval;
}

int rtu_fdb_proxy_set_default_lc(int sid)
{
    int out;

    if (!client_create())
        return -1;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT, 
        &rtu_fdb_set_default_lc_struct, &out, sid) < 0)
        check_conn();    
    return out;
}

void rtu_fdb_proxy_get_default_lc(int *sid, int *lc_type)
{
    struct rtu_fdb_get_default_lc_retdata out;

    if (!client_create())
        return;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_fdb_get_default_lc_struct, &out) == 0) {
        *sid = out.sid;
        *lc_type = out.lc_type;
    } else {
        check_conn();    
    }
}

void rtu_fdb_proxy_read_fid(uint16_t vid, uint8_t *fid, int *fid_fixed)
{
    struct rtu_fdb_read_fid_argdata in;
    struct rtu_fdb_read_fid_retdata out;

    if (!client_create())
        return;

    in.vid = vid;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT, 
        &rtu_fdb_read_fid_struct, &out, &in) == 0) {
        *fid = out.fid;
        *fid_fixed = out.fid_fixed;
    } else {
        check_conn();    
    }
}

int rtu_fdb_proxy_read_next_fid(uint16_t *vid, uint8_t *fid, int *fid_fixed)
{
    struct rtu_fdb_read_next_fid_argdata in;
    struct rtu_fdb_read_next_fid_retdata out;

    if (!client_create())
        return -1;

    in.vid = *vid;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT, 
        &rtu_fdb_read_next_fid_struct, &out, &in) == 0) {
        *vid = out.vid;
        *fid = out.fid;
        *fid_fixed = out.fid_fixed;
    } else {
        check_conn();    
    }
    return out.retval;
}

int rtu_fdb_proxy_set_fid(uint16_t vid, uint8_t fid)
{
    struct rtu_fdb_set_fid_argdata in;
    int out;

    if (!client_create())
        return -1;

    in.vid = vid;
    in.fid = fid;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT, 
        &rtu_fdb_set_fid_struct, &out, &in) < 0)
        check_conn();    
    return out;
}

int rtu_fdb_proxy_delete_fid(uint16_t vid)
{
    struct rtu_fdb_delete_fid_argdata in;
    int out;

    if (!client_create())
        return -1;

    in.vid = vid;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT, 
        &rtu_fdb_delete_fid_struct, &out, &in) < 0)
        check_conn();    
    return out;
}

int rtu_fdb_proxy_set_default_lc_type(int lc_type)
{
    int out;

    if (!client_create())
        return -1;

    errno = 0;
    if (minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_fdb_set_default_lc_type_struct, &out, lc_type) < 0)
        check_conn();    
    return out;
}

void rtu_fdb_proxy_init(char* _name)
{
    strncpy(name, _name, sizeof(name) - 1);
}

int rtu_fdb_proxy_valid()
{
    return (int)client;
}

int rtu_fdb_proxy_connected()
{
    rtu_fdb_proxy_get_size();
    return (int)client;
}

