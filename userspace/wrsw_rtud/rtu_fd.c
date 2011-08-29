/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_rtud v2.0
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *              Miguel Baizan   (miguel.baizan@integrasys.es)
 *              Maciej Lipinski (maciej.lipinski@cern.ch)
 *
 * Description: RTU Filtering database.
 *              Filtering database management related operations.
 *
 * Fixes:
 *              Alessandro Rubini
 *              Tomasz Wlostowski
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


#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <hw/trace.h>

#include <hal_client.h>

#include "rtu_fd.h"
#include "rtu_sw.h"
#include "rtu_drv.h"
#include "rtu_hash.h"

#include "utils.h"


/**
 * Max time that a dynamic MAC entry can remain
 * in the MAC table after being used. [seconds]
 */
static unsigned long aging_time;

/**
 * Keeps track of number of dynamic entries per filtering database.
 */
static uint16_t num_dynamic_entries[NUM_FIDS];

/**
 * Keeps track of learned entry discards (when FDB is full).
 */
static uint64_t num_learned_entry_discards[NUM_FIDS];

/**
 * Keeps track of vlan deletes, regardless of the reason.
 */
static uint64_t num_vlan_deletes;

/**
 * Mutex used to synchronise concurrent access to the filtering database.
 */
static pthread_mutex_t fd_mutex;

/**
 * Filtering Database (SW oriented model for lexicographic ordered list).
 */
static struct filtering_entry_node *fd[NUM_FIDS];

/**
 * Static Filtering Database.(SW oriented model for lexicographic ordered list).
 * Unicast and multicast entries stored in separated linked lists
 * (identified by first array dimension: 0 = Unicast, 1 = Multicast.)
 */
static struct static_filtering_entry *sfd[2][NUM_VLANS];

static inline
int reserved(int vid)
{
    return (vid == 0) || (vid == WILDCARD_VID);
}

static inline
void lock()
{
    pthread_mutex_lock(&fd_mutex);
}

static inline
int unlock(int code)
{
    pthread_mutex_unlock(&fd_mutex);
    return code;
}

//------------------------------------------------------------------------------
// Auxiliary functions to convert 802.1Q data types into low level data types.
//------------------------------------------------------------------------------

/**
 * Obtains de port_mask and use_dynamic masks from the VLAN member_set.
 */
static void calculate_vlan_vector(
        	enum registrar_control member_set[NUM_PORTS],
            uint32_t *port_mask,
            uint32_t *use_dynamic)
{
    int i;

    // If entry specifies Registration_fixed for this port, then Forward
    // else if entry specifies Registration_forbidden for this port, then Filter
    // else use Normal_registration (dynamic info from MVRP)
    *use_dynamic = 0xFFFFFFFF;
    *port_mask   = 0x00000000;
    for (i = 0; i < NUM_PORTS; i++) {
        if (member_set[i] == Registration_fixed) {
            *port_mask   |=  (0x01 << i);
            *use_dynamic &= !(0x01 << i);
        } else if (member_set[i] == Registration_forbidden) {
            *use_dynamic &= !(0x01 << i);
        }
    }
}

/**
 * Fills up the member_set combining info from port_map and use_dynamic masks.
 */
static void calculate_member_set(
            enum registrar_control (*member_set)[NUM_PORTS],
            uint32_t port_map,
            uint32_t use_dynamic)
{
    int i;

    for (i = 0; i < NUM_PORTS; i++) {
        if ((use_dynamic >> i) & 0x01)
            *member_set[i] = Normal_registration;
        else if ((port_map >> i) & 0x01)
            *member_set[i] = Registration_fixed;
        else
            *member_set[i] = Registration_forbidden;
    }
}

/**
 * Combines port_map information from a list of static entries.
 * @param sfd pointer to the list of static entries that must be combined.
 */
static void calculate_forward_vector(
            struct static_filtering_entry *sfd,
            uint32_t *port_map,
            uint32_t *use_dynamic)
{
    int i, n;

    // If any entry specifies Forward for this port, then result is Forward
    // else if any entry specifies Filter for this port, then result is Filter
    // else use Dynamic info
    *use_dynamic = 0xFFFFFFFF;
    *port_map    = 0x00000000;
    for (n = 0; sfd; sfd = sfd->next_sib, n++) {
        // Only active entries are used to compute the forward vector
        if (!sfd->active)
            continue;
        // The static filtering information specified for the wildcard VID only
        // applies to VLANs for which no specific static filtering entry exists.
        if (sfd->vid == WILDCARD_VID)
            if (n > 0)
                break;
        for (i = 0; i < NUM_PORTS; i++) {
            if (sfd->port_map[i] == Forward) {
                *port_map    |=  (0x01 << i);
                *use_dynamic &= !(0x01 << i);
            } else if (sfd->port_map[i] == Filter) {
                *use_dynamic &= !(0x01 << i);
            }
        }
    }
}

//------------------------------------------------------------------------------
// Static FDB - Double Linked List (lexicografically ordered)
//------------------------------------------------------------------------------

/**
 * Creates a filtering entry in the static FDB linked list.
 * @return pointer to entry created. NULL if no memory available
 */
static struct static_filtering_entry *sfe_create(
        uint8_t  mac[ETH_ALEN],
        uint16_t vid,
        enum filtering_control port_map[NUM_PORTS],
        enum storage_type type,
        int active)
{
    int xcast;
    struct static_filtering_entry *node, *sfe;

    sfe = (struct static_filtering_entry*)
          malloc(sizeof(struct static_filtering_entry));

    if (!sfe)
        return NULL;

    mac_copy(sfe->mac, mac);
    memcpy(sfe->port_map, port_map, NUM_PORTS);
    sfe->vid      = vid;
    sfe->type     = type;
    sfe->active   = active;
    sfe->next_sib = NULL;
    // Link into lexicographically ordered list
    xcast = mac_multicast(sfe->mac);
    node  = sfd[xcast][vid];
    if (!node) {
        // first node for this VID
        sfd[xcast][vid] = sfe;
        sfe->prev       = NULL;
        sfe->next       = NULL;
    } else if (mac_cmp(sfe->mac, node->mac) < 0) {
        // entry should be first
        node->prev      = sfe;
        sfe->prev       = NULL;
        sfe->next       = node;
        sfd[xcast][vid] = sfe;
    } else {
        // find place to insert node
        for (;node->next && (mac_cmp(sfe->mac, node->next->mac) > 0);
              node = node->next);
        sfe->next        = node->next;
        sfe->prev        = node;
        node->next->prev = sfe;
        node->next       = sfe;
    }
    return sfe;
}

/**
 * Deletes an entry from the static FDB.
 */
static void sfe_delete(struct static_filtering_entry *sfe)
{
    if(sfe->prev)
        sfe->prev->next = sfe->next;
    else                            // first node in fdb double linked list
        sfd[mac_multicast(sfe->mac)][sfe->vid] = sfe->next;

    if(sfe->next)
        sfe->next->prev = sfe->prev;
    free(sfe);
}

/**
 * Searches for an entry with given mac and vid in static fdb.
 * @return pointer to entry. NULL if none found.
 */
static struct static_filtering_entry *sfe_find(
        uint8_t mac[ETH_ALEN],
        uint8_t vid)
{
    struct static_filtering_entry *sfe;

    for (sfe = sfd[mac_multicast(mac)][vid]; sfe && !mac_equal(mac, sfe->mac);
         sfe = sfe->next);
    return sfe;
}


/**
 * Finds the filtering entry following the given one in lexicographic order.
 * @return pointer to next entry in lexicographic order. NULL if none found.
 */
struct static_filtering_entry *sfe_find_next(
        struct static_filtering_entry *sfe)
{
    int vid, xcast;

    if (sfe->next)
        return sfe->next;

    // check entries for following VIDs
    xcast = mac_multicast(sfe->mac);
    for (vid = sfe->vid + 1; vid < NUM_VLANS; vid++)
        if (sfd[xcast][vid])
            return sfd[xcast][vid];
    return NULL;
}

/**
 * Updates static FDB entry.
 * @return 0 if entry was updated. 1 otherwise.
 */
static int sfe_update(
        struct static_filtering_entry *sfe,
        enum filtering_control port_map[NUM_PORTS],
        int active)
{
    int ret = 1;

    if (sfe->active != active) {
        sfe->active = active;
        ret = 0;
    }
    if (memcmp(sfe->port_map, port_map, NUM_PORTS)) {
        memcpy(sfe->port_map, port_map, NUM_PORTS);
        ret = 0;
    }
    return ret;
}

//------------------------------------------------------------------------------
// FDB - Double Linked List (lexicografically ordered)
//------------------------------------------------------------------------------

/**
 * Creates a filtering entry in the fdb linked list.
 * @return pointer to entry created. NULL if no memory available
 */
static struct filtering_entry_node *fd_create(
        uint8_t  mac[ETH_ALEN],
        uint8_t fid)
{
    struct filtering_entry_node *node, *fe;

    fe = (struct filtering_entry_node*)
            malloc(sizeof(struct filtering_entry_node));

    if (!fe)
        return NULL;

    mac_copy(fe->mac, mac);
    fe->fid = fid;
    // Link into lexicographically ordered list
    node = fd[fid];
    if (!node) {
        // first node for this FID
        fd[fid]  = fe;
        fe->prev = NULL;
        fe->next = NULL;
    } else if (mac_cmp(fe->mac, node->mac) < 0) {
        // entry should be first
        node->prev = fe;
        fe->prev   = NULL;
        fe->next   = node;
        fd[fid]    = fe;
    } else {
        // find place to insert node
        for (;node->next && (mac_cmp(fe->mac, node->next->mac) > 0);
              node = node->next);
        fe->next         = node->next;
        fe->prev         = node;
        node->next->prev = fe;
        node->next       = fe;
    }
    return fe;
}

/**
 * Deletes an entry from the fdb double linked list.
 */
static void fd_delete(struct filtering_entry_node *fe)
{
    if (!fe)
        return;

    if(fe->prev)
        fe->prev->next = fe->next;
    else                            // first node in fdb double linked list
        fd[fe->fid] = fe->next;

    if(fe->next)
        fe->next->prev = fe->prev;
    free(fe);
}

/**
 * Finds the filtering entry with given mac and fid in lex list.
 * @return pointer to entry in lexicographic ordered list. NULL if none found.
 */
struct filtering_entry_node *fd_find(uint8_t mac[ETH_ALEN], uint8_t fid)
{
    struct filtering_entry_node *node;

    for (node = fd[fid]; node && !mac_equal(mac, node->mac); node = node->next);
    return node;
}

/**
 * Finds the filtering entry following the given one in lexicographic order.
 * @return pointer to next entry in lexicographic order. NULL if none found.
 */
struct filtering_entry_node *fd_find_next(struct filtering_entry_node *node)
{
    int fid;

    if (node->next)
        return node->next;

    // check entries for following FIDs
    for (fid = node->fid + 1; fid < NUM_FIDS; fid++)
        if (fd[fid])
            return fd[fid];
    return NULL;
}

/**
 * Creates a filtering entry in fdb (both mirror and lex list).
 * @return 0 if entry was created. -ENOMEM if fdb is full.
 */
static int rtu_fdb_create_entry(
            struct filtering_entry **fe,
            uint8_t  mac[ETH_ALEN],
            uint16_t vid,
            uint8_t fid,
            uint32_t port_map,
            uint32_t use_dynamic,
            int dynamic)
{
    struct filtering_entry_node *node;

    // Create entry in lex list
    node = fd_create(mac, fid);
    if (!node)
        return -ENOMEM;
    // Create entry in RTU SW mirror
    if (rtu_sw_create_entry(fe, mac, vid, port_map, use_dynamic, dynamic) != 0) {
        fd_delete(node);
        return -ENOMEM;
    }
    // Link entry with node in lex list
    (*fe)->fdb = node;
    return 0;
}

/**
 * Deletes a filtering entry from fdb (both mirror and lex list).
 */
static void rtu_fdb_delete_entry(struct filtering_entry *fe)
{
    fd_delete(fe->fdb);
    rtu_sw_delete_entry(fe);
}

//------------------------------------------------------------------------------
// FDB mirror
//------------------------------------------------------------------------------

/**
 * Update FDB entry with dynamic or static info, as indicated by 'dynamic' param
 * @param port_mask VLAN port mask.
 * @return 1 if entry masks were updated. 0 otherwise.
 */
static int fe_update(
            struct filtering_entry *fe,
            uint32_t port_map,
            uint32_t port_mask,
            uint32_t use_dynamic,
            int dynamic)
{
    int update = 0;
    uint32_t mask_src, mask_dst;

    if (dynamic) {
        if (fe->static_fdb && !fe->use_dynamic)
            return 0;                       // dynamic info is not permitted
        // Override dynamic part of port map
        if (fe->use_dynamic)
            mask_dst = ((fe->port_mask_dst & !fe->use_dynamic) |
                        (port_map & fe->use_dynamic));
        else
            mask_dst = port_map;
        if (!fe->dynamic) {
            fe->dynamic = DYNAMIC;          // now entry contains dyn info
            num_dynamic_entries[fe->fid]++;
        }
    } else {
        mask_dst = port_map;                // Static part of forward vector
        if (use_dynamic) {                  // If dynamic info can be used...
            if (fe->dynamic)                // ...and entry contains dyn info
                // Add dynamic part of forward vector.
                mask_dst |= ((fe->port_mask_dst & fe->use_dynamic) &
                              use_dynamic);
            else
                // Apply default unicast/group behavior
                mask_dst |= use_dynamic;
        }
        fe->use_dynamic = use_dynamic;
    }
    mask_src = fe->port_mask_src | port_mask;

    if (fe->port_mask_dst != mask_dst) {
        fe->port_mask_dst = mask_dst;
        update = 1;
    }
    if (fe->port_mask_src != mask_src) {
        fe->port_mask_src = mask_src;
        update = 1;
    }

    return update;
}


/**
 * Deletes dynamic part of a given filtering entry.
 */
static void rtu_fdb_delete_dynamic_entry(struct filtering_entry *fe)
{
    num_dynamic_entries[fe->fid]--;
    if (fe->static_fdb) {
        // If entry contains some static info, just reset dynamic part
        // (i.e. set all dynamic bits to 1 = Forward)
        fe->port_mask_dst |= fe->use_dynamic;
        fe->dynamic = STATIC;
        rtu_sw_update_entry(fe);
    } else {
        // If entry is pure dynamic, just remove it (from lex list and mirror).
        rtu_fdb_delete_entry(fe);
    }
}

//------------------------------------------------------------------------------
// FDB - Static Info
//------------------------------------------------------------------------------


/**
 * Registers a static entry in static list associated to an fdb entry.
 */
static void fe_register_static_entry(
        struct static_filtering_entry *sfe,
        struct filtering_entry *fe)
{
    struct static_filtering_entry *t;

    // Make sure entry is not registered yet
    for (t = fe->static_fdb; t; t = t->next_sib) {
        if (t == sfe)
            return;
        if (!t->next_sib)
            break;
    }
    if(sfe->vid == WILDCARD_VID) {
        // wildcard entry added to tail
        if (fe->static_fdb)
            t->next_sib = sfe;
        else
            fe->static_fdb = sfe;
    } else {
        // non-wildcard entry added to head (or wildcard entry, if first one)
        sfe->next_sib  = fe->static_fdb;
        fe->static_fdb = sfe;
    }
}

/**
 * Unregisters a static entry from static list associated to an fdb entry.
 */
static void fe_unregister_static_entry(
        struct static_filtering_entry *sfe,
        struct filtering_entry *fe)
{
    struct static_filtering_entry *prev;

    if (fe->static_fdb == sfe) {
        fe->static_fdb = sfe->next_sib;
    } else {
        for (prev = fe->static_fdb; prev; prev = prev->next_sib) {
            if (prev->next_sib == sfe) {
                prev->next_sib = sfe->next_sib;
                break;
            }
        }
    }
}

/**
 * Inserts info corresponding to a static entry in the filtering database.
 * @param vid VLAN identifier (except wildcard VID).
 * @param sfe pointer to static entry contains static info to use.
 * @return 0 = entry inserted. 1 = entry unchanged. -ENOMEM = FDB full.
 */
static int fe_insert_static_entry(
            uint16_t vid,
            struct static_filtering_entry *sfe)
{
    int ret;
    struct vlan_table_entry *ve;
    struct filtering_entry *fe;
    uint32_t _port_map, _use_dynamic;       // final forward vector

    ve = rtu_sw_find_vlan_entry(vid);
    if (rtu_sw_find_entry(sfe->mac, ve->fid, &fe)) {
        // If an entry already exists in the fdb for the same mac and fid
        // just register static entry in static_fdb list associated to it
        // (if the static entry was only updated, it is already registered),
        // and recalculate the forward vector combining static and dynamic info
        fe_register_static_entry(sfe, fe);
        calculate_forward_vector(fe->static_fdb, &_port_map, &_use_dynamic);
        ret = fe_update(fe, vid, _port_map, _use_dynamic, STATIC);
    } else {
        // Create pure static entry
        calculate_forward_vector(sfe, &_port_map, &_use_dynamic);
        ret = rtu_fdb_create_entry(&fe, sfe->mac, vid, ve->fid, _port_map,
            _use_dynamic, STATIC);
        if (ret == 0)
            fe_register_static_entry(sfe, fe);
    }
    return ret;
}

/**
 * Removes info corresponding to a static entry from the filtering database.
 * @return 0 if entry was deleted. 1 if filtering entry was not found, or it did
 * not contain a reference to the given static entry or did not change when
 * removing the given static entry.
 */
static int fe_delete_static_entry(
            uint16_t vid,
            struct static_filtering_entry *sfe)
{
    struct filtering_entry *fe;
    struct vlan_table_entry *ve;
    uint32_t _port_map, _use_dynamic;       // final forward vector

    ve = rtu_sw_find_vlan_entry(vid);
    if (!rtu_sw_find_entry(sfe->mac, ve->fid, &fe))
        return 1;                           // filtering entry not found
    fe_unregister_static_entry(sfe, fe);
    if (!fe->static_fdb && !fe->dynamic) {
        // neither static nor dynamic info remains!
        rtu_fdb_delete_entry(fe);
        return 0;
    }
    // Combine all remaining static info and dynamic info (if any).
    calculate_forward_vector(fe->static_fdb, &_port_map, &_use_dynamic);
    return fe_update(fe, vid, _port_map, _use_dynamic, STATIC);
}

//------------------------------------------------------------------------------
// API
//------------------------------------------------------------------------------

uint16_t rtu_fdb_get_num_dynamic_entries(uint8_t fid)
{
    return num_dynamic_entries[fid];
}

uint32_t rtu_fdb_get_num_learned_entry_discards(uint8_t fid)
{
    return num_learned_entry_discards[fid];
}

uint16_t rtu_fdb_get_num_vlans(void)
{
    return rtu_sw_get_num_vlans();
}

uint16_t rtu_fdb_get_max_supported_vlans(void)
{
    // Operating in pure IVL mode, implementation would only support 255 VLANS!
    // (but no means to express learning constraints yet)
    return NUM_VLANS - NUM_RESERVED_VLANS;
}

uint16_t rtu_fdb_get_max_vid(void)
{
    return MAX_VID;
}

uint64_t rtu_fdb_get_num_vlan_deletes(void)
{
    return num_vlan_deletes;
}

/**
 * Set the polynomial used for hash calculation.
 * Changing the hash polynomial requires removing any existing
 * entry from RTU table.
 * Note in case RTU table becomes full, this function may
 * be used to change hash polynomial (thus leading to a different hash
 * distribution).
 * @param poly binary polynomial representation.
 * CRC-16-CCITT -> 1+x^5+x^12+x^16          -> 0x1021
 * CRC-16-IBM   -> 1+x^2+x^15+x^16          -> 0x8005
 * CRC-16-DECT  -> 1+x^3+x^7+x^8+x^10+x^16  -> 0x0589
 */
void rtu_fdb_set_hash_poly(uint16_t poly)
{
    pthread_mutex_lock(&fd_mutex);
    rtu_hw_write_hash_poly(poly);
    rtu_hash_set_poly(poly);
    pthread_mutex_unlock(&fd_mutex);
}

/**
 * Gets the aging time for dynamic filtering entries.
 * @param fid filtering database identifier (ignored currently)
 * @return aging time value [seconds]
 */
unsigned long rtu_fdb_get_aging_time(uint8_t fid)
{
    return aging_time;
}


/**
 * Sets the aging time for dynamic filtering entries.
 * @param fid filtering database identifier (ignored currently)
 * @param t new aging time value [seconds].
 * @return -EINVAL if t < 10 or t > 1000000 (802.1Q, Table 8.3); 0 otherwise.
 */
int rtu_fdb_set_aging_time(uint8_t fid, unsigned long t)
{
    if ((t < 10) || (t > 1000000))
        return -EINVAL;
    aging_time = t;
    return 0;
}

/**
 * Find the next FID in use, starting from the indicated FID.
 * Used to support management operations that traverse the filtering database.
 * @return next fid in use. NUM_FIDS if no next fid.
 */
uint16_t rtu_fdb_get_next_fid(uint8_t fid)
{
    int i;

    lock();
    for (i = fid; (i < NUM_FIDS) && !fd[i]; i++);
    unlock(0);
    return i;
}

//------------------------------------------------------------------------------
// FDB
//------------------------------------------------------------------------------

/**
 * Initializes the RTU filtering database.
 * @param poly hash polinomial.
 * @param aging aging time
 */
int rtu_fdb_init(uint16_t poly, unsigned long aging)
{
    int err, i;

    TRACE_DBG(TRACE_INFO, "clean filtering database.");
    rtu_sw_clean_fd();
    for (i = 0; i < NUM_VLANS; i++)
        sfd[0][i] = sfd[1][i] = NULL;
    for (i = 0; i < NUM_FIDS; i++)
        fd[i] = NULL;
    TRACE_DBG(TRACE_INFO, "clean vlan database.");
    rtu_sw_clean_vd();
    TRACE_DBG(TRACE_INFO, "clean aging map.");
    rtu_sw_clean_aging_map();
    TRACE_DBG(TRACE_INFO, "set aging time [%d].", aging);
    aging_time = aging;

    err = pthread_mutex_init(&fd_mutex, NULL);
    if (err)
        return err;

    TRACE_DBG(TRACE_INFO, "set hash poly [%d].", poly);
    rtu_fdb_set_hash_poly(poly);

    return 0;
}

/**
 * Creates or updates a dynamic filtering entry in filtering database.
 * @return 0 if entry was created or updated. -ENOMEM if FDB is full.
 */
int rtu_fdb_create_dynamic_entry(
            uint8_t  mac[ETH_ALEN],
            uint16_t vid,
            uint32_t port_map)
{
    int ret, fid;
    struct filtering_entry  *fe;
    struct vlan_table_entry *ve;

    // if VLAN is not registered ignore request
    ve = rtu_sw_find_vlan_entry(vid);
    if (!ve)
        return 0;
    // if member set for VID does not contain at least one port, ignore request
    if (ve->port_mask == 0)
        return 0;

    lock();
    fid = ve->fid;
    // Apply VLAN mask on port map
    port_map &= ve->port_mask;
    if (rtu_sw_find_entry(mac, fid, &fe)) {
        // Update entry that already exists.
        if (fe_update(fe, port_map, ve->port_mask, 0, DYNAMIC) != 0)
            return unlock(0);
    } else {
        ret = rtu_fdb_create_entry(&fe, mac, vid, fid, port_map, 0, DYNAMIC);
        if (ret == -ENOMEM) {
            num_learned_entry_discards[fid]++;
            return unlock(-ENOMEM);
        }
        num_dynamic_entries[fid]++;
    }
    rtu_sw_commit();
    return unlock(0);
}

/**
 * Reads an entry from the filtering database.
 * @param entry_type (OUT) filtering entry type. DYNAMIC if the entry
 * contains dynamic information. STATIC if no dynamic info is being used.
 * @return 0 if entry found and read. -EINVAL if entry does not exist.
 */
int rtu_fdb_read_entry(
           uint8_t mac[ETH_ALEN],               // in
           uint8_t fid,                         // in
           uint32_t *port_map,                  // out
           int *entry_type)                     // out
{
    struct filtering_entry *fe;

    lock();
    if (rtu_sw_find_entry(mac, fid, &fe)) {
        *port_map   = fe->port_mask_dst;
        *entry_type = fe->dynamic;
        return unlock(0);
    }
    return unlock(-EINVAL);
}

/**
 * Reads the next entry in the filtering database, following the one with
 * given MAC and FID,  in lexicographic order.
 * @return 0 if next entry exists and could be read. -EINVAL if no next entry.
 */
int rtu_fdb_read_next_entry(
           uint8_t (*mac)[ETH_ALEN],                            // inout
           uint8_t *fid,                                        // inout
           uint32_t *port_map,                                  // out
           int *entry_type)                                     // out
{
    struct filtering_entry *fe;
    struct filtering_entry_node *node;

    lock();

    node = fd_find(*mac, *fid);
    if (!node)
        return unlock(-EINVAL);

    node = fd_find_next(node);
    if (!node)
        return unlock(-EINVAL);

    if (!rtu_sw_find_entry(node->mac, node->fid, &fe))
        return unlock(-EINVAL);     // this should never happen

    mac_copy(*mac, fe->mac);
    *fid         = fe->fid;
    *port_map    = fe->port_mask_dst;
    *entry_type  = fe->dynamic;
    return unlock(0);
}

//------------------------------------------------------------------------------
// Static FDB
//------------------------------------------------------------------------------

/**
 * Creates or updates a static filtering entry in filtering database.
 * @param port_map a port map specification with a control element for each
 * outbound port to specify filtering for that MAC address specification and VID
 * @return 0 if entry created or updated. -ENOMEM if no memory available.-EINVAL
 * if entry is not registered. -EPERM if trying to modify a permanent entry.
 */
int  rtu_fdb_create_static_entry(
            uint8_t mac[ETH_ALEN],
            uint16_t vid,
            enum filtering_control port_map[NUM_PORTS],
            enum storage_type type,
            int active)
{
    int i, created = 0, ret = 0;
    struct static_filtering_entry *sfe;
    struct static_filtering_entry cache;

    // if VLAN is not registered ignore request
    // (but the wildcard VID can still be used for management)
    if ((vid != WILDCARD_VID) && !rtu_sw_find_vlan_entry(vid))
        return -EINVAL;

    lock();
    // Create/Update entry in static FDB
    sfe = sfe_find(mac, vid);
    if (sfe) {
        if (sfe->type == Permanent)
            return unlock(-EPERM);
        rtu_sfe_copy(&cache, sfe);
        if (sfe_update(sfe, port_map, active) != 0)
            return unlock(0);
    } else {
        sfe = sfe_create(mac, vid, port_map, type, active);
        if (!sfe)
            return unlock(-ENOMEM);
        created = 1;
    }
    // Register static entry in FDB
    rtu_sw_cache();
    if (vid == WILDCARD_VID) {
        // ...for all VIDs in use
        for (i = 0; i < NUM_VLANS; i++) {
            if (rtu_sw_find_vlan_entry(i)) {
                ret = fe_insert_static_entry(i, sfe);
                if (ret == -ENOMEM)
                    break;
            }
        }
    } else {
        ret = fe_insert_static_entry(vid, sfe);
    }
    // On error, rollback
    if (ret == -ENOMEM) {
        rtu_sw_uncache();
        rtu_sw_rollback();
        if (created)
            sfe_delete(sfe);
        else
            rtu_sfe_copy(sfe, &cache);
        return unlock(-ENOMEM);
    }
    // On success, commit
    rtu_sw_commit();
    return unlock(0);
}

/**
 * Removes a static filtering entry from the filtering database.
 * @return 0 if entry was deleted.-EPERM if entry removal is not permitted (i.e.
 * entry is Permanent or Read-Only). -EINVAL if entry is not registered.
 */
int rtu_fdb_delete_static_entry(uint8_t mac[ETH_ALEN], uint16_t vid)
{
    int i;
    struct static_filtering_entry *sfe;

    lock();
    // Check that VLAN is registered (except for wildcard vid)
    if ((vid != WILDCARD_VID) && !rtu_sw_find_vlan_entry(vid))
        return unlock(-EINVAL);
    // Check that entry is registered
    sfe = sfe_find(mac, vid);
    if (!sfe)
        return unlock(-EINVAL);
    // Permanent entries can not be removed
    if (sfe->type == Permanent)
        return unlock(-EPERM);
    // Unregister static entry from fdb
    if (vid == WILDCARD_VID) {
        // ...for all VIDs in use
        for(i = 0; i < NUM_VLANS; i++)
            if (rtu_sw_find_vlan_entry(i))
                fe_delete_static_entry(i, sfe);
    } else {
        fe_delete_static_entry(vid, sfe);
    }
    // Remove entry from static fdb
    sfe_delete(sfe);
    // Commit changes
    rtu_sw_commit();
    return unlock(0);
}

/**
 * Reads an static entry from the static filtering database
 * @return 0 if entry found and read. -EINVAL if entry does not exist.
 */
int rtu_fdb_read_static_entry(
            uint8_t mac[ETH_ALEN],                          // in
            uint16_t vid,                                   // in
            enum filtering_control (*port_map)[NUM_PORTS],  // out
            enum storage_type *type,                        // out
            int *active)                                    // out
{
    struct static_filtering_entry *sfe;

    lock();
    sfe = sfe_find(mac, vid);
    if (!sfe)
        return unlock(-EINVAL);
    memcpy(*port_map, sfe->port_map, NUM_PORTS);
    *type   = sfe->type;
    *active = sfe->active;
    return unlock(0);
}

/**
 * Reads the next static entry in the static filtering database, following the
 * one with indicated MAC and FID,  in lexicographic order.
 * Returns the entry MAC address, FID, port map and entry type.
 * @return 0 if next entry exists and could be read. -EINVAL if no next entry.
 */
int rtu_fdb_read_next_static_entry(
        uint8_t (*mac)[ETH_ALEN],                           // inout
        uint16_t *vid,                                      // inout
        enum filtering_control (*port_map)[NUM_PORTS],      // out
        enum storage_type *type,                            // out
        int *active)                                        // out
{
    struct static_filtering_entry *sfe;

    lock();
    sfe = sfe_find(*mac, *vid);
    if (!sfe)
        return unlock(-EINVAL);

    sfe = sfe_find_next(sfe);
    if (!sfe)
        return unlock(-EINVAL);

    mac_copy(*mac, sfe->mac);
    memcpy(*port_map, sfe->port_map, NUM_PORTS);
    *vid    = sfe->vid;
    *type   = sfe->type;
    *active = sfe->active;
    return unlock(0);
}

//------------------------------------------------------------------------------
// VLAN Table
//------------------------------------------------------------------------------

/**
 * Creates a static VLAN entry in the VLAN table
 * @param member_set registrar administrative control for the GVRP protocol.
 * @param untagged_set indicates whether frames are to be VLAN tagged or
 * untagged when transmitted on a given port.
 * @return 0 if entry was created. -EINVAL if vid is reserved or VLAN exists.
 * -ENOMEM in case no memory is available to create the entry.
 */
int rtu_fdb_create_static_vlan_entry(
        uint16_t vid,
        uint8_t fid,
        enum registrar_control member_set[NUM_PORTS],
        uint32_t untagged_set)
{
    int ret = 0, xcast;
    uint32_t port_mask, use_dynamic;
    struct static_filtering_entry *sfe;

    lock();
    rtu_sw_cache();
    // Insert VLAN registration entry into VLAN table
    calculate_vlan_vector(member_set, &port_mask, &use_dynamic);
    rtu_sw_create_vlan_entry(vid, fid, port_mask, use_dynamic, untagged_set, STATIC);
    // Static entries for the Wildcard VID now must also apply to this VLAN
    for (xcast = 0; xcast < 2; xcast++) {
        for (sfe = sfd[xcast][WILDCARD_VID]; sfe; sfe = sfe->next) {
            ret = fe_insert_static_entry(vid, sfe);
            if (ret == -ENOMEM) {
                // Rollback insertions
                rtu_sw_uncache();
                rtu_sw_rollback();
                return unlock(-ENOMEM);
            }
        }
    }
    rtu_sw_commit();
    return unlock(0);
}

/**
 * Deletes a static VLAN entry from the VLAN table.
 * When a static VLAN entry is removed, any static or dynamic info contained in
 * the FDB which refers exclusively to such VLAN is also removed.
 * @return -EINVAL if vid is reserved or VLAN not registered. 0 if was removed.
 */
int rtu_fdb_delete_static_vlan_entry(uint16_t vid)
{
    int fid, xcast;
    struct filtering_entry_node *node, *next;
    struct filtering_entry *fe;
    struct static_filtering_entry *sfe;
    struct vlan_table_entry *ve;

    if (reserved(vid))
        return -EINVAL;

    lock();
    ve = rtu_sw_find_vlan_entry(vid);
    if (!ve || ve->dynamic)
        return unlock(-EINVAL);

    // Delete static entries for VID (from both static FDB and FDB)
   for (xcast = 0; xcast < 2; xcast++) {
        for (sfe = sfd[xcast][vid]; sfe; sfe = sfe->next) {
            fe_delete_static_entry(vid, sfe);
            sfe_delete(sfe);
        }
    }
    // Indirect static info and dynamic info can only be removed from FDB
    // if not shared with other VLANS (i.e. no other VID maps to the same FID).
    fid = ve->fid;
    if (!rtu_sw_fid_shared(fid)) {
        // Delete FDB info originated from static entries for wildcard VID
        for (xcast = 0; xcast < 2; xcast++)
            for (sfe = sfd[xcast][WILDCARD_VID]; sfe; sfe = sfe->next)
                fe_delete_static_entry(vid, sfe);
        // Delete dynamic entries for FID assigned exclusively to VID
        for (node = fd[fid]; node; node = next) {
            next = node->next;
            if (rtu_sw_find_entry(node->mac, fid, &fe))
                rtu_fdb_delete_dynamic_entry(fe);
        }
    }
    // Delete VLAN registration entry from VLAN table
    rtu_sw_delete_vlan_entry(vid);
    rtu_sw_commit();
    num_vlan_deletes++;
    return unlock(0);
}

/**
 * Reads a static vlan entry from the VLAN database.
 * @return 0 if entry found and read.  -EINVAL if vid is not valid.
 */
int rtu_fdb_read_static_vlan_entry(
            uint16_t vid,                                           // in
	        enum registrar_control (*member_set)[NUM_PORTS],        // out
	        uint32_t *untagged_set)                                 // out
{
    struct vlan_table_entry *ve;

    if (vid >= NUM_VLANS)
        return -EINVAL;

    lock();
    ve = rtu_sw_find_vlan_entry(vid);
    if (!ve || ve->dynamic)
        return unlock(-EINVAL);
    calculate_member_set(member_set, ve->port_mask, ve->use_dynamic);
    *untagged_set = ve->untagged_set;
    return unlock(0);
}

/**
 * Reads next static vlan entry from the VLAN database.
 * @param vid (IN) starting VLAN identifier. (OUT) vid of next vlan entry.
 * @return 0 if entry found and read. -EINVAL if vid not valid or no next VLAN.
 */
int rtu_fdb_read_next_static_vlan_entry(
        uint16_t *vid,                                      // inout
        enum registrar_control (*member_set)[NUM_PORTS],    // out
        uint32_t *untagged_set)                             // out
{
    struct vlan_table_entry *ve;

    if (*vid >= NUM_VLANS)
        return -EINVAL;

    lock();
    // find next VLAN entry that is static
    do {
        ve = rtu_sw_find_next_ve(vid);
    } while(ve && ve->dynamic);
    if (!ve)
        return unlock(-EINVAL);
    calculate_member_set(member_set, ve->port_mask, ve->use_dynamic);
    *untagged_set = ve->untagged_set;
    return unlock(0);
}

/**
 * Reads a vlan entry from the VLAN database.
 * @return 0 if entry found and read. -EINVAL if vid not valid.
 */
int rtu_fdb_read_vlan_entry(
        uint16_t vid,
        uint8_t *fid,                                       // out
        int *entry_type,                                    // out
        enum registrar_control (*member_set)[NUM_PORTS],    // out
        uint32_t *untagged_set,                             // out
        unsigned long *creation_t)                          // out
{
    struct vlan_table_entry *ve;

    if (vid >= NUM_VLANS)
        return -EINVAL;

    lock();
    ve = rtu_sw_find_vlan_entry(vid);
    if (!ve)
        return unlock(-EINVAL);
    *fid          = ve->fid;
    *entry_type   = ve->dynamic;
    *creation_t   = ve->creation_t;
    *untagged_set = ve->untagged_set;
    calculate_member_set(member_set, ve->port_mask, ve->use_dynamic);
    return unlock(0);
}

/**
 * Reads next vlan entry from the VLAN database.
 * @param vid (IN) starting VLAN identifier. (OUT) vid of next vlan entry.
 * @return 0 if entry found and read. -EINVAL if vid not valid.
 */
int rtu_fdb_read_next_vlan_entry(
            uint16_t *vid,                                      // inout
            uint8_t *fid,                                       // out
            int *entry_type,                                    // out
            enum registrar_control (*member_set)[NUM_PORTS],    // out
            uint32_t *untagged_set,                             // out
            unsigned long *creation_t)                          // out
{
    struct vlan_table_entry *ve;

    if (*vid >= NUM_VLANS)
        return -EINVAL;

    lock();
    ve = rtu_sw_find_next_ve(vid);
    if (!ve)
        return unlock(-EINVAL);
    *fid          = ve->fid;
    *entry_type   = ve->dynamic;
    *creation_t   = ve->creation_t;
    *untagged_set = ve->untagged_set;
    calculate_member_set(member_set, ve->port_mask, ve->use_dynamic);
    return unlock(0);
}

//------------------------------------------------------------------------------
// Aging
//------------------------------------------------------------------------------

/**
 * Deletes old filtering entries from filtering database.
 */
void rtu_fdb_age_dynamic_entries(void)
{
    int fid;
    struct filtering_entry_node *node;
    struct filtering_entry *fe;
    unsigned long t;            // (secs)

    rtu_sw_update_aging_map();  // Work with latest access info
    rtu_sw_update_fd_age();     // Update filtering entries age

    lock();
    t = now() - aging_time;
    for (fid = 0; fid < NUM_FIDS; fid++) {
        for (node = fd[fid]; node; node = node->next) {
            if (rtu_sw_find_entry(node->mac, fid, &fe)) {
                if(fe->dynamic && time_after(t, fe->last_access_t)) {
                    rtu_fdb_delete_dynamic_entry(fe);
                }
            }
        }
    }
    rtu_sw_commit();            // Commit changes
    unlock(0);
    rtu_sw_clean_aging_map();   // Keep track of entries access in next period
}
