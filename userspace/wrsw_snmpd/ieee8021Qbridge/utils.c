/*
 * White Rabbit Switch Management
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_snmpd v1.0
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: OID manipulation routines
 *
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
#include "utils.h"


/**
 * Update the requested OID to match a given instance
 */
void update_oid(netsnmp_request_info           *req,
                netsnmp_handler_registration   *reginfo,
                int                            column,
                netsnmp_variable_list          *indexes)
{
    oid    build_space[MAX_OID_LEN];
    size_t build_space_len = 0;
    size_t index_oid_len = 0;

    memcpy(build_space, reginfo->rootoid,   /* registered oid */
                        reginfo->rootoid_len * sizeof(oid));
    build_space_len = reginfo->rootoid_len;
    build_space[build_space_len++] = 1;         /* entry */
    build_space[build_space_len++] = column;    /* column */
    build_oid_noalloc(build_space + build_space_len,
                      MAX_OID_LEN - build_space_len, &index_oid_len,
                      NULL, 0, indexes);
    snmp_set_var_objid(req->requestvb, build_space,
                       build_space_len + index_oid_len);
}
