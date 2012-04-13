/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_rtud v2.0
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: RTU Filtering Database RPC data structures.
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
#include "minipc.h"

#include "rtu_fd_structs.h"

struct minipc_pd rtu_fdb_get_max_vid_struct = {
    .name   = "get_max_vid",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_max_vid_retdata),
    .args   = {
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_get_max_supported_vlans_struct = {
    .name   = "get_max_vlans",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_max_supported_vlans_retdata),
    .args   = {
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_get_num_vlans_struct = {
    .name   = "get_num_vlans",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_vlans_retdata),
    .args   = {
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_get_num_dynamic_entries_struct = {
    .name   = "get_num_dyn_entries",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_dynamic_entries_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_dynamic_entries_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_get_num_learned_entry_discards_struct = {
    .name   = "get_num_discards",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_learned_entry_discards_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_learned_entry_discards_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_get_num_vlan_deletes_struct = {
    .name   = "get_num_vlan_del",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_num_vlan_deletes_retdata),
    .args   = {
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_get_aging_time_struct = {
    .name   = "get_aging_time",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_aging_time_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_aging_time_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_set_aging_time_struct = {
    .name   = "set_aging_time",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_set_aging_time_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_read_entry_struct = {
    .name   = "read_mac",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_entry_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_read_next_entry_struct = {
    .name   = "read_nxt_mac",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_entry_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_create_static_entry_struct = {
    .name   = "create_static_mac",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_create_static_entry_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_read_static_entry_struct = {
    .name   = "read_static_mac",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_static_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_static_entry_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_read_next_static_entry_struct = {
    .name   = "read_nxt_static_mac",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_static_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_static_entry_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_delete_static_entry_struct = {
    .name   = "delete_static_mac",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_delete_static_entry_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_get_next_fid_struct = {
    .name   = "get_nxt_fid",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_next_fid_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_next_fid_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_create_static_vlan_entry_struct = {
    .name   = "create_static_vlan",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_create_static_vlan_entry_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_delete_static_vlan_entry_struct = {
    .name   = "delete_static_vlan",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_delete_static_vlan_entry_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_read_static_vlan_entry_struct = {
    .name   = "read_static_vlan",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_static_vlan_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_static_vlan_entry_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_read_next_static_vlan_entry_struct = {
    .name   = "read_nxt_stat_vlan",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_static_vlan_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_static_vlan_entry_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_read_vlan_entry_struct = {
    .name   = "read_vlan",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_vlan_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_vlan_entry_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_read_next_vlan_entry_struct = {
    .name   = "read_nxt_vlan",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_vlan_entry_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_vlan_entry_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_vfdb_forward_dynamic_struct = {
    .name   = "forward_dyn_vlan",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_vfdb_forward_dynamic_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_vfdb_filter_dynamic_struct = {
    .name   = "filter_dyn_vlan",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_vfdb_filter_dynamic_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_delete_dynamic_entries_struct = {
    .name   = "delete_dyn_macs",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_delete_dynamic_entries_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_is_restricted_vlan_reg_struct = {
    .name   = "is_restricted_vlan",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_is_restricted_vlan_reg_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_set_restricted_vlan_reg_struct = {
    .name   = "set_restricted_vlan",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_set_restricted_vlan_reg_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_unset_restricted_vlan_reg_struct = {
    .name   = "unset_restrict_vlan",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_unset_restricted_vlan_reg_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_get_size_struct = {
    .name   = "get_size",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_get_num_all_static_entries_struct = {
    .name   = "get_num_static_macs",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_get_num_all_dynamic_entries_struct = {
    .name   = "get_num_dyn_macs",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_vfdb_get_num_all_static_entries_struct = {
    .name   = "get_num_static_vlan",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_vfdb_get_num_all_dynamic_entries_struct = {
    .name   = "get_num_dyn_vlans",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_create_lc_struct = {
    .name   = "create_lc",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_create_lc_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_delete_lc_struct = {
    .name   = "delete_lc",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_delete_lc_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_read_lc_struct = {
    .name   = "read_lc",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_lc_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_lc_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_read_next_lc_struct = {
    .name   = "read_nxt_lc",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_lc_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_lc_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_read_lc_set_type_struct = {
    .name   = "read_lc_set_type",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_lc_set_type_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_set_default_lc_struct = {
    .name   = "set_default_lc",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_get_default_lc_struct = {
    .name   = "get_default_lc",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_get_default_lc_retdata),
    .args   = {
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_read_fid_struct = {
    .name   = "read_fid",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_fid_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_fid_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_read_next_fid_struct = {
    .name   = "read_nxt_fid",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_fid_retdata),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_read_next_fid_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_set_fid_struct = {
    .name   = "set_fid",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_set_fid_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_delete_fid_struct = {
    .name   = "delete_fid",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT,
                struct rtu_fdb_delete_fid_argdata),
        MINIPC_ARG_END,
    }
};

struct minipc_pd rtu_fdb_set_default_lc_type_struct = {
    .name   = "set_default_lc_type",
    .retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
    .args   = {
        MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
        MINIPC_ARG_END,
    }
};
