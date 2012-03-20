/*
 * White Rabbit Switch Management
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_snmpd v1.0
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: Handles requests for ieee8021QVlanStaticTable table.
 *              Provides the list of VLAN entries currently stored in the VLAN
 *              table.
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

#include "ieee8021QBridgeVlanCurrentTable.h"
#include "rtu_fd_proxy.h"
#include "utils.h"

#define MIBMOD  "8021Q"

/* column number definitions for table ieee8021QBridgeVlanCurrentTable */
#define COLUMN_IEEE8021QBRIDGEVLANTIMEMARK                  1
#define COLUMN_IEEE8021QBRIDGEVLANCURRENTCOMPONENTID        2
#define COLUMN_IEEE8021QBRIDGEVLANINDEX                     3
#define COLUMN_IEEE8021QBRIDGEVLANFDBID                     4
#define COLUMN_IEEE8021QBRIDGEVLANCURRENTEGRESSPORTS        5
#define COLUMN_IEEE8021QBRIDGEVLANCURRENTUNTAGGEDPORTS      6
#define COLUMN_IEEE8021QBRIDGEVLANSTATUS                    7
#define COLUMN_IEEE8021QBRIDGEVLANCREATIONTIME              8

enum vlan_entry_status {
    Other_vlan      = 1,
    Permanent_vlan  = 2,
    Dynamic_mvrp    = 3
};

// Row entry
struct mib_vlan_table_entry {
    /* Index values */
    u_long time_mark;
    u_long cid;
    uint16_t vid;

    /* Column values */
    uint8_t fid;
    uint32_t port_mask;
    uint32_t untagged_set;
    u_long creation_t;
};

/**
 * Get indexes for an entry.
 * @param tinfo table information that contains the indexes (in raw format)
 * @param ent (OUT) used to return the retrieved indexes
 */
static int get_indexes(netsnmp_variable_list           *vb,
                        netsnmp_handler_registration    *reginfo,
                        netsnmp_table_request_info      *tinfo,
                        struct mib_vlan_table_entry     *ent)
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
        ent->time_mark = *idx->val.integer;
    } else {
        ent->time_mark = 0;
    }

    if (oid_len > rootoid_len + 1) {
        idx = idx->next_variable;
        ent->cid = *idx->val.integer;
    } else {
        ent->cid = 0;
    }

    if (oid_len > rootoid_len + 2) {
        idx = idx->next_variable;
        ent->vid = *idx->val.integer;
    } else {
        ent->vid = 0;
    }
    return SNMP_ERR_NOERROR;
}

static int get_column(netsnmp_variable_list         *vb,
                      int                           colnum,
                      struct mib_vlan_table_entry   *ent)
{
    char ep[NUM_PORTS]; // egress ports
    char up[NUM_PORTS]; // untagged ports

    switch (colnum) {
    case COLUMN_IEEE8021QBRIDGEVLANFDBID:
        snmp_set_var_typed_integer(vb, ASN_UNSIGNED, ent->fid);
        break;
    case COLUMN_IEEE8021QBRIDGEVLANCURRENTEGRESSPORTS:
        to_octetstr(ent->port_mask, ep);
        snmp_set_var_typed_value(vb, ASN_OCTET_STR, ep, NUM_PORTS);
        break;
    case COLUMN_IEEE8021QBRIDGEVLANCURRENTUNTAGGEDPORTS:
        to_octetstr(ent->untagged_set, up);
        snmp_set_var_typed_value(vb, ASN_OCTET_STR, up, NUM_PORTS);
        break;
    case COLUMN_IEEE8021QBRIDGEVLANSTATUS:
        // TODO review once MVRP is supported
        snmp_set_var_typed_integer(vb, ASN_INTEGER, Other_vlan);
        break;
    case COLUMN_IEEE8021QBRIDGEVLANCREATIONTIME:
        snmp_set_var_typed_integer(vb, ASN_TIMETICKS, ent->creation_t);
        break;
    default:
        return SNMP_NOSUCHOBJECT;
    }
    return SNMP_ERR_NOERROR;
}

static int get(netsnmp_request_info *req, netsnmp_handler_registration *reginfo)
{
    int err, type;
    struct mib_vlan_table_entry ent;
    netsnmp_table_request_info *tinfo = netsnmp_extract_table_info(req);

    // Get indexes for entry
    err = get_indexes(req->requestvb, reginfo, tinfo, &ent);
    if (err)
        return err;

    DEBUGMSGTL((MIBMOD, "cid=%lu vid=%d time=%lu column=%d\n",
        ent.cid, ent.vid, ent.time_mark, tinfo->colnum));
    // Check index range
    if ((ent.cid != DEFAULT_COMPONENT_ID) ||
        (ent.vid >= NUM_VLANS))
        return SNMP_NOSUCHINSTANCE;

    // Read entry from RTU FDB.
    errno = 0;
    err = rtu_fdb_proxy_read_vlan_entry( ent.vid,
                                        &ent.fid,
                                        &type,
                                        &ent.port_mask,
                                        &ent.untagged_set,
                                        &ent.creation_t);
    if (errno)
        goto minipc_err;
    if (err)
        goto not_found;
    // Get column value
    return get_column(req->requestvb, tinfo->colnum, &ent);

not_found:
    DEBUGMSGTL((MIBMOD, "vlan vid=%d not found in fdb\n", ent.vid));
    return SNMP_NOSUCHINSTANCE;

minipc_err:
    snmp_log(LOG_ERR, "%s(%s): mini-ipc error [%s]\n", __FILE__, __func__,
        strerror(errno));
    return SNMP_ERR_GENERR;
}

static int get_next(netsnmp_request_info         *req,
                    netsnmp_handler_registration *reginfo)
{
    int err, type;
    struct mib_vlan_table_entry ent;
    netsnmp_variable_list *idx;
    netsnmp_table_request_info *tinfo = netsnmp_extract_table_info(req);

    // Get indexes from request
    err = get_indexes(req->requestvb, reginfo, tinfo, &ent);
    if (err)
        return err;

    DEBUGMSGTL((MIBMOD, "time=%d cid=%d vid=%d column=%d\n",
        ent.time_mark, ent.cid, ent.vid, tinfo->colnum));

    // TODO time filtering
    // Get indexes for next entry
    // SNMP_ENDOFMIBVIEW informs the handler to proceed with next column.
    if (ent.cid > DEFAULT_COMPONENT_ID)
        return SNMP_ENDOFMIBVIEW;

    if (ent.cid == 0) {
        ent.cid       = DEFAULT_COMPONENT_ID;
        ent.vid       = 0;
        ent.time_mark = 0;
        // NOTE: since vid=0 is a reserved value, it can be safely used to
        // start walking throught the VLAN table
#ifdef V2
        // Although use of VID=0 is reserved, it is actually used by HW V2,
        // so we need to check it also.
        errno = 0;
        err = rtu_fdb_proxy_read_vlan_entry(ent.vid,
                                            &ent.fid,
                                            &type,
                                            &ent.port_mask,
                                            &ent.untagged_set,
                                            &ent.creation_t);
        if (errno)
            goto minipc_err;
        if (!err)
            goto update_idx;
#endif // V2
    }
    if (ent.vid >= NUM_VLANS)
        return SNMP_ENDOFMIBVIEW;

    errno = 0;
    err = rtu_fdb_proxy_read_next_vlan_entry(&ent.vid,
                                             &ent.fid,
                                             &type,
                                             &ent.port_mask,
                                             &ent.untagged_set,
                                             &ent.creation_t);
    if (errno)
        goto minipc_err;
    if (err)
        return SNMP_ENDOFMIBVIEW;   // No other entry found

update_idx:
    // Update indexes and OID returned in SNMP response
    idx = tinfo->indexes;
    *idx->val.integer = ent.time_mark;

    idx = idx->next_variable;
    *idx->val.integer = ent.cid;

    idx = idx->next_variable;
    *idx->val.integer = ent.vid;

    update_oid(req, reginfo, tinfo->colnum, tinfo->indexes);
    // Get next entry column value
    return get_column(req->requestvb, tinfo->colnum, &ent);

minipc_err:
    snmp_log(LOG_ERR, "%s(%s): mini-ipc error [%s]\n", __FILE__, __func__,
        strerror(errno));
    return SNMP_ERR_GENERR;
}

/**
 * Handles requests for the ieee8021QBridgeVlanCurrentTable table
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
 * Initialize the ieee8021QBridgeVlanCurrentTable table by defining its
 * contents and how it's structured
 */
static void initialize_table(void)
{
    const oid _oid[] = {1,3,111,2,802,1,1,4,1,4,2};
    netsnmp_handler_registration    *reg;
    netsnmp_table_registration_info *tinfo;

    reg = netsnmp_create_handler_registration(
            "ieee8021QBridgeVlanCurrentTable",
            _handler,
            (oid *)_oid,
            OID_LENGTH(_oid),
            HANDLER_CAN_RONLY);

    tinfo = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    netsnmp_table_helper_add_indexes(
            tinfo,
            ASN_TIMETICKS,  /* index: TimeMark */
            ASN_UNSIGNED,   /* index: ComponentId */
            ASN_UNSIGNED,   /* index: VlanIndex */
            0);

    tinfo->min_column = COLUMN_IEEE8021QBRIDGEVLANFDBID;
    tinfo->max_column = COLUMN_IEEE8021QBRIDGEVLANCREATIONTIME;

    netsnmp_register_table(reg, tinfo);
}

/**
 * Initializes the ieee8021QBridgeVlanCurrentTable module
 */
void init_ieee8021QBridgeVlanCurrentTable(void)
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