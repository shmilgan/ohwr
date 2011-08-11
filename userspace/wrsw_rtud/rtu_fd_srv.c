/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_rtud v2.0
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: RTU Filtering Database Server.
 *              Provides remote method invocation to the Filtering Database
 *              based on the mini_ipc framework.
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

#include "rtu_fd_srv.h"
#include "rtu_fd_export.h"

static int rtu_fdb_srv_get_max_vid(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_get_max_vid_argdata *in;
    struct rtu_fdb_get_max_vid_retdata *out;
    in  = (struct rtu_fdb_get_max_vid_argdata*)args;
    out = (struct rtu_fdb_get_max_vid_retdata*)ret;

    out->retval = rtu_fdb_get_max_vid();

    return 0;
}

static int rtu_fdb_srv_get_max_supported_vlans(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_get_max_supported_vlans_argdata *in;
    struct rtu_fdb_get_max_supported_vlans_retdata *out;
    in  = (struct rtu_fdb_get_max_supported_vlans_argdata*)args;
    out = (struct rtu_fdb_get_max_supported_vlans_retdata*)ret;

    out->retval = rtu_fdb_get_max_supported_vlans();

    return 0;
}

static int rtu_fdb_srv_get_num_vlans(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_get_num_vlans_argdata *in;
    struct rtu_fdb_get_num_vlans_retdata *out;
    in  = (struct rtu_fdb_get_num_vlans_argdata*)args;
    out = (struct rtu_fdb_get_num_vlans_retdata*)ret;

    out->retval = rtu_fdb_get_num_vlans();

    return 0;
}

static int rtu_fdb_srv_get_num_dynamic_entries(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_get_num_dynamic_entries_argdata *in;
    struct rtu_fdb_get_num_dynamic_entries_retdata *out;
    in  = (struct rtu_fdb_get_num_dynamic_entries_argdata*)args;
    out = (struct rtu_fdb_get_num_dynamic_entries_retdata*)ret;

    out->retval = rtu_fdb_get_num_dynamic_entries(in->fid);

    return 0;
}

static int rtu_fdb_srv_get_num_learned_entry_discards(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_get_num_learned_entry_discards_argdata *in;
    struct rtu_fdb_get_num_learned_entry_discards_retdata *out;
    in  = (struct rtu_fdb_get_num_learned_entry_discards_argdata*)args;
    out = (struct rtu_fdb_get_num_learned_entry_discards_retdata*)ret;

    out->retval = rtu_fdb_get_num_learned_entry_discards(in->fid);

    return 0;
}

static int rtu_fdb_srv_get_num_vlan_deletes(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_get_num_vlan_deletes_argdata *in;
    struct rtu_fdb_get_num_vlan_deletes_retdata *out;
    in  = (struct rtu_fdb_get_num_vlan_deletes_argdata*)args;
    out = (struct rtu_fdb_get_num_vlan_deletes_retdata*)ret;

    out->retval = rtu_fdb_get_num_vlan_deletes();

    return 0;
}

static int rtu_fdb_srv_get_aging_time(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_get_aging_time_argdata *in;
    struct rtu_fdb_get_aging_time_retdata *out;
    in  = (struct rtu_fdb_get_aging_time_argdata*)args;
    out = (struct rtu_fdb_get_aging_time_retdata*)ret;

    out->retval = rtu_fdb_get_aging_time(in->fid);

    return 0;
}

static int rtu_fdb_srv_set_aging_time(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_set_aging_time_argdata *in;
    struct rtu_fdb_set_aging_time_retdata *out;
    in  = (struct rtu_fdb_set_aging_time_argdata*)args;
    out = (struct rtu_fdb_set_aging_time_retdata*)ret;

    out->retval = rtu_fdb_set_aging_time(in->fid, in->t);

    return 0;
}

static int rtu_fdb_srv_read_entry(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_read_entry_argdata *in;
    struct rtu_fdb_read_entry_retdata *out;
    in  = (struct rtu_fdb_read_entry_argdata*)args;
    out = (struct rtu_fdb_read_entry_retdata*)ret;

    out->retval =
        rtu_fdb_read_entry(
            in->mac,
            in->fid,
            &out->port_map,
            &out->entry_type
        );

    return 0;
}

static int rtu_fdb_srv_read_next_entry(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_read_next_entry_argdata *in;
    struct rtu_fdb_read_next_entry_retdata *out;
    in  = (struct rtu_fdb_read_next_entry_argdata*)args;
    out = (struct rtu_fdb_read_next_entry_retdata*)ret;

    out->retval =
        rtu_fdb_read_next_entry(
            &in->mac,
            &in->fid,
            &out->port_map,
            &out->entry_type
        );
    mac_copy(out->mac, in->mac);
    out->fid = in->fid;

    return 0;
}

static int rtu_fdb_srv_create_static_entry(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_create_static_entry_argdata *in;
    struct rtu_fdb_create_static_entry_retdata *out;
    in  = (struct rtu_fdb_create_static_entry_argdata*)args;
    out = (struct rtu_fdb_create_static_entry_retdata*)ret;

    out->retval =
        rtu_fdb_create_static_entry(
            in->mac,
            in->vid,
            in->port_map,
            in->type,
            in->active
        );

    return 0;
}

static int rtu_fdb_srv_read_static_entry(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_read_static_entry_argdata *in;
    struct rtu_fdb_read_static_entry_retdata *out;
    in  = (struct rtu_fdb_read_static_entry_argdata*)args;
    out = (struct rtu_fdb_read_static_entry_retdata*)ret;

    out->retval =
        rtu_fdb_read_static_entry(
            in->mac,
            in->vid,
            &out->port_map,
            &out->type,
            &out->active
        );

    return 0;
}

static int rtu_fdb_srv_read_next_static_entry(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_read_next_static_entry_argdata *in;
    struct rtu_fdb_read_next_static_entry_retdata *out;
    in  = (struct rtu_fdb_read_next_static_entry_argdata*)args;
    out = (struct rtu_fdb_read_next_static_entry_retdata*)ret;

    out->retval =
        rtu_fdb_read_next_static_entry(
            &in->mac,
            &in->vid,
            &out->port_map,
            &out->type,
            &out->active
        );
    mac_copy(out->mac, in->mac);
    out->vid = in->vid;

    return 0;
}

static int rtu_fdb_srv_delete_static_entry(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_delete_static_entry_argdata *in;
    struct rtu_fdb_delete_static_entry_retdata *out;
    in  = (struct rtu_fdb_delete_static_entry_argdata*)args;
    out = (struct rtu_fdb_delete_static_entry_retdata*)ret;

    out->retval = rtu_fdb_delete_static_entry(in->mac, in->vid);

    return 0;
}

static int rtu_fdb_srv_get_next_fid(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_get_next_fid_argdata *in;
    struct rtu_fdb_get_next_fid_retdata *out;
    in  = (struct rtu_fdb_get_next_fid_argdata*)args;
    out = (struct rtu_fdb_get_next_fid_retdata*)ret;

    out->retval = rtu_fdb_get_next_fid(in->fid);

    return 0;
}

const struct minipc_pd rtu_fdb_srv_get_max_vid_struct = {
    .f      = rtu_fdb_srv_get_max_vid,
    .name   = "0",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_max_vid_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_max_vid_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_srv_get_max_supported_vlans_struct = {
    .f      = rtu_fdb_srv_get_max_supported_vlans,
    .name   = "1",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_max_supported_vlans_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_max_supported_vlans_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_srv_get_num_vlans_struct = {
    .f      = rtu_fdb_srv_get_num_vlans,
    .name   = "2",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_vlans_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_vlans_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_srv_get_num_dynamic_entries_struct = {
    .f      = rtu_fdb_srv_get_num_dynamic_entries,
    .name   = "3",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_dynamic_entries_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_dynamic_entries_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_srv_get_num_learned_entry_discards_struct = {
    .f      = rtu_fdb_srv_get_num_learned_entry_discards,
    .name   = "4",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_learned_entry_discards_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_learned_entry_discards_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_srv_get_num_vlan_deletes_struct = {
    .f      = rtu_fdb_srv_get_num_vlan_deletes,
    .name   = "5",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_vlan_deletes_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_vlan_deletes_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_srv_get_aging_time_struct = {
    .f      = rtu_fdb_srv_get_aging_time,
    .name   = "6",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_aging_time_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_aging_time_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_srv_set_aging_time_struct = {
    .f      = rtu_fdb_srv_set_aging_time,
    .name   = "7",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_set_aging_time_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_set_aging_time_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_srv_read_entry_struct = {
    .f      = rtu_fdb_srv_read_entry,
    .name   = "8",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_entry_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_srv_read_next_entry_struct = {
    .f      = rtu_fdb_srv_read_next_entry,
    .name   = "9",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_entry_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_srv_create_static_entry_struct = {
    .f      = rtu_fdb_srv_create_static_entry,
    .name   = "10",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_create_static_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_create_static_entry_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_srv_read_static_entry_struct = {
    .f      = rtu_fdb_srv_read_static_entry,
    .name   = "11",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_static_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_static_entry_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_srv_read_next_static_entry_struct = {
    .f      = rtu_fdb_srv_read_next_static_entry,
    .name   = "12",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_static_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_static_entry_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_srv_delete_static_entry_struct = {
    .f      = rtu_fdb_srv_delete_static_entry,
    .name   = "13",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_delete_static_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_delete_static_entry_argdata),
        MINIPC_ARG_END,
    }
};

const struct minipc_pd rtu_fdb_srv_get_next_fid_struct = {
    .f      = rtu_fdb_srv_get_next_fid,
    .name   = "14",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_next_fid_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_next_fid_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_ch *rtu_fdb_srv_create(char *name)
{
	struct minipc_ch *server;

	server = minipc_server_create(name, 0);
    if (server) {
	    minipc_set_logfile(server, stderr);

	    minipc_export(server, &rtu_fdb_srv_get_max_vid_struct);
	    minipc_export(server, &rtu_fdb_srv_get_max_supported_vlans_struct);
	    minipc_export(server, &rtu_fdb_srv_get_num_vlans_struct);
	    minipc_export(server, &rtu_fdb_srv_get_num_dynamic_entries_struct);
	    minipc_export(server, &rtu_fdb_srv_get_num_learned_entry_discards_struct);
	    minipc_export(server, &rtu_fdb_srv_get_num_vlan_deletes_struct);
	    minipc_export(server, &rtu_fdb_srv_get_aging_time_struct);
	    minipc_export(server, &rtu_fdb_srv_set_aging_time_struct);
	    minipc_export(server, &rtu_fdb_srv_read_entry_struct);
	    minipc_export(server, &rtu_fdb_srv_read_next_entry_struct);
	    minipc_export(server, &rtu_fdb_srv_create_static_entry_struct);
	    minipc_export(server, &rtu_fdb_srv_read_static_entry_struct);
	    minipc_export(server, &rtu_fdb_srv_delete_static_entry_struct);
	    minipc_export(server, &rtu_fdb_srv_get_next_fid_struct);
    }
    return server;
}


