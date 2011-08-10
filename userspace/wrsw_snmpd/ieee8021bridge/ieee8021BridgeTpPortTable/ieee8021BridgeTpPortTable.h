/*
 * Note: this file originally auto-generated by mib2c using
 *       version : 17337 $ of $
 *
 * $Id:$
 */
#ifndef IEEE8021BRIDGETPPORTTABLE_H
#define IEEE8021BRIDGETPPORTTABLE_H

#ifdef __cplusplus
extern "C" {
#endif


/** @addtogroup misc misc: Miscellaneous routines
 *
 * @{
 */
#include <net-snmp/library/asn1.h>

/* other required module components */
    /* *INDENT-OFF*  */
config_add_mib(IEEE8021-BRIDGE-MIB)
config_require(IEEE8021-BRIDGE-MIB/ieee8021BridgeTpPortTable/ieee8021BridgeTpPortTable_interface)
config_require(IEEE8021-BRIDGE-MIB/ieee8021BridgeTpPortTable/ieee8021BridgeTpPortTable_data_access)
config_require(IEEE8021-BRIDGE-MIB/ieee8021BridgeTpPortTable/ieee8021BridgeTpPortTable_data_get)
config_require(IEEE8021-BRIDGE-MIB/ieee8021BridgeTpPortTable/ieee8021BridgeTpPortTable_data_set)
    /* *INDENT-ON*  */

/* OID and column number definitions for ieee8021BridgeTpPortTable */
#include "ieee8021BridgeTpPortTable_oids.h"

/* enum definions */
#include "ieee8021BridgeTpPortTable_enums.h"

/* *********************************************************************
 * function declarations
 */
void init_ieee8021BridgeTpPortTable(void);
void shutdown_ieee8021BridgeTpPortTable(void);

/* *********************************************************************
 * Table declarations
 */
/**********************************************************************
 **********************************************************************
 ***
 *** Table ieee8021BridgeTpPortTable
 ***
 **********************************************************************
 **********************************************************************/
/*
 * IEEE8021-BRIDGE-MIB::ieee8021BridgeTpPortTable is subid 1 of ieee8021BridgeTp.
 * Its status is Current.
 * OID: .1.3.111.2.802.1.1.2.1.2.1, length: 11
*/
/* *********************************************************************
 * When you register your mib, you get to provide a generic
 * pointer that will be passed back to you for most of the
 * functions calls.
 *
 * TODO:100:r: Review all context structures
 */
    /*
     * TODO:101:o: |-> Review ieee8021BridgeTpPortTable registration context.
     */
typedef netsnmp_data_list ieee8021BridgeTpPortTable_registration;

/**********************************************************************/
/*
 * TODO:110:r: |-> Review ieee8021BridgeTpPortTable data context structure.
 * This structure is used to represent the data for ieee8021BridgeTpPortTable.
 */
/*
 * This structure contains storage for all the columns defined in the
 * ieee8021BridgeTpPortTable.
 */
typedef struct ieee8021BridgeTpPortTable_data_s {
    
        /*
         * ieee8021BridgeTpPortMaxInfo(3)/INTEGER32/ASN_INTEGER/long(long)//l/A/w/e/r/d/h
         */
   long   ieee8021BridgeTpPortMaxInfo;
    
        /*
         * ieee8021BridgeTpPortInFrames(4)/COUNTER64/ASN_COUNTER64/U64(U64)//l/A/w/e/r/d/h
         */
   U64   ieee8021BridgeTpPortInFrames;
    
        /*
         * ieee8021BridgeTpPortOutFrames(5)/COUNTER64/ASN_COUNTER64/U64(U64)//l/A/w/e/r/d/h
         */
   U64   ieee8021BridgeTpPortOutFrames;
    
        /*
         * ieee8021BridgeTpPortInDiscards(6)/COUNTER64/ASN_COUNTER64/U64(U64)//l/A/w/e/r/d/h
         */
   U64   ieee8021BridgeTpPortInDiscards;
    
} ieee8021BridgeTpPortTable_data;


/*
 * TODO:120:r: |-> Review ieee8021BridgeTpPortTable mib index.
 * This structure is used to represent the index for ieee8021BridgeTpPortTable.
 */
typedef struct ieee8021BridgeTpPortTable_mib_index_s {

        /*
         * ieee8021BridgeTpPortComponentId(1)/IEEE8021PbbComponentIdentifier/ASN_UNSIGNED/u_long(u_long)//l/a/w/e/R/d/H
         */
   u_long   ieee8021BridgeTpPortComponentId;

        /*
         * ieee8021BridgeTpPort(2)/IEEE8021BridgePortNumber/ASN_UNSIGNED/u_long(u_long)//l/a/w/e/R/d/H
         */
   u_long   ieee8021BridgeTpPort;


} ieee8021BridgeTpPortTable_mib_index;

    /*
     * TODO:121:r: |   |-> Review ieee8021BridgeTpPortTable max index length.
     * If you KNOW that your indexes will never exceed a certain
     * length, update this macro to that length.
*/
#define MAX_ieee8021BridgeTpPortTable_IDX_LEN     2


/* *********************************************************************
 * TODO:130:o: |-> Review ieee8021BridgeTpPortTable Row request (rowreq) context.
 * When your functions are called, you will be passed a
 * ieee8021BridgeTpPortTable_rowreq_ctx pointer.
 */
typedef struct ieee8021BridgeTpPortTable_rowreq_ctx_s {

    /** this must be first for container compare to work */
    netsnmp_index        oid_idx;
    oid                  oid_tmp[MAX_ieee8021BridgeTpPortTable_IDX_LEN];
    
    ieee8021BridgeTpPortTable_mib_index        tbl_idx;
    
    ieee8021BridgeTpPortTable_data              data;
    unsigned int                column_exists_flags; /* flags for existence */

    /*
     * flags per row. Currently, the first (lower) 8 bits are reserved
     * for the user. See mfd.h for other flags.
     */
    u_int                       rowreq_flags;

    /*
     * TODO:131:o: |   |-> Add useful data to ieee8021BridgeTpPortTable rowreq context.
     */
    
    /*
     * storage for future expansion
     */
    netsnmp_data_list             *ieee8021BridgeTpPortTable_data_list;

} ieee8021BridgeTpPortTable_rowreq_ctx;

typedef struct ieee8021BridgeTpPortTable_ref_rowreq_ctx_s {
    ieee8021BridgeTpPortTable_rowreq_ctx *rowreq_ctx;
} ieee8021BridgeTpPortTable_ref_rowreq_ctx;

/* *********************************************************************
 * function prototypes
 */
    int ieee8021BridgeTpPortTable_pre_request(ieee8021BridgeTpPortTable_registration * user_context);
    int ieee8021BridgeTpPortTable_post_request(ieee8021BridgeTpPortTable_registration * user_context,
        int rc);

    int ieee8021BridgeTpPortTable_rowreq_ctx_init(ieee8021BridgeTpPortTable_rowreq_ctx *rowreq_ctx,
                                   void *user_init_ctx);
    void ieee8021BridgeTpPortTable_rowreq_ctx_cleanup(ieee8021BridgeTpPortTable_rowreq_ctx *rowreq_ctx);


    ieee8021BridgeTpPortTable_rowreq_ctx *
                  ieee8021BridgeTpPortTable_row_find_by_mib_index(ieee8021BridgeTpPortTable_mib_index *mib_idx);

extern const oid ieee8021BridgeTpPortTable_oid[];
extern const int ieee8021BridgeTpPortTable_oid_size;


#include "ieee8021BridgeTpPortTable_interface.h"
#include "ieee8021BridgeTpPortTable_data_access.h"
#include "ieee8021BridgeTpPortTable_data_get.h"
#include "ieee8021BridgeTpPortTable_data_set.h"

/*
 * DUMMY markers, ignore
 *
 * TODO:099:x: *************************************************************
 * TODO:199:x: *************************************************************
 * TODO:299:x: *************************************************************
 * TODO:399:x: *************************************************************
 * TODO:499:x: *************************************************************
 */

#ifdef __cplusplus
}
#endif

#endif /* IEEE8021BRIDGETPPORTTABLE_H */
/** @} */
