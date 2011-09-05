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

// IMPORTANT NOTE: errno used to inform of mini-ipc related errors
// (errno value as set by minipc_call)

// errno should be checked by callee
int  rtu_fdb_proxy_create_static_entry(
            uint8_t mac[ETH_ALEN],
            uint16_t vid,
            enum filtering_control port_map[NUM_PORTS],
            enum storage_type type,
            int active)
{
    int ret;
    struct rtu_fdb_create_static_entry_argdata in;
    struct rtu_fdb_create_static_entry_retdata out;

    mac_copy(in.mac, mac);
    in.vid    = vid;
    in.type   = type;
    in.active = active;
    memcpy(in.port_map, port_map, NUM_PORTS);

	ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_create_static_entry_struct, &in, &out);

    return out.retval;
}

// errno should be checked by callee
int rtu_fdb_proxy_delete_static_entry(
            uint8_t mac[ETH_ALEN],
            uint16_t vid)
{
    int ret;
    struct rtu_fdb_delete_static_entry_argdata in;
    struct rtu_fdb_delete_static_entry_retdata out;

    mac_copy(in.mac, mac);
    in.vid  = vid;

	ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_delete_static_entry_struct, &in, &out);

    return out.retval;
}

// errno should be checked by callee
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
    in.fid  = fid;

	ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_read_entry_struct, &in, &out);

    if (ret == 0) {
        *port_map   = out.port_map;
        *entry_type = out.entry_type;
    }

    return out.retval;
}

// errno should be checked by callee
int rtu_fdb_proxy_read_next_entry(
           uint8_t (*mac)[ETH_ALEN],
           uint8_t *fid,
           uint32_t *port_map,
           int *entry_type)
{
    int ret;
    struct rtu_fdb_read_next_entry_argdata in;
    struct rtu_fdb_read_next_entry_retdata out;

    mac_copy(in.mac, *mac);
    in.fid  = *fid;

	ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_read_next_entry_struct, &in, &out);

    if (ret == 0) {
        mac_copy(*mac, out.mac);
        *fid        = out.fid;
        *port_map   = out.port_map;
        *entry_type = out.entry_type;
    }

    return out.retval;
}

// errno should be checked by callee
int rtu_fdb_proxy_read_static_entry(
            uint8_t mac[ETH_ALEN],
            uint16_t vid,
            enum filtering_control (*port_map)[NUM_PORTS],
            enum storage_type *type,
            int *active)
{
    int ret;
    struct rtu_fdb_read_static_entry_argdata in;
    struct rtu_fdb_read_static_entry_retdata out;

    mac_copy(in.mac, mac);
    in.vid  = vid;

	ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_read_static_entry_struct, &in, &out);

    if (ret == 0) {
        memcpy(*port_map, out.port_map, NUM_PORTS);
        *type   = out.type;
        *active = out.active;
    }

    return out.retval;
}


// errno should be checked by callee
int rtu_fdb_proxy_read_next_static_entry(
            uint8_t (*mac)[ETH_ALEN],
            uint16_t *vid,
            enum filtering_control (*port_map)[NUM_PORTS],
            enum storage_type *type,
            int *active)
{
    int ret;
    struct rtu_fdb_read_next_static_entry_argdata in;
    struct rtu_fdb_read_next_static_entry_retdata out;

    mac_copy(in.mac, *mac);
    in.vid  = *vid;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_read_next_static_entry_struct, &in, &out);

    if (ret == 0) {
        mac_copy(*mac, out.mac);
        *vid = out.vid;
        memcpy(*port_map, out.port_map, NUM_PORTS);
        *type   = out.type;
        *active = out.active;
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

    in.fid  = fid;
    in.t = t;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_set_aging_time_struct, &in, &out);

    return out.retval;
}

// errno should be checked by callee
unsigned long rtu_fd_proxy_get_aging_time(uint8_t fid)
{
    int ret;
    struct rtu_fdb_get_aging_time_argdata in;
    struct rtu_fdb_get_aging_time_retdata out;

    in.fid  = fid;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_get_aging_time_struct, &in, &out);

    return out.retval;
}


// errno should be checked by callee
uint16_t rtu_fdb_proxy_get_num_dynamic_entries(uint8_t fid)
{
    int ret;
    struct rtu_fdb_get_num_dynamic_entries_argdata in;
    struct rtu_fdb_get_num_dynamic_entries_retdata out;

    in.fid  = fid;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_get_num_dynamic_entries_struct, &in, &out);

    return out.retval;
}

// errno should be checked by callee
uint32_t rtu_fdb_proxy_get_num_learned_entry_discards(uint8_t fid)
{
    int ret;
    struct rtu_fdb_get_num_learned_entry_discards_argdata in;
    struct rtu_fdb_get_num_learned_entry_discards_retdata out;

    in.fid  = fid;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_get_num_learned_entry_discards_struct, &in, &out);

    return out.retval;
}

// errno should be checked by callee
uint16_t rtu_fdb_proxy_get_num_vlans(void)
{
    int ret;
    struct rtu_fdb_get_num_vlans_argdata in;
    struct rtu_fdb_get_num_vlans_retdata out;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_get_num_vlans_struct, &in, &out);

    return out.retval;
}

// errno should be checked by callee
uint16_t rtu_fdb_proxy_get_max_supported_vlans(void)
{
    int ret;
    struct rtu_fdb_get_max_supported_vlans_argdata in;
    struct rtu_fdb_get_max_supported_vlans_retdata out;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_get_max_supported_vlans_struct, &in, &out);

    return out.retval;
}

// errno should be checked by callee
uint16_t rtu_fdb_proxy_get_max_vid(void)
{
    int ret;
    struct rtu_fdb_get_max_vid_argdata in;
    struct rtu_fdb_get_max_vid_retdata out;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_get_max_vid_struct, &in, &out);

    return out.retval;
}

// errno should be checked by callee
uint64_t rtu_fdb_proxy_get_num_vlan_deletes(void)
{
    int ret;
    struct rtu_fdb_get_num_vlan_deletes_argdata in;
    struct rtu_fdb_get_num_vlan_deletes_retdata out;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_get_num_vlan_deletes_struct, &in, &out);

    return out.retval;
}

uint16_t rtu_fdb_proxy_get_next_fid(uint8_t fid)
{
    int ret;
    struct rtu_fdb_get_next_fid_argdata in;
    struct rtu_fdb_get_next_fid_retdata out;

    in.fid = fid;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
            &rtu_fdb_proxy_get_next_fid_struct, &in, &out);

    return out.retval;
}

int rtu_fdb_proxy_create_static_vlan_entry(
            uint16_t vid,
            uint8_t fid,
            enum registrar_control member_set[NUM_PORTS],
            uint32_t untagged_set)
{
    int ret;
    struct rtu_fdb_create_static_vlan_entry_argdata in;
    struct rtu_fdb_create_static_vlan_entry_retdata out;

    in.vid = vid;
    in.fid = fid;
    memcpy(in.member_set, member_set, NUM_PORTS);
    in.untagged_set = untagged_set;

    ret = minipc_call(client,  MILLISEC_TIMEOUT,
        &rtu_fdb_proxy_create_static_vlan_entry_struct, &in, &out);

    return out.retval;
}

int rtu_fdb_proxy_delete_static_vlan_entry(uint16_t vid)
{
    int ret;
    struct rtu_fdb_delete_static_vlan_entry_argdata in;
    struct rtu_fdb_delete_static_vlan_entry_retdata out;

    in.vid = vid;

    ret = minipc_call(client,  MILLISEC_TIMEOUT,
        &rtu_fdb_proxy_delete_static_vlan_entry_struct, &in, &out);

    return out.retval;
}

int rtu_fdb_proxy_read_static_vlan_entry(
            uint16_t vid,
            enum registrar_control (*member_set)[NUM_PORTS],    // out
            uint32_t *untagged_set)                             // out
{
    int ret;
    struct rtu_fdb_read_static_vlan_entry_argdata in;
    struct rtu_fdb_read_static_vlan_entry_retdata out;

    in.vid = vid;

    ret = minipc_call(client,  MILLISEC_TIMEOUT,
        &rtu_fdb_proxy_read_static_vlan_entry_struct, &in, &out);

    if (ret == 0) {
        memcpy(*member_set, out.member_set, NUM_PORTS);
        *untagged_set = out.untagged_set;
    }

    return out.retval;
}

int rtu_fdb_proxy_read_next_static_vlan_entry(
            uint16_t *vid,                                      // inout
            enum registrar_control (*member_set)[NUM_PORTS],    // out
            uint32_t *untagged_set)                             // out
{
    int ret;
    struct rtu_fdb_read_next_static_vlan_entry_argdata in;
    struct rtu_fdb_read_next_static_vlan_entry_retdata out;

    in.vid = *vid;

    ret = minipc_call(client,  MILLISEC_TIMEOUT,
        &rtu_fdb_proxy_read_next_static_vlan_entry_struct, &in, &out);

    if (ret == 0) {
        *vid = out.vid;
        memcpy(*member_set, out.member_set, NUM_PORTS);
        *untagged_set = out.untagged_set;
    }

    return out.retval;
}

int rtu_fdb_proxy_read_vlan_entry(
            uint16_t vid,
            uint8_t *fid,                                       // out
            int *entry_type,                                    // out
            enum registrar_control (*member_set)[NUM_PORTS],    // out
            uint32_t *untagged_set,                             // out
            unsigned long *creation_t)                          // out
{
    int ret;
    struct rtu_fdb_read_vlan_entry_argdata in;
    struct rtu_fdb_read_vlan_entry_retdata out;

    in.vid = vid;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_fdb_proxy_read_vlan_entry_struct, &in, &out);

    if (ret == 0) {
        *fid = out.fid;
        *entry_type = out.entry_type;
        memcpy(*member_set, out.member_set, NUM_PORTS);
        *untagged_set = out.untagged_set;
        *creation_t = out.creation_t;
    }

    return out.retval;
}

int rtu_fdb_proxy_read_next_vlan_entry(
            uint16_t *vid,                                      // inout
            uint8_t *fid,                                       // out
            int *entry_type,                                    // out
            enum registrar_control (*member_set)[NUM_PORTS],    // out
            uint32_t *untagged_set,                             // out
            unsigned long *creation_t)                          // out
{
    int ret;
    struct rtu_fdb_read_next_vlan_entry_argdata in;
    struct rtu_fdb_read_next_vlan_entry_retdata out;

    in.vid = *vid;

    ret = minipc_call(client, MILLISEC_TIMEOUT,
        &rtu_fdb_proxy_read_next_vlan_entry_struct, &in, &out);

    if (ret == 0) {
        *vid = out.vid;
        *fid = out.fid;
        *entry_type = out.entry_type;
        memcpy(*member_set, out.member_set, NUM_PORTS);
        *untagged_set = out.untagged_set;
        *creation_t = out.creation_t;
    }

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
