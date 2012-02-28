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

#include "rtu_fd_proxy.h"
#include "rtu_fd_export.h"

struct minipc_ch *client = NULL;

const struct minipc_pd rtu_fdb_proxy_get_max_vid_struct = {
    .name   = "0",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_max_vid_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_max_vid_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_get_max_supported_vlans_struct = {
    .name   = "1",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_max_supported_vlans_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_max_supported_vlans_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_get_num_vlans_struct = {
    .name   = "2",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_vlans_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_vlans_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_get_num_dynamic_entries_struct = {
    .name   = "3",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_dynamic_entries_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_dynamic_entries_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_get_num_learned_entry_discards_struct = {
    .name   = "4",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_learned_entry_discards_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_learned_entry_discards_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_get_num_vlan_deletes_struct = {
    .name   = "5",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_vlan_deletes_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_vlan_deletes_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_get_aging_time_struct = {
    .name   = "6",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_aging_time_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_aging_time_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_set_aging_time_struct = {
    .name   = "7",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_set_aging_time_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_set_aging_time_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_read_entry_struct = {
    .name   = "8",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_entry_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_read_next_entry_struct = {
    .name   = "9",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_entry_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_create_static_entry_struct = {
    .name   = "10",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_create_static_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_create_static_entry_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_read_static_entry_struct = {
    .name   = "11",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_static_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_static_entry_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_read_next_static_entry_struct = {
    .name   = "12",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_static_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_static_entry_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_delete_static_entry_struct = {
    .name   = "13",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_delete_static_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_delete_static_entry_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_get_next_fid_struct = {
    .name   = "14",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_next_fid_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_next_fid_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_create_static_vlan_entry_struct = {
    .name   = "15",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_create_static_vlan_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_create_static_vlan_entry_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_delete_static_vlan_entry_struct = {
    .name   = "16",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_delete_static_vlan_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_delete_static_vlan_entry_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_read_static_vlan_entry_struct = {
    .name   = "17",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_static_vlan_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_static_vlan_entry_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_read_next_static_vlan_entry_struct = {
    .name   = "18",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_static_vlan_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_static_vlan_entry_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_read_vlan_entry_struct = {
    .name   = "19",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_vlan_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_vlan_entry_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_read_next_vlan_entry_struct = {
    .name   = "20",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_vlan_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_vlan_entry_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_vfdb_proxy_forward_dynamic_struct = {
    .name   = "21",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_vfdb_forward_dynamic_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_vfdb_forward_dynamic_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_vfdb_proxy_filter_dynamic_struct = {
    .name   = "22",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_vfdb_filter_dynamic_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_vfdb_filter_dynamic_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_delete_dynamic_entries_struct = {
    .name   = "23",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_delete_dynamic_entries_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_delete_dynamic_entries_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_is_restricted_vlan_reg_struct = {
    .name   = "24",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_is_restricted_vlan_reg_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_is_restricted_vlan_reg_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_set_restricted_vlan_reg_struct = {
    .name   = "25",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_set_restricted_vlan_reg_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_set_restricted_vlan_reg_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_proxy_unset_restricted_vlan_reg_struct = {
    .name   = "26",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_unset_restricted_vlan_reg_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_unset_restricted_vlan_reg_argdata),
        MINIPC_ARG_END,
    }
};



// IMPORTANT NOTE: errno used to inform of mini-ipc related errors
// (errno value as set by minipc_call)
// errno should be checked by callee

int  rtu_fdb_proxy_create_static_entry(
            uint8_t mac[ETH_ALEN],
            uint16_t vid,
            uint32_t egress_ports,
            uint32_t forbidden_ports,
            int type,
            int active,
            int is_bpdu)
{
    int ret;
    struct rtu_fdb_create_static_entry_argdata in;
    struct rtu_fdb_create_static_entry_retdata out;

    in.vid              = vid;
    in.egress_ports     = egress_ports;
    in.forbidden_ports  = forbidden_ports;
    in.type             = type;
    in.active           = active;
    in.is_bpdu          = is_bpdu;
    mac_copy(in.mac, mac);

	ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_create_static_entry_struct, &out, &in);

    return out.retval;
}

int rtu_fdb_proxy_delete_static_entry(
            uint8_t mac[ETH_ALEN],
            uint16_t vid)
{
    int ret;
    struct rtu_fdb_delete_static_entry_argdata in;
    struct rtu_fdb_delete_static_entry_retdata out;

    in.vid = vid;
    mac_copy(in.mac, mac);

	ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_delete_static_entry_struct, &out, &in);

    return out.retval;
}

int rtu_fdb_proxy_read_entry(
           uint8_t mac[ETH_ALEN],
           uint8_t fid,
           uint32_t *port_map,
           int *entry_type)
{
    int ret;
    struct rtu_fdb_read_entry_argdata in;
    struct rtu_fdb_read_entry_retdata out;

    mac_copy(in.mac, mac);
    in.fid = fid;

	ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_read_entry_struct, &out, &in);

    if (ret == 0) {
        *port_map   = out.port_map;
        *entry_type = out.entry_type;
    }

    return out.retval;
}

int rtu_fdb_proxy_read_next_entry(
           uint8_t (*mac)[ETH_ALEN],
           uint8_t *fid,
           uint32_t *port_map,
           int *entry_type)
{
    int ret;
    struct rtu_fdb_read_next_entry_argdata in;
    struct rtu_fdb_read_next_entry_retdata out;

    in.fid = *fid;
    mac_copy(in.mac, *mac);

	ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_read_next_entry_struct, &out, &in);

    if (ret == 0) {
        mac_copy(*mac, out.mac);
        *fid        = out.fid;
        *port_map   = out.port_map;
        *entry_type = out.entry_type;
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
    int ret;
    struct rtu_fdb_read_static_entry_argdata in;
    struct rtu_fdb_read_static_entry_retdata out;

    in.vid = vid;
    mac_copy(in.mac, mac);

	ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_read_static_entry_struct, &out, &in);

    if (ret == 0) {
        *egress_ports    = out.egress_ports;
        *forbidden_ports = out.forbidden_ports;
        *type            = out.type;
        *active          = out.active;
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
    int ret;
    struct rtu_fdb_read_next_static_entry_argdata in;
    struct rtu_fdb_read_next_static_entry_retdata out;

    in.vid = *vid;
    mac_copy(in.mac, *mac);

    ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_read_next_static_entry_struct, &out, &in);

    if (ret == 0) {
        *vid             = out.vid;
        *egress_ports    = out.egress_ports;
        *forbidden_ports = out.forbidden_ports;
        *type            = out.type;
        *active          = out.active;
        mac_copy(*mac, out.mac);
    }

    return out.retval;
}


// errno should be checked by callee
int  rtu_fdb_proxy_set_aging_time(
            uint8_t fid,
            unsigned long t)
{
    int ret;
    struct rtu_fdb_set_aging_time_argdata in;
    struct rtu_fdb_set_aging_time_retdata out;

    in.fid = fid;
    in.t   = t;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_set_aging_time_struct, &out, &in);

    return out.retval;
}

// errno should be checked by callee
unsigned long rtu_fdb_proxy_get_aging_time(uint8_t fid)
{
    int ret;
    struct rtu_fdb_get_aging_time_argdata in;
    struct rtu_fdb_get_aging_time_retdata out;

    in.fid = fid;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_get_aging_time_struct, &out, &in);

    return out.retval;
}


// errno should be checked by callee
uint16_t rtu_fdb_proxy_get_num_dynamic_entries(uint8_t fid)
{
    int ret;
    struct rtu_fdb_get_num_dynamic_entries_argdata in;
    struct rtu_fdb_get_num_dynamic_entries_retdata out;

    in.fid = fid;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_get_num_dynamic_entries_struct, &out, &in);

    return out.retval;
}

// errno should be checked by callee
uint32_t rtu_fdb_proxy_get_num_learned_entry_discards(uint8_t fid)
{
    int ret;
    struct rtu_fdb_get_num_learned_entry_discards_argdata in;
    struct rtu_fdb_get_num_learned_entry_discards_retdata out;

    in.fid = fid;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_get_num_learned_entry_discards_struct, &out, &in);

    return out.retval;
}

// errno should be checked by callee
uint16_t rtu_fdb_proxy_get_num_vlans(void)
{
    int ret;
    struct rtu_fdb_get_num_vlans_argdata in;
    struct rtu_fdb_get_num_vlans_retdata out;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_get_num_vlans_struct, &out, &in);

    return out.retval;
}

// errno should be checked by callee
uint16_t rtu_fdb_proxy_get_max_supported_vlans(void)
{
    int ret;
    struct rtu_fdb_get_max_supported_vlans_argdata in;
    struct rtu_fdb_get_max_supported_vlans_retdata out;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_get_max_supported_vlans_struct, &out, &in);

    return out.retval;
}

// errno should be checked by callee
uint16_t rtu_fdb_proxy_get_max_vid(void)
{
    int ret;
    struct rtu_fdb_get_max_vid_argdata in;
    struct rtu_fdb_get_max_vid_retdata out;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_get_max_vid_struct, &out, &in);

    return out.retval;
}

// errno should be checked by callee
uint64_t rtu_fdb_proxy_get_num_vlan_deletes(void)
{
    int ret;
    struct rtu_fdb_get_num_vlan_deletes_argdata in;
    struct rtu_fdb_get_num_vlan_deletes_retdata out;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_get_num_vlan_deletes_struct, &out, &in);

    return out.retval;
}

uint16_t rtu_fdb_proxy_get_next_fid(uint8_t fid)
{
    int ret;
    struct rtu_fdb_get_next_fid_argdata in;
    struct rtu_fdb_get_next_fid_retdata out;

    in.fid = fid;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_get_next_fid_struct, &out, &in);

    return out.retval;
}

int rtu_fdb_proxy_create_static_vlan_entry(
            uint16_t vid,
            uint8_t fid,
            uint32_t egress_ports,
            uint32_t forbidden_ports,
            uint32_t untagged_set)
{
    int ret;
    struct rtu_fdb_create_static_vlan_entry_argdata in;
    struct rtu_fdb_create_static_vlan_entry_retdata out;

    in.vid              = vid;
    in.fid              = fid;
    in.egress_ports     = egress_ports;
    in.forbidden_ports  = forbidden_ports;
    in.untagged_set     = untagged_set;

    ret = minipc_call(client,  MILLISEC_TIMEOUT,
        &rtu_fdb_proxy_create_static_vlan_entry_struct, &out, &in);

    return out.retval;
}

int rtu_fdb_proxy_delete_static_vlan_entry(uint16_t vid)
{
    int ret;
    struct rtu_fdb_delete_static_vlan_entry_argdata in;
    struct rtu_fdb_delete_static_vlan_entry_retdata out;

    in.vid = vid;

    ret = minipc_call(client,  MILLISEC_TIMEOUT,
        &rtu_fdb_proxy_delete_static_vlan_entry_struct, &out, &in);

    return out.retval;
}

int rtu_fdb_proxy_read_static_vlan_entry(
            uint16_t vid,
            uint32_t *egress_ports,                             // out
            uint32_t *forbidden_ports,                          // out
            uint32_t *untagged_set)                             // out
{
    int ret;
    struct rtu_fdb_read_static_vlan_entry_argdata in;
    struct rtu_fdb_read_static_vlan_entry_retdata out;

    in.vid = vid;

    ret = minipc_call(client,  MILLISEC_TIMEOUT,
        &rtu_fdb_proxy_read_static_vlan_entry_struct, &out, &in);

    if (ret == 0) {
        *egress_ports    = out.egress_ports;
        *forbidden_ports = out.forbidden_ports;
        *untagged_set    = out.untagged_set;
    }

    return out.retval;
}

int rtu_fdb_proxy_read_next_static_vlan_entry(
            uint16_t *vid,                                      // inout
            uint32_t *egress_ports,                             // out
            uint32_t *forbidden_ports,                          // out
            uint32_t *untagged_set)                             // out
{
    int ret;
    struct rtu_fdb_read_next_static_vlan_entry_argdata in;
    struct rtu_fdb_read_next_static_vlan_entry_retdata out;

    in.vid = *vid;

    ret = minipc_call(client,  MILLISEC_TIMEOUT,
        &rtu_fdb_proxy_read_next_static_vlan_entry_struct, &out, &in);

    if (ret == 0) {
        *vid             = out.vid;
        *egress_ports    = out.egress_ports;
        *forbidden_ports = out.forbidden_ports;
        *untagged_set    = out.untagged_set;
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
    int ret;
    struct rtu_fdb_read_vlan_entry_argdata in;
    struct rtu_fdb_read_vlan_entry_retdata out;

    in.vid = vid;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_fdb_proxy_read_vlan_entry_struct, &out, &in);

    if (ret == 0) {
        *fid            = out.fid;
        *entry_type     = out.entry_type;
        *port_mask      = out.port_mask;
        *untagged_set   = out.untagged_set;
        *creation_t     = out.creation_t;
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
    int ret;
    struct rtu_fdb_read_next_vlan_entry_argdata in;
    struct rtu_fdb_read_next_vlan_entry_retdata out;

    in.vid = *vid;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_fdb_proxy_read_next_vlan_entry_struct, &out, &in);

    if (ret == 0) {
        *vid            = out.vid;
        *fid            = out.fid;
        *entry_type     = out.entry_type;
        *port_mask      = out.port_mask;
        *untagged_set   = out.untagged_set;
        *creation_t     = out.creation_t;
    }

    return out.retval;
}

int rtu_vfdb_proxy_forward_dynamic(int port, uint16_t vid)
{
    int ret;
    struct rtu_vfdb_forward_dynamic_argdata in;
    struct rtu_vfdb_forward_dynamic_retdata out;

    in.vid = vid;
    in.port = port;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_vfdb_proxy_forward_dynamic_struct, &out, &in);

    return out.retval;
}

int rtu_vfdb_proxy_filter_dynamic(int port, uint16_t vid)
{
    int ret;
    struct rtu_vfdb_filter_dynamic_argdata in;
    struct rtu_vfdb_filter_dynamic_retdata out;

    in.vid = vid;
    in.port = port;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_vfdb_proxy_filter_dynamic_struct, &out, &in);

    return out.retval;
}

void rtu_fdb_proxy_delete_dynamic_entries(int port, uint16_t vid)
{
    int ret;
    struct rtu_fdb_delete_dynamic_entries_argdata in;
    struct rtu_fdb_delete_dynamic_entries_retdata out;

    in.vid = vid;
    in.port = port;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_fdb_proxy_delete_dynamic_entries_struct, &out, &in);
}

int rtu_fdb_proxy_is_restricted_vlan_reg(int port)
{
    int ret;
    struct rtu_fdb_is_restricted_vlan_reg_argdata in;
    struct rtu_fdb_is_restricted_vlan_reg_retdata out;

    in.port = port;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_fdb_proxy_is_restricted_vlan_reg_struct, &out, &in);

    return out.retval;
}

int rtu_fdb_proxy_set_restricted_vlan_reg(int port)
{
    int ret;
    struct rtu_fdb_set_restricted_vlan_reg_argdata in;
    struct rtu_fdb_set_restricted_vlan_reg_retdata out;

    in.port = port;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_fdb_proxy_set_restricted_vlan_reg_struct, &out, &in);

    return out.retval;
}

int rtu_fdb_proxy_unset_restricted_vlan_reg(int port)
{
    int ret;
    struct rtu_fdb_unset_restricted_vlan_reg_argdata in;
    struct rtu_fdb_unset_restricted_vlan_reg_retdata out;

    in.port = port;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_fdb_proxy_unset_restricted_vlan_reg_struct, &out, &in);

    return out.retval;
}


struct minipc_ch *rtu_fdb_proxy_create(char* name)
{
    if (!client) {
    	client = minipc_client_create(name, 0);
        if (client)
    	    minipc_set_logfile(client, stderr);
    }
    return client;
}
