/*
 * White Rabbit Switch Management
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_snmpd v1.0
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: Handles requests for ieee8021QBridgeTpFdbTable table.
 *              Provides the list of unicast entries currently stored
 *              in the RTU filtering database.
 *              Note: this file originally auto-generated by mib2c using
 *              : mib2c.raw-table.conf 17436 2009-03-31 15:12:19Z dts12 $
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

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "ieee8021QBridgeTpFdbTable.h"
#include "rtu_fd_proxy.h"
#include "utils.h"

#define MIBMOD  "8021Q"

/* column number definitions for table ieee8021QBridgeTpFdbTable */
#define COLUMN_ADDRESS      1
#define COLUMN_PORT         2
#define COLUMN_STATUS       3

// Row entry
struct mib_fdb_table_entry {
    // indexes
    u_long  cid;
    uint8_t fid;
    uint8_t mac[ETH_ALEN];

    // Columns
    uint32_t port_map;
    int type;
};

/**
 * Get indexes for an entry.
 * @param tinfo table information that contains the indexes (in raw format)
 * @param ent (OUT) used to return the retrieved indexes
 */
static int get_indexes(netsnmp_variable_list          *vb,
                        netsnmp_handler_registration   *reginfo,
                        netsnmp_table_request_info     *tinfo,
                        struct mib_fdb_table_entry     *ent)
{
    int oid_len, rootoid_len;
    netsnmp_variable_list *idx;

    // Get indexes from request - in case OID contains them!.
    // Otherwise use default values for first row
    oid_len     = vb->name_length;
    rootoid_len = reginfo->rootoid_len;

    if (oid_len > rootoid_len) {
        if (!tinfo || !tinfo->indexes)
            return SNMP_ERR_GENERR;
        idx = tinfo->indexes;
        ent->cid = *idx->val.integer;
    } else {
        ent->cid = 0;
    }

    if (oid_len > rootoid_len + 1) {
        idx = idx->next_variable;
        ent->fid = *idx->val.integer;
    } else {
        ent->fid = 0;
    }

    if (oid_len > rootoid_len + 2) {
        idx = idx->next_variable;
        memcpy(ent->mac, idx->val.string, idx->val_len);
    } else {
        mac_copy(ent->mac, (uint8_t*)DEFAULT_MAC);
    }
    return SNMP_ERR_NOERROR;
}

static int get_column(netsnmp_variable_list      *vb,
                      int                        colnum,
                      struct mib_fdb_table_entry *ent)
{
    switch (colnum) {
    case COLUMN_PORT:
        snmp_set_var_typed_integer(vb, ASN_UNSIGNED, ent->port_map);
        break;
    case COLUMN_STATUS:
        snmp_set_var_typed_integer(vb, ASN_INTEGER,
            (ent->type == STATIC) ? Mgmt:Learned);
        break;
    default:
        return SNMP_NOSUCHOBJECT;
    }
    return SNMP_ERR_NOERROR;
}

static int get(netsnmp_request_info *req, netsnmp_handler_registration *reginfo)
{
    int err;
    struct mib_fdb_table_entry ent;
    netsnmp_table_request_info *tinfo = netsnmp_extract_table_info(req);

    // Read indexes from request and insert them into ent
    err = get_indexes(req->requestvb, reginfo, tinfo, &ent);
    if (err)
        return err;

    DEBUGMSGTL((MIBMOD, "cid=%lu fid=%d mac=%s column=%d\n",
        ent.cid, ent.fid, mac_to_str(ent.mac), tinfo->colnum));

    if ((ent.cid != DEFAULT_COMPONENT_ID) ||
        (ent.fid >= NUM_FIDS))
        return SNMP_NOSUCHINSTANCE;

    // Read entry from FDB.
    errno = 0;
    err = rtu_fdb_proxy_read_entry(ent.mac, ent.fid, &ent.port_map, &ent.type);
    if (errno)
        goto minipc_err;
    if (err)
        goto entry_not_found;
    return get_column(req->requestvb, tinfo->colnum, &ent);

entry_not_found:
    DEBUGMSGTL((MIBMOD, "entry fid=%d mac=%s not found in fdb\n",
        ent.fid, mac_to_str(ent.mac)));
    return SNMP_NOSUCHINSTANCE;

minipc_err:
    snmp_log(LOG_ERR, "%s(%d): mini-ipc error [%s]\n",
        __FILE__, __LINE__, strerror(errno));
    return SNMP_ERR_GENERR;
}

static int get_next(netsnmp_request_info *req,
    netsnmp_handler_registration *reginfo)
{
    int err;
    struct mib_fdb_table_entry ent;
    netsnmp_variable_list *idx;
    netsnmp_table_request_info *tinfo = netsnmp_extract_table_info(req);

    // Get indexes from request
    err = get_indexes(req->requestvb, reginfo, tinfo, &ent);
    if (err)
        return err;

    DEBUGMSGTL((MIBMOD, "cid=%d fid =%d mac=%s column=%d\n",
        ent.cid, ent.fid, mac_to_str(ent.mac), tinfo->colnum));

    // Get indexes for next entry - SNMP_ENDOFMIBVIEW informs the handler
    // to proceed with next column.
    if (ent.cid > DEFAULT_COMPONENT_ID)
        return SNMP_ENDOFMIBVIEW;
    if (ent.cid == 0) {
        ent.cid = DEFAULT_COMPONENT_ID;
        ent.fid = 0;
        mac_copy(ent.mac, (uint8_t*)DEFAULT_MAC);
    }
    if (ent.fid >= NUM_FIDS)
        return SNMP_ENDOFMIBVIEW;

    do {
        errno = 0;
        err = rtu_fdb_proxy_read_next_entry(
            &ent.mac, &ent.fid, &ent.port_map, &ent.type);
        if (errno)
            goto minipc_err;
        if (err)
            return SNMP_ENDOFMIBVIEW;   // No other entry found
    } while (mac_multicast(ent.mac));   // Make sure entry is unicast

    // Update indexes and OID returned in SNMP response
    idx = tinfo->indexes;
    *idx->val.integer = ent.cid;

    idx = idx->next_variable;
    *idx->val.integer = ent.fid;

    idx = idx->next_variable;
    memcpy(idx->val.string, ent.mac, ETH_ALEN);
    // Update OID
    update_oid(req, reginfo, tinfo->colnum, tinfo->indexes);
    // Return next entry column value
    return get_column(req->requestvb, tinfo->colnum, &ent);

minipc_err:
    snmp_log(LOG_ERR, "%s(%s): mini-ipc error [%s]\n", __FILE__, __func__,
        strerror(errno));
    return SNMP_ERR_GENERR;
}

/**
 * Handles requests for the ieee8021QBridgeTpFdbTable table
 */
static int _handler(netsnmp_mib_handler          *handler,
                    netsnmp_handler_registration *reginfo,
                    netsnmp_agent_request_info   *reqinfo,
                    netsnmp_request_info         *requests)
{
    int err;
    netsnmp_request_info *req;

    switch (reqinfo->mode) {
    case MODE_GET:
        for (req = requests; req; req = req->next) {
            err = get(req, reginfo);
            if (err)
                netsnmp_set_request_error(reqinfo, req, err);
        }
        break;
    case MODE_GETNEXT:
        for (req = requests; req; req = req->next) {
            err = get_next(req, reginfo);
            if (err)
                netsnmp_set_request_error(reqinfo, req, err);
        }
        break;
    }
    return SNMP_ERR_NOERROR;
}

/**
 * Initialize the ieee8021QBridgeTpFdbTable table by defining its
 * contents and how it's structured
 */
static void initialize_table(void)
{
    const oid _oid[] = {1,3,111,2,802,1,1,4,1,2,2};
    netsnmp_handler_registration    *reg;
    netsnmp_table_registration_info *tinfo;
    netsnmp_variable_list           *idx;

    reg = netsnmp_create_handler_registration(
            "ieee8021QBridgeTpFdbTable",
            _handler,
            (oid *)_oid,
            OID_LENGTH(_oid),
            HANDLER_CAN_RONLY);

    tinfo = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    netsnmp_table_helper_add_indexes(
            tinfo,
            ASN_UNSIGNED,  /* index: ComponentId */
            ASN_UNSIGNED,  /* index: FdbId */
            ASN_PRIV_IMPLIED_OCTET_STR, /* index: Address */
            0);

    // Fix the MacAddress Index variable binding lenght
    idx = tinfo->indexes;
    idx = idx->next_variable; // skip componentId
    idx = idx->next_variable; // skip FdbId
    idx->val_len = ETH_ALEN;

    tinfo->min_column = COLUMN_PORT;
    tinfo->max_column = COLUMN_STATUS;

    netsnmp_register_table(reg, tinfo);
}

/**
 * Initializes the ieee8021QBridgeTpFdbTable module
 */
void init_ieee8021QBridgeTpFdbTable(void)
{
    struct minipc_ch *client;

    client = rtu_fdb_proxy_create("rtu_fdb");
    if(client) {
        initialize_table();
        snmp_log(LOG_INFO, "%s: initialised\n", __FILE__);
    } else {
        snmp_log(LOG_ERR, "%s: error creating mini-ipc proxy - %s\n", __FILE__,
            strerror(errno));
    }
}
