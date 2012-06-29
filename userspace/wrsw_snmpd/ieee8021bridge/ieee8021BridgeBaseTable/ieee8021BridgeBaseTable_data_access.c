/*
 * Note: this file originally auto-generated by mib2c using
 *       version : 14170 $ of $
 *
 * $Id:$
 */
/*
 * White Rabbit SNMP
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_snmpd v1.0
 *
 * Authors:     Miguel Baizán (miguel.baizan@integrasys-sa.com)
 *
 * Description: The ieee8021BridgeBaseTable_data_access file contains the
 *              interface to the data in its raw format. These functions are
 *              used to build the row cache or locate the row (depending on the
 *              table access method).
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

/* standard Net-SNMP includes */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_ether.h>

#include <wr_ipc.h>
#include <wrsw_hal.h>
#include <hal_exports.h>

/* include our parent header */
#include "ieee8021BridgeBaseTable.h"


#include "ieee8021BridgeBaseTable_data_access.h"

/** @ingroup interface
 * @addtogroup data_access data_access: Routines to access data
 *
 * These routines are used to locate the data used to satisfy
 * requests.
 *
 * @{
 */
/**********************************************************************
 **********************************************************************
 ***
 *** Table ieee8021BridgeBaseTable
 ***
 **********************************************************************
 **********************************************************************/
/*
 * IEEE8021-BRIDGE-MIB::ieee8021BridgeBaseTable is subid 1 of ieee8021BridgeBase.
 * Its status is Current.
 * OID: .1.3.111.2.802.1.1.2.1.1.1, length: 11
*/

/**
 * initialization for ieee8021BridgeBaseTable data access
 *
 * This function is called during startup to allow you to
 * allocate any resources you need for the data table.
 *
 * @param ieee8021BridgeBaseTable_reg
 *        Pointer to ieee8021BridgeBaseTable_registration
 *
 * @retval MFD_SUCCESS : success.
 * @retval MFD_ERROR   : unrecoverable error.
 */
int
ieee8021BridgeBaseTable_init_data(ieee8021BridgeBaseTable_registration * ieee8021BridgeBaseTable_reg)
{
    DEBUGMSGTL(("verbose:ieee8021BridgeBaseTable:ieee8021BridgeBaseTable_init_data","called\n"));

    /*
     * TODO:303:o: Initialize ieee8021BridgeBaseTable data.
     */

    return MFD_SUCCESS;
} /* ieee8021BridgeBaseTable_init_data */

/**
 * container overview
 *
 */

/**
 * container initialization
 *
 * @param container_ptr_ptr A pointer to a container pointer. If you
 *        create a custom container, use this parameter to return it
 *        to the MFD helper. If set to NULL, the MFD helper will
 *        allocate a container for you.
 * @param  cache A pointer to a cache structure. You can set the timeout
 *         and other cache flags using this pointer.
 *
 *  This function is called at startup to allow you to customize certain
 *  aspects of the access method. For the most part, it is for advanced
 *  users. The default code should suffice for most cases. If no custom
 *  container is allocated, the MFD code will create one for your.
 *
 *  This is also the place to set up cache behavior. The default, to
 *  simply set the cache timeout, will work well with the default
 *  container. If you are using a custom container, you may want to
 *  look at the cache helper documentation to see if there are any
 *  flags you want to set.
 *
 * @remark
 *  This would also be a good place to do any initialization needed
 *  for you data source. For example, opening a connection to another
 *  process that will supply the data, opening a database, etc.
 */
void
ieee8021BridgeBaseTable_container_init(netsnmp_container **container_ptr_ptr,
                             netsnmp_cache *cache)
{
    DEBUGMSGTL(("verbose:ieee8021BridgeBaseTable:ieee8021BridgeBaseTable_container_init","called\n"));

    if (NULL == container_ptr_ptr) {
        snmp_log(LOG_ERR,"bad container param to ieee8021BridgeBaseTable_container_init\n");
        return;
    }

    /*
     * For advanced users, you can use a custom container. If you
     * do not create one, one will be created for you.
     */
    *container_ptr_ptr = NULL;

    if (NULL == cache) {
        snmp_log(LOG_ERR,"bad cache param to ieee8021BridgeBaseTable_container_init\n");
        return;
    }

    /*
     * TODO:345:A: Set up ieee8021BridgeBaseTable cache properties.
     *
     * Also for advanced users, you can set parameters for the
     * cache. Do not change the magic pointer, as it is used
     * by the MFD helper. To completely disable caching, set
     * cache->enabled to 0.
     */
    cache->timeout = IEEE8021BRIDGEBASETABLE_CACHE_TIMEOUT; /* seconds */
} /* ieee8021BridgeBaseTable_container_init */

/**
 * container shutdown
 *
 * @param container_ptr A pointer to the container.
 *
 *  This function is called at shutdown to allow you to customize certain
 *  aspects of the access method. For the most part, it is for advanced
 *  users. The default code should suffice for most cases.
 *
 *  This function is called before ieee8021BridgeBaseTable_container_free().
 *
 * @remark
 *  This would also be a good place to do any cleanup needed
 *  for you data source. For example, closing a connection to another
 *  process that supplied the data, closing a database, etc.
 */
void
ieee8021BridgeBaseTable_container_shutdown(netsnmp_container *container_ptr)
{
    DEBUGMSGTL(("verbose:ieee8021BridgeBaseTable:ieee8021BridgeBaseTable_container_shutdown","called\n"));

    if (NULL == container_ptr) {
        snmp_log(LOG_ERR,"bad params to ieee8021BridgeBaseTable_container_shutdown\n");
        return;
    }

} /* ieee8021BridgeBaseTable_container_shutdown */

/**
 * load initial data
 *
 * TODO:350:M: Implement ieee8021BridgeBaseTable data load
 * This function will also be called by the cache helper to load
 * the container again (after the container free function has been
 * called to free the previous contents).
 *
 * @param container container to which items should be inserted
 *
 * @retval MFD_SUCCESS              : success.
 * @retval MFD_RESOURCE_UNAVAILABLE : Can't access data source
 * @retval MFD_ERROR                : other error.
 *
 *  This function is called to load the index(es) (and data, optionally)
 *  for the every row in the data set.
 *
 * @remark
 *  While loading the data, the only important thing is the indexes.
 *  If access to your data is cheap/fast (e.g. you have a pointer to a
 *  structure in memory), it would make sense to update the data here.
 *  If, however, the accessing the data invovles more work (e.g. parsing
 *  some other existing data, or peforming calculations to derive the data),
 *  then you can limit yourself to setting the indexes and saving any
 *  information you will need later. Then use the saved information in
 *  ieee8021BridgeBaseTable_row_prep() for populating data.
 *
 * @note
 *  If you need consistency between rows (like you want statistics
 *  for each row to be from the same time frame), you should set all
 *  data here.
 *
 */
int
ieee8021BridgeBaseTable_container_load(netsnmp_container *container)
{
    ieee8021BridgeBaseTable_rowreq_ctx  *rowreq_ctx;

    wripc_handle_t          hal_ipc;
    hexp_port_list_t        port_list;

    struct ifreq            ifr;
    char                    if_name[MAX_IFNAME_SIZE];
    uint32_t                mac_high, mac_low;
    uint32_t                mac_high_prev = 0;
    uint32_t                mac_low_prev = 0;
    int                     sockfd, i;

    /* Index */
    u_long   ieee8021BridgeBaseComponentId;


    DEBUGMSGTL(("ieee8021BridgeBaseTable:ieee8021BridgeBaseTable_container_load"
                ,"called\n"));

    /* Set index value for the Component Id */
    ieee8021BridgeBaseComponentId = DEFAULT_COMPONENTID;

    /* Allocate rowreq context */
    rowreq_ctx = ieee8021BridgeBaseTable_allocate_rowreq_ctx(NULL);
    if (NULL == rowreq_ctx) {
        snmp_log(LOG_ERR, "memory allocation failed\n");
        return MFD_RESOURCE_UNAVAILABLE;
    }

    rowreq_ctx->column_exists_flags = COLUMNS_IMPLEMENTED;

    /* Set indexes in the row requets context */
    if (MFD_SUCCESS != ieee8021BridgeBaseTable_indexes_set(rowreq_ctx,
        ieee8021BridgeBaseComponentId)) {
        snmp_log(LOG_ERR,"error setting index while loading "
                 "ieee8021BridgeBaseTable data.\n");
        ieee8021BridgeBaseTable_release_rowreq_ctx(rowreq_ctx);
        return MFD_CANNOT_CREATE_NOW;
    }

    DEBUGMSGTL(("ieee8021BridgeBaseTable:ieee8021BridgeBaseTable_container_load"
                ,"indexes set\n"));

    /* Populate ieee8021BridgeBaseTable data context */

    /* Setup/save data for ieee8021BridgeBaseBridgeAddress */
    /* TODO: This object is permanent, so it should be read from a configuration
       file. For now, it will be the numerically smallest MAC address of the
       switch, as suggested by the standard */
    if ((NULL == rowreq_ctx->data.ieee8021BridgeBaseBridgeAddress) ||
        (sizeof(rowreq_ctx->data.ieee8021BridgeBaseBridgeAddress) < ETH_ALEN)) {
        snmp_log(LOG_ERR,"not enough space for value\n");
        return MFD_ERROR;
    }

    rowreq_ctx->data.ieee8021BridgeBaseBridgeAddress_len = ETH_ALEN;

    /* Create socket interface */
    sockfd = socket(AF_PACKET, SOCK_RAW, 0);
    if (sockfd < 0) {
        snmp_log(LOG_ERR,"socket failed\n");
        return MFD_RESOURCE_UNAVAILABLE;
    }

    /* Connect to HAL to get information of the ports */
    hal_ipc = wripc_connect("wrsw_hal");
    if (hal_ipc < 0) {
        snmp_log(LOG_ERR,"Unable to connect to HAL\n");
        close(sockfd);
        return MFD_ERROR;
    }

    DEBUGMSGTL(("ieee8021BridgeBaseTable:ieee8021BridgeBaseTable_container_load"
                ,"connected to HAL\n"));

    /* Get port list */
    if (wripc_call(hal_ipc, "halexp_query_all_ports", &port_list, 0) < 0) {
        snmp_log(LOG_ERR,"halexp_query_all_ports has not worked\n");
        wripc_close(hal_ipc);
        close(sockfd);
        return MFD_ERROR;
    }

    /* Iterate through port list */
    for (i = 0; i < HAL_MAX_PORTS; i++) {
        DEBUGMSGTL(("ieee8021BridgeBaseTable:"
                    "ieee8021BridgeBaseTable_container_load",
                    "port %d in port_list is: %s \n",
                    i, port_list.port_names[i]));
        /* Only interested in non null port names*/
        if (port_list.port_names[i][0] != '\0') {

            /* Find the numerically smallest MAC address */
            strncpy(ifr.ifr_name, port_list.port_names[i],
                    sizeof(ifr.ifr_name));
            if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
                continue;
            } else {
                mac_high =  ifr.ifr_hwaddr.sa_data[0] << 24 |
                            ifr.ifr_hwaddr.sa_data[1] << 16 |
                            ifr.ifr_hwaddr.sa_data[2] << 8  |
                            ifr.ifr_hwaddr.sa_data[3];
                mac_low =   ifr.ifr_hwaddr.sa_data[4] << 8  |
                            ifr.ifr_hwaddr.sa_data[5];

                if ((mac_high_prev == 0) && (mac_low_prev == 0)) {
                    mac_high_prev = mac_high;
                    mac_low_prev = mac_low;
                    strcpy(&if_name[0], port_list.port_names[i]);
                } else {
                    if (mac_high == mac_high_prev) {
                        if (mac_low <= mac_low_prev) {
                            mac_low_prev = mac_low;
                            strcpy(&if_name[0], port_list.port_names[i]);
                        }
                    } else {
                        if (mac_high <= mac_high_prev) {
                            mac_high_prev = mac_high;
                            strcpy(&if_name[0], port_list.port_names[i]);
                        }
                    }
                }
            } /* Find the numerically smallest MAC address */
        } /* Only interested in non null port names*/
    } /* Iterate through port list */

    DEBUGMSGTL(("ieee8021BridgeBaseTable:ieee8021BridgeBaseTable_container_load"
                ,"numerically smallest MAC address found in '%s'\n", if_name));

    strncpy(ifr.ifr_name, &if_name[0], sizeof(ifr.ifr_name));

    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
        memset(rowreq_ctx->data.ieee8021BridgeBaseBridgeAddress, (0), ETH_ALEN);
    } else {
        memcpy(rowreq_ctx->data.ieee8021BridgeBaseBridgeAddress,
               ifr.ifr_hwaddr.sa_data, ETH_ALEN);
    }

    /* Setup/save data for ieee8021BridgeBaseNumPorts */
    /* FIXME: Dummy ports shall not be included. However, the wrsw_hal does not
       make this differentiation with the new wr-nic driver (as it did with
       the minic driver) */
    if (wripc_call(hal_ipc, "halexp_query_ports", &port_list, 0) < 0) {
        snmp_log(LOG_ERR,"halexp_query_ports has not worked\n");
        wripc_close(hal_ipc);
        close(sockfd);
        return MFD_ERROR;
    }
    rowreq_ctx->data.ieee8021BridgeBaseNumPorts = port_list.num_ports;

    /* Setup/save data for ieee8021BridgeBaseComponentType */
    /* TODO: WR: The value now is hardcoded. However we should think of a
       source where it can be easily read from (config file?) */
    rowreq_ctx->data.ieee8021BridgeBaseComponentType =
    IEEE8021BRIDGEBASECOMPONENTTYPE_CVLANCOMPONENT;

    /* Setup/save data for ieee8021BridgeBaseDeviceCapabilities */
    /* TODO: WR: The values now are hardcoded. However we should think of a
       source where it can be easily read from (config file?) without needing
       to recompile */
    rowreq_ctx->data.ieee8021BridgeBaseDeviceCapabilities =
    IEEE8021BRIDGEBASEDEVICECAPABILITIES_FLAG;

    /* Setup/save data for ieee8021BridgeBaseTrafficClassesEnabled */
    /* TODO: WR: To be implemented when HW support for TCs be available */
    /* rowreq_ctx->data.ieee8021BridgeBaseTrafficClassesEnabled =
       ieee8021BridgeBaseTrafficClassesEnabled;*/

    /* Setup/save data for ieee8021BridgeBaseMmrpEnabledStatus */
    /* TODO: WR: To be implemented when MMRP be developed */
    /* rowreq_ctx->data.ieee8021BridgeBaseMmrpEnabledStatus =
       ieee8021BridgeBaseMmrpEnabledStatus;*/

    /* Setup/save data for ieee8021BridgeBaseRowStatus */
    rowreq_ctx->data.ieee8021BridgeBaseRowStatus = ROWSTATUS_ACTIVE;

    /* Insert into table container */
    CONTAINER_INSERT(container, rowreq_ctx);

    wripc_close(hal_ipc);
    close(sockfd);
    return MFD_SUCCESS;
} /* ieee8021BridgeBaseTable_container_load */


/**
 * container clean up
 *
 * @param container container with all current items
 *
 *  This optional callback is called prior to all
 *  item's being removed from the container. If you
 *  need to do any processing before that, do it here.
 *
 * @note
 *  The MFD helper will take care of releasing all the row contexts.
 *
 */
void
ieee8021BridgeBaseTable_container_free(netsnmp_container *container)
{
    DEBUGMSGTL(("verbose:ieee8021BridgeBaseTable:ieee8021BridgeBaseTable_container_free","called\n"));

    /*
     * TODO:380:M: Free ieee8021BridgeBaseTable container data.
     */
} /* ieee8021BridgeBaseTable_container_free */

/**
 * prepare row for processing.
 *
 *  When the agent has located the row for a request, this function is
 *  called to prepare the row for processing. If you fully populated
 *  the data context during the index setup phase, you may not need to
 *  do anything.
 *
 * @param rowreq_ctx pointer to a context.
 *
 * @retval MFD_SUCCESS     : success.
 * @retval MFD_ERROR       : other error.
 */
int
ieee8021BridgeBaseTable_row_prep( ieee8021BridgeBaseTable_rowreq_ctx *rowreq_ctx)
{
    DEBUGMSGTL(("verbose:ieee8021BridgeBaseTable:ieee8021BridgeBaseTable_row_prep","called\n"));

    netsnmp_assert(NULL != rowreq_ctx);

    /*
     * TODO:390:o: Prepare row for request.
     * If populating row data was delayed, this is the place to
     * fill in the row for this request.
     */

    return MFD_SUCCESS;
} /* ieee8021BridgeBaseTable_row_prep */

/*
 * TODO:420:r: Implement ieee8021BridgeBaseTable index validation.
 */
/*---------------------------------------------------------------------
 * IEEE8021-BRIDGE-MIB::ieee8021BridgeBaseEntry.ieee8021BridgeBaseComponentId
 * ieee8021BridgeBaseComponentId is subid 1 of ieee8021BridgeBaseEntry.
 * Its status is Current, and its access level is NoAccess.
 * OID: .1.3.111.2.802.1.1.2.1.1.1.1.1
 * Description:
The component identifier is used to distinguish between the
        multiple virtual bridge instances within a PBB.  In simple
        situations where there is only a single component the default
        value is 1.
 *
 * Attributes:
 *   accessible 0     isscalar 0     enums  0      hasdefval 0
 *   readable   0     iscolumn 1     ranges 1      hashint   1
 *   settable   0
 *   hint: d
 *
 * Ranges:  1 - 2147483647;
 *
 * Its syntax is IEEE8021PbbComponentIdentifier (based on perltype UNSIGNED32)
 * The net-snmp type is ASN_UNSIGNED. The C type decl is u_long (u_long)
 *
 *
 *
 * NOTE: NODE ieee8021BridgeBaseComponentId IS NOT ACCESSIBLE
 *
 *
 */
/**
 * check validity of ieee8021BridgeBaseComponentId index portion
 *
 * @retval MFD_SUCCESS   : the incoming value is legal
 * @retval MFD_ERROR     : the incoming value is NOT legal
 *
 * @note this is not the place to do any checks for the sanity
 *       of multiple indexes. Those types of checks should be done in the
 *       ieee8021BridgeBaseTable_validate_index() function.
 *
 * @note Also keep in mind that if the index refers to a row in this or
 *       some other table, you can't check for that row here to make
 *       decisions, since that row might not be created yet, but may
 *       be created during the processing this request. If you have
 *       such checks, they should be done in the check_dependencies
 *       function, because any new/deleted/changed rows should be
 *       available then.
 *
 * The following checks have already been done for you:
 *    The value is in (one of) the range set(s):  1 - 2147483647
 *
 * If there a no other checks you need to do, simply return MFD_SUCCESS.
 */
int
ieee8021BridgeBaseComponentId_check_index( ieee8021BridgeBaseTable_rowreq_ctx *rowreq_ctx )
{
    DEBUGMSGTL(("verbose:ieee8021BridgeBaseTable:ieee8021BridgeBaseComponentId_check_index","called\n"));

    netsnmp_assert(NULL != rowreq_ctx);

    /*
     * TODO:426:M: |-> Check ieee8021BridgeBaseTable index ieee8021BridgeBaseComponentId.
     * check that index value in the table context is legal.
     * (rowreq_ctx->tbl_index.ieee8021BridgeBaseComponentId)
     */

    return MFD_SUCCESS; /* ieee8021BridgeBaseComponentId index ok */
} /* ieee8021BridgeBaseComponentId_check_index */

/**
 * verify specified index is valid.
 *
 * This check is independent of whether or not the values specified for
 * the columns of the new row are valid. Column values and row consistency
 * will be checked later. At this point, only the index values should be
 * checked.
 *
 * All of the individual index validation functions have been called, so this
 * is the place to make sure they are valid as a whole when combined. If
 * you only have one index, then you probably don't need to do anything else
 * here.
 *
 * @note Keep in mind that if the indexes refer to a row in this or
 *       some other table, you can't check for that row here to make
 *       decisions, since that row might not be created yet, but may
 *       be created during the processing this request. If you have
 *       such checks, they should be done in the check_dependencies
 *       function, because any new/deleted/changed rows should be
 *       available then.
 *
 *
 * @param ieee8021BridgeBaseTable_reg
 *        Pointer to the user registration data
 * @param ieee8021BridgeBaseTable_rowreq_ctx
 *        Pointer to the users context.
 * @retval MFD_SUCCESS            : success
 * @retval MFD_CANNOT_CREATE_NOW  : index not valid right now
 * @retval MFD_CANNOT_CREATE_EVER : index never valid
 */
int
ieee8021BridgeBaseTable_validate_index( ieee8021BridgeBaseTable_registration * ieee8021BridgeBaseTable_reg,
                           ieee8021BridgeBaseTable_rowreq_ctx *rowreq_ctx)
{
    int rc = MFD_SUCCESS;

    DEBUGMSGTL(("verbose:ieee8021BridgeBaseTable:ieee8021BridgeBaseTable_validate_index","called\n"));

    /** we should have a non-NULL pointer */
    netsnmp_assert( NULL != rowreq_ctx );

    /*
     * TODO:430:M: |-> Validate potential ieee8021BridgeBaseTable index.
     */
    if(1) {
        snmp_log(LOG_WARNING,"invalid index for a new row in the "
                 "ieee8021BridgeBaseTable table.\n");
        /*
         * determine failure type.
         *
         * If the index could not ever be created, return MFD_NOT_EVER
         * If the index can not be created under the present circumstances
         * (even though it could be created under other circumstances),
         * return MFD_NOT_NOW.
         */
        if(0) {
            return MFD_CANNOT_CREATE_EVER;
        }
        else {
            return MFD_CANNOT_CREATE_NOW;
        }
    }

    return rc;
} /* ieee8021BridgeBaseTable_validate_index */

/** @} */
