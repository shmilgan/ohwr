/*
 * Note: this file originally auto-generated by mib2c using
 *       version : 12077 $ of $ 
 *
 * $Id:$
 */
#ifndef IEEE8021BRIDGETRAFFICCLASSTABLE_DATA_SET_H
#define IEEE8021BRIDGETRAFFICCLASSTABLE_DATA_SET_H

#ifdef __cplusplus
extern "C" {
#endif

/* *********************************************************************
 * SET function declarations
 */

/* *********************************************************************
 * SET Table declarations
 */
/**********************************************************************
 **********************************************************************
 ***
 *** Table ieee8021BridgeTrafficClassTable
 ***
 **********************************************************************
 **********************************************************************/
/*
 * IEEE8021-BRIDGE-MIB::ieee8021BridgeTrafficClassTable is subid 3 of ieee8021BridgePriority.
 * Its status is Current.
 * OID: .1.3.111.2.802.1.1.2.1.3.3, length: 11
*/


int ieee8021BridgeTrafficClassTable_undo_setup( ieee8021BridgeTrafficClassTable_rowreq_ctx *rowreq_ctx);
int ieee8021BridgeTrafficClassTable_undo_cleanup( ieee8021BridgeTrafficClassTable_rowreq_ctx *rowreq_ctx);
int ieee8021BridgeTrafficClassTable_undo( ieee8021BridgeTrafficClassTable_rowreq_ctx *rowreq_ctx);
int ieee8021BridgeTrafficClassTable_commit( ieee8021BridgeTrafficClassTable_rowreq_ctx *rowreq_ctx);
int ieee8021BridgeTrafficClassTable_undo_commit( ieee8021BridgeTrafficClassTable_rowreq_ctx *rowreq_ctx);


int ieee8021BridgeTrafficClass_check_value( ieee8021BridgeTrafficClassTable_rowreq_ctx *rowreq_ctx, long ieee8021BridgeTrafficClass_val);
int ieee8021BridgeTrafficClass_undo_setup( ieee8021BridgeTrafficClassTable_rowreq_ctx *rowreq_ctx );
int ieee8021BridgeTrafficClass_set( ieee8021BridgeTrafficClassTable_rowreq_ctx *rowreq_ctx, long ieee8021BridgeTrafficClass_val );
int ieee8021BridgeTrafficClass_undo( ieee8021BridgeTrafficClassTable_rowreq_ctx *rowreq_ctx );


int ieee8021BridgeTrafficClassTable_check_dependencies(ieee8021BridgeTrafficClassTable_rowreq_ctx *ctx);


#ifdef __cplusplus
}
#endif

#endif /* IEEE8021BRIDGETRAFFICCLASSTABLE_DATA_SET_H */
