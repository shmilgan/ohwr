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
#include "rtu_fd_structs.h"

#include "rtu_sw.h"

static int rtu_fdb_srv_get_max_vid(const struct minipc_pd *pd,
                                   uint32_t *args, void *ret)
{
    struct rtu_fdb_get_max_vid_retdata *out;
    out = (struct rtu_fdb_get_max_vid_retdata*)ret;

    out->retval = rtu_fdb_get_max_vid();

    return 0;
}

static int rtu_fdb_srv_get_max_supported_vlans(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_get_max_supported_vlans_retdata *out;
    out = (struct rtu_fdb_get_max_supported_vlans_retdata*)ret;

    out->retval = rtu_fdb_get_max_supported_vlans();

    return 0;
}

static int rtu_fdb_srv_get_num_vlans(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_get_num_vlans_retdata *out;
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
    struct rtu_fdb_get_num_vlan_deletes_retdata *out;
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
    in  = (struct rtu_fdb_set_aging_time_argdata*)args;

    *(int *)ret = rtu_fdb_set_aging_time(in->fid, in->t);

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
    in  = (struct rtu_fdb_create_static_entry_argdata*)args;

    *(int *)ret =
        rtu_fdb_create_static_entry(
            in->mac,
            in->vid,
            in->egress_ports,
            in->forbidden_ports,
            in->type,
            in->active,
            in->is_bpdu
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
            &out->egress_ports,
            &out->forbidden_ports,
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
            &out->egress_ports,
            &out->forbidden_ports,
            &out->type,
            &out->active
        );

    mac_copy(out->mac, in->mac);
    out->vid    = in->vid;

    return 0;
}

static int rtu_fdb_srv_delete_static_entry(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_delete_static_entry_argdata *in;
    in  = (struct rtu_fdb_delete_static_entry_argdata*)args;

    *(int *)ret = rtu_fdb_delete_static_entry(in->mac, in->vid);

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

static int rtu_fdb_srv_create_static_vlan_entry(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_create_static_vlan_entry_argdata *in;
    in  = (struct rtu_fdb_create_static_vlan_entry_argdata*)args;

    *(int *)ret =
        rtu_fdb_create_static_vlan_entry(
            in->vid,
            in->egress_ports,
            in->forbidden_ports,
            in->untagged_set
        );

    return 0;
}

static int rtu_fdb_srv_delete_static_vlan_entry(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_delete_static_vlan_entry_argdata *in;
    in  = (struct rtu_fdb_delete_static_vlan_entry_argdata*)args;

    *(int *)ret = rtu_fdb_delete_static_vlan_entry(in->vid);

    return 0;
}

static int rtu_fdb_srv_read_static_vlan_entry(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_read_static_vlan_entry_argdata *in;
    struct rtu_fdb_read_static_vlan_entry_retdata *out;
    in  = (struct rtu_fdb_read_static_vlan_entry_argdata*)args;
    out = (struct rtu_fdb_read_static_vlan_entry_retdata*)ret;

    out->retval =
        rtu_fdb_read_static_vlan_entry(
            in->vid,
            &out->egress_ports,
            &out->forbidden_ports,
            &out->untagged_set
        );

    return 0;
}

static int rtu_fdb_srv_read_next_static_vlan_entry(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_read_next_static_vlan_entry_argdata *in;
    struct rtu_fdb_read_next_static_vlan_entry_retdata *out;
    in  = (struct rtu_fdb_read_next_static_vlan_entry_argdata*)args;
    out = (struct rtu_fdb_read_next_static_vlan_entry_retdata*)ret;

    out->retval =
        rtu_fdb_read_next_static_vlan_entry(
            &in->vid,
            &out->egress_ports,
            &out->forbidden_ports,
            &out->untagged_set
        );

    out->vid = in->vid;

    return 0;
}

static int rtu_fdb_srv_read_vlan_entry(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_read_vlan_entry_argdata *in;
    struct rtu_fdb_read_vlan_entry_retdata *out;
    in  = (struct rtu_fdb_read_vlan_entry_argdata*)args;
    out = (struct rtu_fdb_read_vlan_entry_retdata*)ret;

    out->retval =
        rtu_fdb_read_vlan_entry(
            in->vid,
            &out->fid,
            &out->entry_type,
            &out->port_mask,
            &out->untagged_set,
            &out->creation_t
        );

    return 0;
}

static int rtu_fdb_srv_read_next_vlan_entry(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_read_next_vlan_entry_argdata *in;
    struct rtu_fdb_read_next_vlan_entry_retdata *out;
    in  = (struct rtu_fdb_read_next_vlan_entry_argdata*)args;
    out = (struct rtu_fdb_read_next_vlan_entry_retdata*)ret;

    out->retval =
        rtu_fdb_read_next_vlan_entry(
            &in->vid,
            &out->fid,
            &out->entry_type,
            &out->port_mask,
            &out->untagged_set,
            &out->creation_t
        );

    out->vid = in->vid;

    return 0;
}

static int rtu_vfdb_srv_forward_dynamic(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_vfdb_forward_dynamic_argdata *in;
    in  = (struct rtu_vfdb_forward_dynamic_argdata*)args;

    *(int *)ret =
        rtu_vfdb_forward_dynamic(
            in->port,
            in->vid
        );

    return 0;
}

static int rtu_vfdb_srv_filter_dynamic(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_vfdb_filter_dynamic_argdata *in;
    in  = (struct rtu_vfdb_filter_dynamic_argdata*)args;

    *(int *)ret =
        rtu_vfdb_filter_dynamic(
            in->port,
            in->vid
        );

    return 0;
}

static int rtu_fdb_srv_delete_dynamic_entries(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_delete_dynamic_entries_argdata *in;
    in  = (struct rtu_fdb_delete_dynamic_entries_argdata*)args;

    rtu_fdb_delete_dynamic_entries(
            in->port,
            in->vid
        );

    return 0;
}

static int rtu_fdb_srv_is_restricted_vlan_reg(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_is_restricted_vlan_reg_argdata *in;
    in  = (struct rtu_fdb_is_restricted_vlan_reg_argdata*)args;

    *(int *)ret = rtu_fdb_is_restricted_vlan_reg(in->port);

    return 0;
}

static int rtu_fdb_srv_set_restricted_vlan_reg(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_set_restricted_vlan_reg_argdata *in;
    in  = (struct rtu_fdb_set_restricted_vlan_reg_argdata*)args;

    *(int *)ret = rtu_fdb_set_restricted_vlan_reg(in->port);

    return 0;
}

static int rtu_fdb_srv_unset_restricted_vlan_reg(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_unset_restricted_vlan_reg_argdata *in;
    in  = (struct rtu_fdb_unset_restricted_vlan_reg_argdata*)args;

    *(int *)ret = rtu_fdb_unset_restricted_vlan_reg(in->port);

    return 0;
}

static int rtu_fdb_srv_get_size(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    *(int *)ret = rtu_fdb_get_size();
    return 0;
}

static int rtu_fdb_srv_get_num_all_static_entries(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    *(int *)ret = rtu_fdb_get_num_all_static_entries();
    return 0;
}

static int rtu_fdb_srv_get_num_all_dynamic_entries(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    *(int *)ret = rtu_fdb_get_num_all_dynamic_entries();
    return 0;
}

static int rtu_vfdb_srv_get_num_all_static_entries(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    *(int *)ret = rtu_vfdb_get_num_all_static_entries();
    return 0;
}

static int rtu_vfdb_srv_get_num_all_dynamic_entries(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    *(int *)ret = rtu_vfdb_get_num_all_dynamic_entries();
    return 0;
}

static int rtu_fdb_srv_create_lc(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_create_lc_argdata *in;
    in  = (struct rtu_fdb_create_lc_argdata*)args;

    *(int *)ret =
        rtu_fdb_create_lc(
            in->sid,
            in->vid,
            in->lc_type
        );

    return 0;
}

static int rtu_fdb_srv_delete_lc(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_delete_lc_argdata *in;
    in  = (struct rtu_fdb_delete_lc_argdata*)args;

    *(int *)ret =
        rtu_fdb_delete_lc(
            in->sid,
            in->vid
        );

    return 0;
}

static int rtu_fdb_srv_read_lc(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_read_lc_argdata *in;
    struct rtu_fdb_read_lc_retdata *out;
    in  = (struct rtu_fdb_read_lc_argdata*)args;
    out = (struct rtu_fdb_read_lc_retdata*)ret;

    out->retval =
        rtu_fdb_read_lc(
            in->vid,
            &out->lc_set
        );

    return 0;
}

static int rtu_fdb_srv_read_next_lc(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_read_next_lc_argdata *in;
    struct rtu_fdb_read_next_lc_retdata *out;
    in  = (struct rtu_fdb_read_next_lc_argdata*)args;
    out = (struct rtu_fdb_read_next_lc_retdata*)ret;

    out->retval =
        rtu_fdb_read_next_lc(
            &in->vid,
            &out->lc_set
        );

    out->vid = in->vid;

    return 0;
}

static int rtu_fdb_srv_read_lc_set_type(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    int sid = args[0];
    struct rtu_fdb_read_lc_set_type_retdata *out;
    out = (struct rtu_fdb_read_lc_set_type_retdata*)ret;

    out->retval =
        rtu_fdb_read_lc_set_type(
            sid,
            &out->lc_type
        );

    return 0;
}

static int rtu_fdb_srv_set_default_lc(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    int sid = args[0];

    *(int *)ret = rtu_fdb_set_default_lc(sid);
    return 0;
}

static int rtu_fdb_srv_get_default_lc(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_get_default_lc_retdata *out;
    out = (struct rtu_fdb_get_default_lc_retdata*)ret;

    rtu_fdb_get_default_lc(
        &out->sid,
        &out->lc_type
    );

    return 0;
}

static int rtu_fdb_srv_read_fid(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_read_fid_argdata *in;
    struct rtu_fdb_read_fid_retdata *out;
    in  = (struct rtu_fdb_read_fid_argdata*)args;
    out = (struct rtu_fdb_read_fid_retdata*)ret;

    rtu_fdb_read_fid(
        in->vid,
        &out->fid,
        &out->fid_fixed
    );

    return 0;
}

static int rtu_fdb_srv_read_next_fid(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_read_next_fid_argdata *in;
    struct rtu_fdb_read_next_fid_retdata *out;
    in  = (struct rtu_fdb_read_next_fid_argdata*)args;
    out = (struct rtu_fdb_read_next_fid_retdata*)ret;

    out->retval =
        rtu_fdb_read_next_fid(
            &in->vid,
            &out->fid,
            &out->fid_fixed
        );

    out->vid = in->vid;

    return 0;
}

static int rtu_fdb_srv_set_fid(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_set_fid_argdata *in;
    in  = (struct rtu_fdb_set_fid_argdata*)args;

    *(int *)ret = rtu_fdb_set_fid(in->vid, in->fid);
    return 0;
}

static int rtu_fdb_srv_delete_fid(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    struct rtu_fdb_delete_fid_argdata *in;
    in  = (struct rtu_fdb_delete_fid_argdata*)args;

    *(int *)ret = rtu_fdb_delete_fid(in->vid);
    return 0;
}

static int rtu_fdb_srv_set_default_lc_type(
            const struct minipc_pd *pd, uint32_t *args, void *ret)
{
    int lc_type = args[0];

    *(int *)ret = rtu_fdb_set_default_lc_type(lc_type);
    return 0;
}

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

struct minipc_ch *rtu_fdb_srv_create(char *name)
{
    int i;
	struct minipc_ch *server;


    static struct {
        struct minipc_pd *desc;
        minipc_f *f;
    } export_list [] = {
        {&rtu_fdb_get_max_vid_struct, rtu_fdb_srv_get_max_vid},
        {&rtu_fdb_get_max_supported_vlans_struct, rtu_fdb_srv_get_max_supported_vlans},
        {&rtu_fdb_get_num_vlans_struct, rtu_fdb_srv_get_num_vlans},
        {&rtu_fdb_get_num_dynamic_entries_struct, rtu_fdb_srv_get_num_dynamic_entries},
        {&rtu_fdb_get_num_learned_entry_discards_struct, rtu_fdb_srv_get_num_learned_entry_discards},
        {&rtu_fdb_get_num_vlan_deletes_struct, rtu_fdb_srv_get_num_vlan_deletes},
        {&rtu_fdb_get_aging_time_struct, rtu_fdb_srv_get_aging_time},
        {&rtu_fdb_set_aging_time_struct, rtu_fdb_srv_set_aging_time},
        {&rtu_fdb_read_entry_struct, rtu_fdb_srv_read_entry},
        {&rtu_fdb_read_next_entry_struct, rtu_fdb_srv_read_next_entry},
        {&rtu_fdb_create_static_entry_struct, rtu_fdb_srv_create_static_entry},
        {&rtu_fdb_read_static_entry_struct, rtu_fdb_srv_read_static_entry},
        {&rtu_fdb_read_next_static_entry_struct, rtu_fdb_srv_read_next_static_entry},
        {&rtu_fdb_delete_static_entry_struct, rtu_fdb_srv_delete_static_entry},
        {&rtu_fdb_get_next_fid_struct, rtu_fdb_srv_get_next_fid},
        {&rtu_fdb_create_static_vlan_entry_struct, rtu_fdb_srv_create_static_vlan_entry},
        {&rtu_fdb_delete_static_vlan_entry_struct, rtu_fdb_srv_delete_static_vlan_entry},
        {&rtu_fdb_read_static_vlan_entry_struct, rtu_fdb_srv_read_static_vlan_entry},
        {&rtu_fdb_read_next_static_vlan_entry_struct, rtu_fdb_srv_read_next_static_vlan_entry},
        {&rtu_fdb_read_vlan_entry_struct, rtu_fdb_srv_read_vlan_entry},
        {&rtu_fdb_read_next_vlan_entry_struct, rtu_fdb_srv_read_next_vlan_entry},
        {&rtu_vfdb_forward_dynamic_struct, rtu_vfdb_srv_forward_dynamic},
        {&rtu_vfdb_filter_dynamic_struct, rtu_vfdb_srv_filter_dynamic},
        {&rtu_fdb_delete_dynamic_entries_struct, rtu_fdb_srv_delete_dynamic_entries},
        {&rtu_fdb_is_restricted_vlan_reg_struct, rtu_fdb_srv_is_restricted_vlan_reg},
        {&rtu_fdb_set_restricted_vlan_reg_struct, rtu_fdb_srv_set_restricted_vlan_reg},
        {&rtu_fdb_unset_restricted_vlan_reg_struct, rtu_fdb_srv_unset_restricted_vlan_reg},
        {&rtu_fdb_get_size_struct, rtu_fdb_srv_get_size},
        {&rtu_fdb_get_num_all_static_entries_struct, rtu_fdb_srv_get_num_all_static_entries},
        {&rtu_fdb_get_num_all_dynamic_entries_struct, rtu_fdb_srv_get_num_all_dynamic_entries},
        {&rtu_fdb_get_num_all_static_entries_struct, rtu_fdb_srv_get_num_all_static_entries},
        {&rtu_fdb_get_num_all_dynamic_entries_struct, rtu_fdb_srv_get_num_all_dynamic_entries},
        {&rtu_fdb_create_lc_struct, rtu_fdb_srv_create_lc},
        {&rtu_fdb_delete_lc_struct, rtu_fdb_srv_delete_lc},
        {&rtu_fdb_read_lc_struct, rtu_fdb_srv_read_lc},
        {&rtu_fdb_read_next_lc_struct, rtu_fdb_srv_read_next_lc},
        {&rtu_fdb_read_lc_set_type_struct, rtu_fdb_srv_read_lc_set_type},
        {&rtu_fdb_set_default_lc_struct, rtu_fdb_srv_set_default_lc},
        {&rtu_fdb_get_default_lc_struct, rtu_fdb_srv_get_default_lc},
        {&rtu_fdb_read_fid_struct, rtu_fdb_srv_read_fid},
        {&rtu_fdb_read_next_fid_struct, rtu_fdb_srv_read_next_fid},
        {&rtu_fdb_set_fid_struct, rtu_fdb_srv_set_fid},
        {&rtu_fdb_delete_fid_struct, rtu_fdb_srv_delete_fid},
        {&rtu_fdb_set_default_lc_type_struct, rtu_fdb_srv_set_default_lc_type},
        {&rtu_vfdb_get_num_all_static_entries_struct, rtu_vfdb_srv_get_num_all_static_entries},
        {&rtu_vfdb_get_num_all_dynamic_entries_struct, rtu_vfdb_srv_get_num_all_dynamic_entries},
    };

	server = minipc_server_create(name, 0);
    if (server) {
	    minipc_set_logfile(server, stderr);

	    for (i = 0; i < ARRAY_SIZE(export_list); i++) {
		    export_list[i].desc->f = export_list[i].f;
		    if (minipc_export(server, export_list[i].desc) < 0)
		        return NULL;
        }

   }
   return server;
}
