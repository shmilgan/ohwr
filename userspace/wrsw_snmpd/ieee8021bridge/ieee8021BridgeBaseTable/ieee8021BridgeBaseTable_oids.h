/*
 * Note: this file originally auto-generated by mib2c using
 *  : generic-table-oids.m2c 17548 2009-04-23 16:35:18Z hardaker $
 *
 * $Id:$
 */
#ifndef IEEE8021BRIDGEBASETABLE_OIDS_H
#define IEEE8021BRIDGEBASETABLE_OIDS_H

#ifdef __cplusplus
extern "C" {
#endif


/* column number definitions for table ieee8021BridgeBaseTable */
#define IEEE8021BRIDGEBASETABLE_OID              1,3,111,2,802,1,1,2,1,1,1


#define COLUMN_IEEE8021BRIDGEBASECOMPONENTID         1
    
#define COLUMN_IEEE8021BRIDGEBASEBRIDGEADDRESS         2
#define COLUMN_IEEE8021BRIDGEBASEBRIDGEADDRESS_FLAG    (0x1 << 0)
    
#define COLUMN_IEEE8021BRIDGEBASENUMPORTS         3
#define COLUMN_IEEE8021BRIDGEBASENUMPORTS_FLAG    (0x1 << 1)
    
#define COLUMN_IEEE8021BRIDGEBASECOMPONENTTYPE         4
#define COLUMN_IEEE8021BRIDGEBASECOMPONENTTYPE_FLAG    (0x1 << 2)
    
#define COLUMN_IEEE8021BRIDGEBASEDEVICECAPABILITIES         5
#define COLUMN_IEEE8021BRIDGEBASEDEVICECAPABILITIES_FLAG    (0x1 << 3)
    
#define COLUMN_IEEE8021BRIDGEBASETRAFFICCLASSESENABLED         6
#define COLUMN_IEEE8021BRIDGEBASETRAFFICCLASSESENABLED_FLAG    (0x1 << 4)
    
#define COLUMN_IEEE8021BRIDGEBASEMMRPENABLEDSTATUS         7
#define COLUMN_IEEE8021BRIDGEBASEMMRPENABLEDSTATUS_FLAG    (0x1 << 5)
    
#define COLUMN_IEEE8021BRIDGEBASEROWSTATUS         8
#define COLUMN_IEEE8021BRIDGEBASEROWSTATUS_FLAG    (0x1 << 6)
    

#define IEEE8021BRIDGEBASETABLE_MIN_COL   COLUMN_IEEE8021BRIDGEBASEBRIDGEADDRESS
#define IEEE8021BRIDGEBASETABLE_MAX_COL   COLUMN_IEEE8021BRIDGEBASEROWSTATUS
    

    /*
     * TODO:405:r: Review IEEE8021BRIDGEBASETABLE_SETTABLE_COLS macro.
     * OR together all the writable cols.
     */
#define IEEE8021BRIDGEBASETABLE_SETTABLE_COLS (COLUMN_IEEE8021BRIDGEBASEBRIDGEADDRESS_FLAG | COLUMN_IEEE8021BRIDGEBASENUMPORTS_FLAG | COLUMN_IEEE8021BRIDGEBASECOMPONENTTYPE_FLAG | COLUMN_IEEE8021BRIDGEBASEDEVICECAPABILITIES_FLAG | COLUMN_IEEE8021BRIDGEBASETRAFFICCLASSESENABLED_FLAG | COLUMN_IEEE8021BRIDGEBASEMMRPENABLEDSTATUS_FLAG | COLUMN_IEEE8021BRIDGEBASEROWSTATUS_FLAG)
    /*
     * TODO:405:r: Review IEEE8021BRIDGEBASETABLE_REQUIRED_COLS macro.
     * OR together all the required rows for row creation.
     * default is writable cols w/out defaults.
     */
#define IEEE8021BRIDGEBASETABLE_REQUIRED_COLS (COLUMN_IEEE8021BRIDGEBASEBRIDGEADDRESS_FLAG | COLUMN_IEEE8021BRIDGEBASECOMPONENTTYPE_FLAG | COLUMN_IEEE8021BRIDGEBASEDEVICECAPABILITIES_FLAG | COLUMN_IEEE8021BRIDGEBASEROWSTATUS_FLAG)

/* Temporary mask to control what columns we want to implement */
#define COLUMNS_IMPLEMENTED \
                (COLUMN_IEEE8021BRIDGEBASEBRIDGEADDRESS_FLAG | \
                 COLUMN_IEEE8021BRIDGEBASENUMPORTS_FLAG | \
                 COLUMN_IEEE8021BRIDGEBASECOMPONENTTYPE_FLAG | \
                 COLUMN_IEEE8021BRIDGEBASEDEVICECAPABILITIES_FLAG | \
                 COLUMN_IEEE8021BRIDGEBASETRAFFICCLASSESENABLED_FLAG | \
                 COLUMN_IEEE8021BRIDGEBASEROWSTATUS_FLAG)

#ifdef __cplusplus
}
#endif

#endif /* IEEE8021BRIDGEBASETABLE_OIDS_H */
