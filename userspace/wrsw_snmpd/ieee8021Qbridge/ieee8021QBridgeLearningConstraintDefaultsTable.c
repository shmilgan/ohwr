/*
 * White Rabbit Switch Management
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_snmpd v1.0
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: Handles requests for ieee8021QBridgeLearningConstraintDefaults table.
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

#include "ieee8021QBridgeLearningConstraintDefaultsTable.h"
#include "rtu_fd_proxy.h"
#include "utils.h"

#define MIBMOD "8021Q"

/* column number definitions for table ieee8021QBridgeLearningConstraintDefaultsTable */
#define COLUMN_COMPONENTID	1
#define COLUMN_SET		    2
#define COLUMN_TYPE		    3

static int get_column(netsnmp_variable_list *vb, int colnum)
{
    int sid, lc_type;

    errno = 0;
    rtu_fdb_proxy_get_default_lc(&sid, &lc_type);
    if (errno)
        goto minipc_err;
    switch (colnum) {
    case COLUMN_SET:
        snmp_set_var_typed_integer(vb, ASN_INTEGER, sid);
        break;
    case COLUMN_TYPE:
        snmp_set_var_typed_integer(vb, ASN_INTEGER, lc_type);
        break;
    default:
        return SNMP_NOSUCHOBJECT;
    }
    return SNMP_ERR_NOERROR;

minipc_err:
    snmp_log(LOG_ERR, "%s(%s): mini-ipc error [%s]\n", __FILE__, __func__,
        strerror(errno));
    return SNMP_ERR_GENERR;
}

static int get(netsnmp_request_info *req)
{
    u_long cid;
    netsnmp_table_request_info *tinfo = netsnmp_extract_table_info(req);

    if (!tinfo || !tinfo->indexes)
        return SNMP_ERR_GENERR;

    // Get indexes from request
    cid  = *tinfo->indexes->val.integer;

    DEBUGMSGTL((MIBMOD, "cid=%d column=%d\n", cid, tinfo->colnum));

    if (cid != DEFAULT_COMPONENT_ID)
        return SNMP_NOSUCHINSTANCE;

    return get_column(req->requestvb, tinfo->colnum);
}


static int get_next(netsnmp_request_info         *req,
                    netsnmp_handler_registration *reginfo)
{
    int err;
    u_long cid;
    int oid_len, rootoid_len;
    netsnmp_table_request_info  *tinfo = netsnmp_extract_table_info(req);


    // Get indexes from request - in case OID contains them!.
    // Otherwise use default values for first row.
    oid_len     = req->requestvb->name_length;
    rootoid_len = reginfo->rootoid_len;

    if (oid_len > rootoid_len) {
        if (!tinfo || !tinfo->indexes)
            return SNMP_ERR_GENERR;
        cid = *tinfo->indexes->val.integer;
    } else {
        cid = 0;
    }

    DEBUGMSGTL((MIBMOD, "cid=%d column=%d\n", cid, tinfo->colnum));

    // Get index for next entry - SNMP_ENDOFMIBVIEW informs the handler
    // to proceed with next column.
    if (cid >= DEFAULT_COMPONENT_ID)
        return SNMP_ENDOFMIBVIEW;
    if (cid == 0)
        cid = DEFAULT_COMPONENT_ID;

    // Update indexes and OID returned in SNMP response
    *tinfo->indexes->val.integer = cid;
    update_oid(req, reginfo, tinfo->colnum, tinfo->indexes);

    // return next entry column value
    return get_column(req->requestvb, tinfo->colnum);
}

static int set_reserve1(netsnmp_request_info *req)
{
    int ret = SNMP_ERR_NOERROR;
    u_long cid;                 // ieee8021BridgeBasePortComponentId;
    netsnmp_table_request_info *tinfo = netsnmp_extract_table_info(req);

    if (!tinfo || !tinfo->indexes)
        return SNMP_ERR_GENERR;

    // Check indexes
    cid = *tinfo->indexes->val.integer;
    if (cid != DEFAULT_COMPONENT_ID)
        return SNMP_NOSUCHINSTANCE;

    // Check column value
    switch (tinfo->colnum) {
    case COLUMN_SET:
        ret = netsnmp_check_vb_int_range(req->requestvb, 0, NUM_LC_SETS - 1);
        break;
    case COLUMN_TYPE:
        ret = netsnmp_check_vb_int_range(req->requestvb, LC_INDEPENDENT, LC_SHARED);
        break;
    default:
        return SNMP_ERR_NOTWRITABLE;
    }
    return ret;
}

static int set_commit(netsnmp_request_info *req)
{
    int sid, lc_type;
    netsnmp_table_request_info *tinfo;
    int err = SNMP_ERR_NOERROR;

    tinfo = netsnmp_extract_table_info(req);
    errno = 0;
    switch (tinfo->colnum) {
    case COLUMN_SET:
        sid = *req->requestvb->val.integer;
        err = rtu_fdb_proxy_set_default_lc(sid);
        if (errno)
            goto minipc_err;
        if (err)
            return SNMP_NOSUCHINSTANCE;
        break;
    case COLUMN_TYPE:
        lc_type = *req->requestvb->val.integer;
        err = rtu_fdb_proxy_set_default_lc_type(lc_type);
        if (errno)
            goto minipc_err;
        if (err)
            return SNMP_NOSUCHINSTANCE;
        break;
    }

    return err;

minipc_err:
    snmp_log(LOG_ERR, "%s(%s): mini-ipc error [%s]\n", __FILE__, __func__,
        strerror(errno));
    return SNMP_ERR_GENERR;
}

/**
 * Handles requests for the ieee8021QBridgePortVlanTable table
 */
static int _handler(netsnmp_mib_handler          *handler,
                    netsnmp_handler_registration *reginfo,
                    netsnmp_agent_request_info   *reqinfo,
                    netsnmp_request_info         *requests)
{

    netsnmp_request_info *req;
    int err;

    switch (reqinfo->mode) {
    case MODE_GET:
        for (req = requests; req; req = req->next) {
            err = get(req);
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
    case MODE_SET_RESERVE1:
        for (req=requests; req; req=req->next) {
            err = set_reserve1(req);
            if (err) {
                netsnmp_set_request_error(reqinfo, req, err);
                return SNMP_ERR_NOERROR;
            }
        }
        break;
    case MODE_SET_COMMIT:
        for (req = requests; req; req = req->next) {
            err = set_commit(req);
            if (err)
                netsnmp_set_request_error(reqinfo, req, err);
        }
        break;
    }
    return SNMP_ERR_NOERROR;
}


/**
 * Initialize the ieee8021QBridgePortVlanTable table by defining its
 * contents and how it's structured
 */
static void initialize_table(void)
{
    const oid _oid[] = {1,3,111,2,802,1,1,4,1,4,9};
    netsnmp_handler_registration    *reg;
    netsnmp_table_registration_info *tinfo;

    reg = netsnmp_create_handler_registration(
            "ieee8021QBridgeLearningConstraintDefaultsTable",
            _handler,
            (oid *)_oid,
            OID_LENGTH(_oid),
            HANDLER_CAN_RWRITE);

    tinfo = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
    netsnmp_table_helper_add_indexes(
            tinfo,
            ASN_UNSIGNED,  /* index: ieee8021BridgeBasePortComponentId */
            0);

    tinfo->min_column = COLUMN_SET;
    tinfo->max_column = COLUMN_TYPE;

    netsnmp_register_table(reg, tinfo);
}

/**
 * Initializes the ieee8021QBridgeLearningConstraintsTable module
 */
void init_ieee8021QBridgeLearningConstraintDefaultsTable(void)
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
