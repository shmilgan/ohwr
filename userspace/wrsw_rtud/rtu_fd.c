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

#include <net-snmp/library/snmp-tc.h>

#include "rtu_fd.h"
#include "rtu_sw.h"
#include "rtu_drv.h"
#include "rtu_hash.h"

#include "utils.h"

/**
 * RTU Filtering Database Entry Node in lexicographic ordered list.
 */
struct filtering_entry_node {
    uint8_t mac[ETH_ALEN];
    uint8_t fid;
    struct filtering_entry_node *next; // Double linked list
    struct filtering_entry_node *prev; // (in lexicographic order)
};

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

/* Restricted VLAN registration port map */
static uint32_t restricted_vlan_reg;

static inline
int reserved(int vid)
{
    return
#ifdef V3
        (vid == 0) ||
#endif // V3
        (vid == WILDCARD_VID);
}

static inline
int illegal(int port)
{
    return (port < 0) || (port > NUM_PORTS);
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

static inline
int is_restricted_vlan_reg(int port)
{
    return (restricted_vlan_reg >> port) & 0x01;
}

static inline
int is_normal_vlan_reg(struct vlan_table_entry *vlan, int port)
{
    return vlan->use_dynamic & (1 << port);
}

//------------------------------------------------------------------------------
// Auxiliary functions to convert 802.1Q data types into low level data types.
//------------------------------------------------------------------------------

/**
 * Combines port_map information from a list of static entries.
 * @param sfe pointer to the list of static entries that must be combined.
 */
static void calculate_forward_vector(struct static_filtering_entry *sfe,
                                     uint32_t *port_map,
                                     uint32_t *use_dynamic)
{
    int i;

    // If any entry specifies Forward for this port, then result is Forward
    // else if any entry specifies Filter for this port, then result is Filter
    // else use Dynamic info
    *use_dynamic = 0xFFFFFFFF;
    *port_map    = 0x00000000;
    for (i = 0; sfe; sfe = sfe->next_sib, i++) {
        // Only active entries are used to compute the forward vector
        if (!sfe->active)
            continue;
        // The static filtering information specified for the wildcard VID only
        // applies to VLANs for which no specific static filtering entry exists.
        if ((i > 0) && (sfe->vid == WILDCARD_VID))
            break;
        *port_map    |= sfe->egress_ports;
        *use_dynamic &= ~(sfe->egress_ports | sfe->forbidden_ports);
    }
}

static int is_bpdu(struct static_filtering_entry *sfe)
{
    int i;

    for (i = 0; sfe; sfe = sfe->next_sib, i++) {
        if (!sfe->active)
            continue;
        if ((i > 0) && (sfe->vid == WILDCARD_VID))
            break;
        if (sfe->is_bpdu)
            return 1;
    }
    return 0;
}

//------------------------------------------------------------------------------
// Static FDB - Double Linked List (lexicografically ordered)
//------------------------------------------------------------------------------

/**
 * Creates a filtering entry in the static FDB linked list.
 * @return pointer to entry created. NULL if no memory available
 */
static struct static_filtering_entry *sfe_create(uint8_t  mac[ETH_ALEN],
                                                 uint16_t vid,
                                                 uint32_t egress_ports,
                                                 uint32_t forbidden_ports,
                                                 int type,
                                                 int active,
                                                 int is_bpdu)
{
    int xcast;
    struct static_filtering_entry *node, *prev, *sfe, **ptr;

    sfe = (struct static_filtering_entry*)
          malloc(sizeof(struct static_filtering_entry));

    if (!sfe)
        return NULL;

    sfe->vid             = vid;
    sfe->egress_ports    = egress_ports;
    sfe->forbidden_ports = forbidden_ports;
    sfe->type            = type;
    sfe->active          = active;
    sfe->is_bpdu         = is_bpdu;
    sfe->next_sib        = NULL;
    mac_copy(sfe->mac, mac);

    xcast = mac_multicast(mac);

    // Link into lexicographically ordered list
    for (ptr = &sfd[xcast][vid], node = sfd[xcast][vid], prev = NULL;
         node && mac_cmp(sfe->mac, node->mac) > 0;
         ptr = &node->next, prev = node, node = node->next)
         ;
    sfe->prev = prev;
    *ptr = sfe;
    sfe->next = node;
    if (node) node->prev = sfe;

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
static struct static_filtering_entry *sfe_find(uint8_t mac[ETH_ALEN],
                                               uint16_t vid)
{
    int cmp;
    struct static_filtering_entry *node;

    for (node = sfd[mac_multicast(mac)][vid];
         node && ((cmp = mac_cmp(mac, node->mac)) > 0);
         node = node->next)
        ;
    return node && cmp == 0 ? node:NULL;
}


/**
 * Finds the filtering entry following the given one in lexicographic order.
 * @return pointer to next entry in lexicographic order. NULL if none found.
 */
struct static_filtering_entry *sfe_find_next(uint8_t mac[ETH_ALEN], uint32_t vid)
{
    int xcast;
    struct static_filtering_entry *node;

    xcast = mac_multicast(mac);
    // First try to find next entry for same VID.
    for (node = sfd[xcast][vid]; node; node = node->next)
        if (mac_cmp(node->mac, mac) > 0)
            return node;

    // If none found, check entries for following FIDs
    for (vid = vid + 1; vid < NUM_VLANS; vid++)
        if (sfd[xcast][vid])
            return sfd[xcast][vid];

    // No more entries in static FDB
    return NULL;
}

/**
 * Updates static FDB entry.
 * @return 0 if entry was updated. 1 otherwise.
 */
static int sfe_update(struct static_filtering_entry *sfe,
                      uint32_t egress_ports,
                      uint32_t forbidden_ports,
                      int active,
                      int is_bpdu)
{
    // TODO persistence support
    // TODO handle storage type update

    int ret = 1;

    if (sfe->active != active) {
        sfe->active = active;
        ret = 0;
    }
    if (sfe->egress_ports != egress_ports) {
        sfe->egress_ports = egress_ports;
        ret = 0;
    }
    if (sfe->forbidden_ports != forbidden_ports) {
        sfe->forbidden_ports = forbidden_ports;
        ret = 0;
    }
    if (sfe->is_bpdu != is_bpdu) {
        sfe->is_bpdu = is_bpdu;
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
static struct filtering_entry_node *fd_create(uint8_t mac[ETH_ALEN], uint8_t fid)
{
    struct filtering_entry_node *node, *prev, *fe, **ptr;

    fe = (struct filtering_entry_node*) malloc(sizeof(*fe));

    if (!fe)
        return NULL;

    mac_copy(fe->mac, mac);
    fe->fid = fid;
    // Link into lexicographically ordered list
    for (ptr = &fd[fid], node = fd[fid], prev = NULL;
         node && mac_cmp(fe->mac, node->mac) > 0;
         ptr = &node->next, prev = node, node = node->next)
         ;
    fe->prev = prev;
    *ptr = fe;
    fe->next = node;
    if (node) node->prev = fe;

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
    else                           // first node in fdb double linked list
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
    int cmp;
    struct filtering_entry_node *node;

    for (node = fd[fid];
         node && ((cmp = mac_cmp(mac, node->mac)) > 0);
         node = node->next)
        ;
    return node && cmp == 0 ? node:NULL;
}

/**
 * Finds the filtering entry following the given one in lexicographic order.
 * @return pointer to next entry in lexicographic order. NULL if none found.
 */
struct filtering_entry_node *fd_find_next(uint8_t mac[ETH_ALEN], uint16_t fid)
{
    struct filtering_entry_node *node;

    // First try to find next entry for same FID.
    for (node = fd[fid]; node; node = node->next)
        if (mac_cmp(node->mac, mac) > 0)
            return node;

    // In none found, check entries for following FIDs
    for (fid = fid + 1; fid < NUM_FIDS; fid++)
        if (fd[fid])
            return fd[fid];

    // No more entries in FDB
    return NULL;
}

/**
 * Creates a filtering entry in fdb (both mirror and lex list).
 * @return 0 if entry was created. -ENOMEM if fdb is full.
 */
static int rtu_fdb_create_entry(struct filtering_entry **fe,
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
 * @param use_dynamic the new use_dynamic mask (only makes sense if static infi
 * is being provided.
 * @return 0 if entry masks were updated. 1 otherwise.
 */
static int fe_update(struct filtering_entry *fe,
                     uint32_t port_map,
                     uint32_t port_mask,
                     uint32_t use_dynamic,
                     int dynamic)
{
    int ret = 1;
    uint32_t mask_src, mask_dst;
    uint32_t port_map_static, port_map_dynamic;

    port_map_static  = fe->port_mask_dst & ~fe->use_dynamic;
    port_map_dynamic = fe->port_mask_dst & fe->use_dynamic;

    if (dynamic) { /* dynamic info is being provided */
        if (fe->static_fdb && !fe->use_dynamic)
            return 1;   /* ...but dynamic info is not permitted */
        /* Override dynamic part of port map
           If entry is static, pick up just those ports permitted by dyn mask */
        mask_dst = fe->static_fdb
            ? port_map_static | (port_map & fe->use_dynamic)
            : port_map;

        /* if entry was not dynamic then this is a new one */
        if (!fe->dynamic) {
            fe->dynamic = DYNAMIC;
            num_dynamic_entries[fe->fid]++;
        }
    } else {  /* static info is being provided */
        /* If dynamic info can now be used and entry contains dyn info, keep it.
           Otherwise apply default unicast/group behaviour */
        mask_dst = use_dynamic && fe->dynamic
            ? port_map | (port_map_dynamic & use_dynamic)
            : port_map | use_dynamic;
        fe->use_dynamic = use_dynamic;
    }
    mask_src = fe->port_mask_src | port_mask;

    if (fe->port_mask_dst != mask_dst) {
        fe->port_mask_dst = mask_dst;
        ret = 0;
    }
    if (fe->port_mask_src != mask_src) {
        fe->port_mask_src = mask_src;
        ret = 0;
    }

    if (ret == 0)
        rtu_sw_update_entry(fe);

    return ret;
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
static void fe_register_static_entry(struct static_filtering_entry *sfe,
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
static void fe_unregister_static_entry(struct static_filtering_entry *sfe,
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
static int fe_insert_static_entry(uint16_t vid,
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
        ret = fe_update(fe, _port_map, ve->port_mask, _use_dynamic, STATIC);
    } else {
        // Create pure static entry
        calculate_forward_vector(sfe, &_port_map, &_use_dynamic);
        ret = rtu_fdb_create_entry(&fe, sfe->mac, vid, ve->fid, _port_map,
            _use_dynamic, STATIC);
        if (ret == 0)
            fe_register_static_entry(sfe, fe);
    }
    fe->is_bpdu |= sfe->is_bpdu;
    return ret;
}

/**
 * Removes info corresponding to a static entry from the filtering database.
 * @return 0 if entry was deleted. 1 if filtering entry was not found, or it did
 * not contain a reference to the given static entry or did not change when
 * removing the given static entry.
 */
static int fe_delete_static_entry(uint16_t vid,
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
    fe->is_bpdu = is_bpdu(fe->static_fdb);
    return fe_update(fe, _port_map, ve->port_mask, _use_dynamic, STATIC);
}

static int fe_insert_wildcard_static_entries(uint16_t vid)
{
    int xcast;
    struct static_filtering_entry *sfe;

    for (xcast = 0; xcast < 2; xcast++) {
        for (sfe = sfd[xcast][WILDCARD_VID]; sfe; sfe = sfe->next) {
            if (fe_insert_static_entry(vid, sfe) < 0)
                return -ENOMEM;
        }
    }
    return 0;
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

/**
 * Gets the maximum number of entries that can be held in the Filtering
 * Database.
 * @return Filtering Database size.
 */
int rtu_fdb_get_size(void)
{
    return RTU_ENTRIES;
}

/**
 * Gets the number of Static Filtering Entries in the Switch's Filtering
 * Database.
 * @return number of Static Filtering Entries.
 */
int rtu_fdb_get_num_all_static_entries(void)
{
    int xcast, vid, n = 0;
    struct static_filtering_entry *entry;

    for (xcast = 0; xcast < 2; xcast++) {
        for (vid = 0 ; vid < NUM_VLANS; vid++) {
            entry = sfd[xcast][vid];
            while (entry) {
                n++;
                entry = entry->next;
            }
        }
    }
    return n;
}

/**
 * Gets the number of Dynamic Filtering Entries in the Switch's Filtering
 * Database.
 * @return number of Dynamic Filtering Entries.
 */
int rtu_fdb_get_num_all_dynamic_entries(void)
{
    int fid, n;

    for (fid = 0, n = 0; fid < NUM_FIDS; fid++)
        n += num_dynamic_entries[fid];
    return n;
}

/**
 * Gets the number of Static VLAN Registration Entries in the Switch's
 * Filtering Database.
 * @return number of Static VLAN Registration Entries.
 */
int rtu_vfdb_get_num_all_static_entries(void)
{
    return rtu_sw_get_num_static_vlan_entries();
}

/**
 * Gets the number of Dynamic VLAN Registration Entries in the Switch's
 * Filtering Database.
 * @return number of Dynamic VLAN Registration Entries.
 */
int rtu_vfdb_get_num_all_dynamic_entries(void)
{
    return rtu_sw_get_num_dynamic_vlan_entries();
}

uint16_t rtu_fdb_get_max_supported_vlans(void)
{
    // Operating in pure IVL mode, implementation would only support 255 VLANS!
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
    for (i = fid + 1; (i < NUM_FIDS) && !fd[i]; i++);
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
    int err;

    TRACE_DBG(TRACE_INFO, "clean filtering database.");
    rtu_sw_clean_fd();
    memset(&fd, 0, sizeof(fd));//sizeof(struct filtering_entry_node *) * NUM_FIDS);
    memset(&sfd, 0, sizeof(sfd));//sizeof(struct static_filtering_entry *) * 2 * NUM_VLANS);
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
int rtu_fdb_create_dynamic_entry(uint8_t  mac[ETH_ALEN],
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
int rtu_fdb_read_entry(uint8_t mac[ETH_ALEN],  // in
                       uint8_t fid,            // in
                       uint32_t *port_map,     // out
                       int *entry_type)        // out
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
int rtu_fdb_read_next_entry(uint8_t (*mac)[ETH_ALEN],   // inout
                            uint8_t *fid,               // inout
                            uint32_t *port_map,         // out
                            int *entry_type)            // out
{
    struct filtering_entry *fe;
    struct filtering_entry_node *node;

    lock();

    node = fd_find_next(*mac, *fid);
    if (!node)
        return unlock(-EINVAL);

    if (!rtu_sw_find_entry(node->mac, node->fid, &fe))
        return unlock(-EINVAL);     // this should never happen, since fd and
                                    // htab/hcam should be always consistent
    mac_copy(*mac, fe->mac);
    *fid         = fe->fid;
    *port_map    = fe->port_mask_dst;
    *entry_type  = fe->dynamic;
    return unlock(0);
}

void rtu_fdb_delete_dynamic_entries(int port, uint16_t vid)
{
    if (illegal(port) || reserved(vid))
        return;
    lock();
    rtu_sw_delete_dynamic_entries(port, vid);
    rtu_sw_commit();
    unlock(0);
}

/**
 * Creates or updates a dynamic vlan registration entry.
 * @return 0 if entry was created or updated. -1 if vlan registration is
 * restricted and there is no static vlan entry. -EINVAL if port number is not
 * valid or vid is reserved.
 */
int rtu_vfdb_forward_dynamic(int port, uint16_t vid)
{
    int err = 0;
    struct vlan_table_entry *vlan;

    if (reserved(vid) || illegal(port))
        return -EINVAL;

    /* If the value of the Restricted_VLAN_Registration is TRUE, a dynamic entry
    for a given VLAN may only be created if a Static VLAN Registration Entry
    already exists for that VLAN, in which the Registrar Administrative Control
    value is Normal Registration */
    lock();
    vlan = rtu_sw_find_vlan_entry(vid);
    if (is_restricted_vlan_reg(port) && !(vlan && vlan->is_static))
        return unlock(-1);
    if (vlan) {
        if (vlan->is_static && !is_normal_vlan_reg(vlan, port))
            return unlock(-1);
        vlan->port_mask |= (1 << port);
        rtu_sw_update_vlan_entry(vlan);
    } else {
        rtu_sw_cache();
        /* Note: If no static entry exists for a VLAN, then it is assumed
        that frames for that VLAN are transmitted VLAN-tagged on all Ports */
        rtu_sw_create_vlan_entry(vid, (1 << port), 0, 0, DYNAMIC);
        /* Allocate VID to FID based on fixed allocations and learning constraints*/
        err = rtu_sw_allocate_fid(vid);
        if (err)
            goto rollback;
        /* Static entries for the Wildcard VID must also apply to this VLAN */
        err = fe_insert_wildcard_static_entries(vid);
        if (err)
            goto rollback;
    }
    rtu_sw_commit();
    return unlock(err);

rollback:
    rtu_sw_uncache();
    rtu_sw_rollback();
    return unlock(err);
}

/**
 * Delete a dynamic vlan registration entry for the given VLAN and port.
 */
int rtu_vfdb_filter_dynamic(int port, uint16_t vid)
{
    struct vlan_table_entry *vlan;

    if (reserved(vid))
        return -EINVAL;

    if ((port < 0) || (port > NUM_PORTS))
        return -EINVAL;

    lock();
    vlan = rtu_sw_find_vlan_entry(vid);
    if (vlan) {
        /* If vlan entry is dynamic, entry can be filtered out straight forward.
        If vlan entry is static, normal registration is required */
        if (vlan->is_static && !is_normal_vlan_reg(vlan, port))
            return unlock(-1);
        /* Filter out port from vlan */
        unset(&vlan->port_mask, port);
        /* Either update or remove vlan (in case it contains no other ports) */
        if (vlan->port_mask)
            rtu_sw_update_vlan_entry(vlan);
        else
            rtu_sw_delete_vlan_entry(vid);
        // TODO remove FDB entries for VLAN and port
        rtu_sw_commit();
    }
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
int  rtu_fdb_create_static_entry(uint8_t mac[ETH_ALEN],
                                 uint16_t vid,
                                 uint32_t egress_ports,
                                 uint32_t forbidden_ports,
                                 int type,
                                 int active,
                                 int is_bpdu)
{
    int i, created = 0, ret = 0;
    struct static_filtering_entry *sfe;
    struct static_filtering_entry cache;

    TRACE_DBG(
        TRACE_INFO,
        "create static entry: vid=%d mac=%s egress_ports=%x forbidden_ports=%x)",
        vid, mac_to_string(mac), egress_ports, forbidden_ports);

    // if VLAN is not registered ignore request
    // (but the wildcard VID can still be used for management)
    if ((vid != WILDCARD_VID) && !rtu_sw_find_vlan_entry(vid))
        return -EINVAL;

    // The same port cannot be a member of both egress ports and forbidden ports
    if (egress_ports & forbidden_ports)
        return -EINVAL;

    lock();
    // Create/Update entry in static FDB
    sfe = sfe_find(mac, vid);
    if (sfe) {
        if (sfe->type == ST_READONLY)
            return unlock(-EPERM);
        rtu_sfe_copy(&cache, sfe);
        if (sfe_update(sfe, egress_ports, forbidden_ports, active, is_bpdu) != 0)
            return unlock(0);
    } else {
        sfe = sfe_create(
                mac, vid, egress_ports, forbidden_ports, type, active, is_bpdu);
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

    TRACE_DBG(
        TRACE_INFO,
        "delete static entry: vid=%d mac=%s", vid, mac_to_string(mac));

    lock();
    // Check that entry is registered
    sfe = sfe_find(mac, vid);
    if (!sfe)
        return unlock(-EINVAL);
    // Permanent entries can not be removed
    if (sfe->type == ST_READONLY)
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
int rtu_fdb_read_static_entry(uint8_t mac[ETH_ALEN],        // in
                              uint16_t vid,                 // in
                              uint32_t *egress_ports,       // out
                              uint32_t *forbidden_ports,    // out
                              int *type,                    // out
                              int *active)                  // out
{
    struct static_filtering_entry *sfe;

    lock();
    sfe = sfe_find(mac, vid);
    if (!sfe)
        return unlock(-EINVAL);
    *egress_ports    = sfe->egress_ports;
    *forbidden_ports = sfe->forbidden_ports;
    *type            = sfe->type;
    *active          = sfe->active;
    return unlock(0);
}

/**
 * Reads the next static entry in the static filtering database, following the
 * one with indicated MAC and FID,  in lexicographic order.
 * Returns the entry MAC address, FID, port map and entry type.
 * @return 0 if next entry exists and could be read. -EINVAL if no next entry.
 */
int rtu_fdb_read_next_static_entry(uint8_t (*mac)[ETH_ALEN],    // inout
                                   uint16_t *vid,               // inout
                                   uint32_t *egress_ports,      // out
                                   uint32_t *forbidden_ports,   // out
                                   int *type,                   // out
                                   int *active)                 // out
{
    struct static_filtering_entry *sfe;

    lock();
    sfe = sfe_find_next(*mac, *vid);
    if (!sfe)
        return unlock(-EINVAL);
    *vid             = sfe->vid;
    *egress_ports    = sfe->egress_ports;
    *forbidden_ports = sfe->forbidden_ports;
    *type            = sfe->type;
    *active          = sfe->active;
    mac_copy(*mac, sfe->mac);
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
int rtu_fdb_create_static_vlan_entry(uint16_t vid,
                                     uint32_t egress_ports,
                                     uint32_t forbidden_ports,
                                     uint32_t untagged_set)
{
    int err;
    uint32_t port_mask, use_dynamic;
    struct vlan_table_entry *ve;

    TRACE_DBG(
        TRACE_INFO,
        "create static vlan entry: vid=%d egress_ports=%x forbidden_ports=%x untagged_set=%x",
        vid, egress_ports, forbidden_ports, untagged_set);

    if (reserved(vid))
        return -EINVAL;

    // The same port cannot be a member of both egress ports and forbidden ports
    if (egress_ports & forbidden_ports)
        return -EINVAL;

    lock();
    rtu_sw_cache();
    ve = rtu_sw_find_vlan_entry(vid);
    // Insert VLAN registration entry into VLAN table (keeping dyn info, if any)
    use_dynamic = ~(egress_ports | forbidden_ports);
    port_mask = ve ?
        (egress_ports | ((ve->port_mask & ve->use_dynamic) & use_dynamic)):
         egress_ports;
    rtu_sw_create_vlan_entry(vid, port_mask, use_dynamic, untagged_set, STATIC);

    /* Allocate VID to FID based on fixed allocations and learning constraints*/
    err = rtu_sw_allocate_fid(vid);
    if (err)
        goto rollback;

    // Static entries for the Wildcard VID now must also apply to this VLAN
    err = fe_insert_wildcard_static_entries(vid);
    if (err)
        goto rollback;

    rtu_sw_commit();
    return unlock(err);

rollback:
    rtu_sw_uncache();
    rtu_sw_rollback();
    return unlock(err);
}

/**
 * Deletes a static VLAN entry from the VLAN table.
 * @return -EINVAL if vid is reserved or VLAN not registered. 0 if was removed.
 */
int rtu_fdb_delete_static_vlan_entry(uint16_t vid)
{
    int fid, xcast;
    struct filtering_entry_node *node, *next;
    struct filtering_entry *fe;
    struct static_filtering_entry *sfe, *next_sfe;
    struct vlan_table_entry *ve;

    TRACE_DBG(TRACE_INFO, "delete static vlan entry: vid=%d", vid);

    if (reserved(vid))
        return -EINVAL;

    lock();
    ve = rtu_sw_find_vlan_entry(vid);
    if (!ve)
        return unlock(-EINVAL);

    // Delete all static information
    ve->port_mask &= ve->use_dynamic;
    // Dynamic info for restricted vlan registration ports must also be removed
    ve->port_mask &= ~restricted_vlan_reg;

    if (ve->port_mask == 0) {
        // Delete static entries for VID (from both static FDB and FDB)
        for (xcast = 0; xcast < 2; xcast++) {
            for (sfe = sfd[xcast][vid]; sfe; sfe = next_sfe) {
                next_sfe = sfe->next;
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
        num_vlan_deletes++;
    } else {
        // Dynamic info still remains
        rtu_sw_update_vlan_entry(ve);
    }
    rtu_sw_commit();
    return unlock(0);
}

/**
 * Reads a static vlan entry from the VLAN database.
 * @return 0 if entry found and read.  -EINVAL if vid is not valid.
 */
int rtu_fdb_read_static_vlan_entry(uint16_t vid,                // in
                                   uint32_t *egress_ports,      // out
                                   uint32_t *forbidden_ports,   // out
                                   uint32_t *untagged_set)      // out
{
    struct vlan_table_entry *ve;

    if ((vid >= NUM_VLANS) || reserved(vid))
        return -EINVAL;

    lock();
    ve = rtu_sw_find_vlan_entry(vid);
    if (!ve || !ve->is_static)
        return unlock(-EINVAL);
    *egress_ports    = ve->port_mask;
    *forbidden_ports = ~ve->use_dynamic & ~ve->port_mask;
    *untagged_set    = ve->untagged_set;
    return unlock(0);
}

/**
 * Reads next static vlan entry from the VLAN database.
 * @param vid (IN) starting VLAN identifier. (OUT) vid of next vlan entry.
 * @return 0 if entry found and read. -EINVAL if vid not valid or no next VLAN.
 */
int rtu_fdb_read_next_static_vlan_entry(uint16_t *vid,              // inout
                                        uint32_t *egress_ports,     // out
                                        uint32_t *forbidden_ports,  // out
                                        uint32_t *untagged_set)     // out
{
    struct vlan_table_entry *ve;

    if (*vid >= NUM_VLANS)
        return -EINVAL;

    lock();
    // find next VLAN entry that is static
    do {
        ve = rtu_sw_find_next_ve(vid);
    } while(ve && !ve->is_static);
    if (!ve)
        return unlock(-EINVAL);
    *egress_ports    = ve->port_mask;
    *forbidden_ports = ~ve->use_dynamic & ~ve->port_mask;
    *untagged_set    = ve->untagged_set;
    return unlock(0);
}

/**
 * Reads a vlan entry from the VLAN database.
 * @return 0 if entry found and read. -EINVAL if vid not valid.
 */
int rtu_fdb_read_vlan_entry(uint16_t vid,
                            uint8_t *fid,               // out
                            int *entry_type,            // out
                            uint32_t *port_mask,        // out
                            uint32_t *untagged_set,     // out
                            unsigned long *creation_t)  // out
{
    struct vlan_table_entry *ve;

    if ((vid >= NUM_VLANS) || reserved(vid))
        return -EINVAL;

    lock();
    ve = rtu_sw_find_vlan_entry(vid);
    if (!ve)
        return unlock(-EINVAL);
    *fid          = ve->fid;
    *entry_type   = ve->dynamic;
    *creation_t   = ve->creation_t;
    *untagged_set = ve->untagged_set;
    *port_mask    = ve->port_mask;
    return unlock(0);
}

/**
 * Reads next vlan entry from the VLAN database.
 * @param vid (IN) starting VLAN identifier. (OUT) vid of next vlan entry.
 * @return 0 if entry found and read. -EINVAL if vid not valid.
 */
int rtu_fdb_read_next_vlan_entry(uint16_t *vid,             // inout
                                 uint8_t *fid,              // out
                                 int *entry_type,           // out
                                 uint32_t *port_mask,       // out
                                 uint32_t *untagged_set,    // out
                                 unsigned long *creation_t) // out
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
    *port_mask    = ve->port_mask;
    return unlock(0);
}

int rtu_fdb_is_restricted_vlan_reg(int port_no)
{
    if (illegal(port_no))
        return -EINVAL;
    return is_set(restricted_vlan_reg, port_no);
}

int rtu_fdb_set_restricted_vlan_reg(int port_no)
{
    if (illegal(port_no))
        return -EINVAL;
    set(&restricted_vlan_reg, port_no);
    return 0;
}

int rtu_fdb_unset_restricted_vlan_reg(int port_no)
{
    if (illegal(port_no))
        return -EINVAL;
    unset(&restricted_vlan_reg, port_no);
    return 0;
}

//------------------------------------------------------------------------------
// Learning Constraints
//------------------------------------------------------------------------------

int rtu_fdb_create_lc(int sid, uint16_t vid, int lc_type)
{
    int ret;

    if ((sid < 0) || (sid > NUM_LC_SETS))
        return -EINVAL;

    if ((lc_type < LC_INDEPENDENT) || (lc_type > LC_SHARED))
        return -EINVAL;

    if (reserved(vid))
        return -EINVAL;

    lock();
    ret = rtu_sw_create_lc(sid, vid, lc_type);
    unlock(0);
    return ret;
}

int rtu_fdb_delete_lc(int sid, uint16_t vid)
{
    if ((sid < 0) || (sid > NUM_LC_SETS))
        return -EINVAL;

    if (reserved(vid))
        return -EINVAL;

    lock();
    rtu_sw_delete_lc(sid, vid);
    unlock(0);
    return 0;
}

int rtu_fdb_read_lc(uint16_t vid, uint32_t *lc_set)
{
    if (reserved(vid))
        return -EINVAL;

    lock();
    rtu_sw_read_lc(vid, lc_set);
    unlock(0);
    return 0;
}

int rtu_fdb_read_next_lc(uint16_t *vid, uint32_t *lc_set)
{
    int ret;

    if (reserved(*vid))
        return -EINVAL;

    lock();
    ret = rtu_sw_read_next_lc(vid, lc_set);
    unlock(0);
    return ret;
}

int rtu_fdb_read_lc_set_type(int sid, int *lc_type)
{
    if ((sid < 0) || (sid > NUM_LC_SETS))
        return -EINVAL;

    lock();
    rtu_sw_get_lc_set_type(sid, lc_type);
    unlock(0);
    return 0;
}

//------------------------------------------------------------------------------
// Default Learning Constraints
//------------------------------------------------------------------------------

void rtu_fdb_get_default_lc(int *sid, int *lc_type)
{
    lock();
    rtu_sw_get_default_lc(sid, lc_type);
    unlock(0);
}

int rtu_fdb_set_default_lc(int sid)
{
    int ret;

    if ((sid < 0) || (sid > NUM_LC_SETS))
        return -EINVAL;

    lock();
    ret = rtu_sw_set_default_lc(sid);
    unlock(0);
    return ret;
}

int rtu_fdb_set_default_lc_type(int lc_type)
{
    int ret;

    if ((lc_type < LC_INDEPENDENT) || (lc_type > LC_SHARED))
        return -EINVAL;

    lock();
    ret = rtu_sw_set_default_lc_type(lc_type);
    unlock(0);
    return ret;
}

//-----------------------------------------------------------------------------
// VID to FID allocation
//-----------------------------------------------------------------------------


/**
 * Deletes all static and dynamic mac entries from FDB, for the given VLAN.
 */
static void delete_mac_entries(uint16_t vid)
{
    int fid, xcast;
    struct filtering_entry_node *node, *next;
    struct filtering_entry *fe;
    struct static_filtering_entry *sfe, *next_sfe;
    struct vlan_table_entry *ve;

    ve = rtu_sw_find_vlan_entry(vid);
    if (ve) {
        fid = ve->fid;
        // Delete dynamic entries for FID assigned to VID
        for (node = fd[fid]; node; node = next) {
            next = node->next;
            if (rtu_sw_find_entry(node->mac, fid, &fe))
                rtu_fdb_delete_dynamic_entry(fe);
        }

        // Delete FDB info from static entries for VID
        for (xcast = 0; xcast < 2; xcast++) {
            for (sfe = sfd[xcast][vid]; sfe; sfe = next_sfe) {
                next_sfe = sfe->next;
                fe_delete_static_entry(vid, sfe);
            }
        }

        // Delete FDB info from static entries for wildcard VID
        // (if no other VID maps to the same FID).
        if (!rtu_sw_fid_shared(fid)) {
            for (xcast = 0; xcast < 2; xcast++)
                for (sfe = sfd[xcast][WILDCARD_VID]; sfe; sfe = sfe->next)
                    fe_delete_static_entry(vid, sfe);
        }

    }
}

/**
 * Inserts all static mac entries in FDB, for the given VLAN.
 */
static int insert_mac_entries(uint16_t vid)
{
    int err;
    int xcast;
    struct static_filtering_entry *sfe, *next_sfe;

    if (rtu_sw_find_vlan_entry(vid)) {
        // Insert static entries for VID in FDB with new VID to FID mapping
        for (xcast = 0; xcast < 2; xcast++) {
            for (sfe = sfd[xcast][vid]; sfe; sfe = next_sfe) {
                next_sfe = sfe->next;
                err = fe_insert_static_entry(vid, sfe);
                if (err)
                    return err;
            }
        }

        // Apply static entries for the Wildcard VID with new VID to FID mapping
        err = fe_insert_wildcard_static_entries(vid);
        if (err)
            return err;
    }
    return 0;
}


int rtu_fdb_set_fid(uint16_t vid, uint8_t fid)
{
    int err;

    if (reserved(vid))
        return -EINVAL;

    if (!fid)
        return -EINVAL; /* 0 is reserved */

    lock();
    rtu_sw_cache();

    // Delete FDB information associated to previous VID to FID mapping
    delete_mac_entries(vid);

    // Modify VID to FID allocation
    err = rtu_sw_set_fid(vid, fid);
    if (err)
        goto rollback;

    // Insert FDB information with the new VID to FID mapping
    err = insert_mac_entries(vid);
    if (err)
        goto rollback;

    rtu_sw_commit();
    unlock(0);
    return 0;

rollback:
    rtu_sw_uncache();
    rtu_sw_rollback();
    unlock(0);
    return err;
}

int rtu_fdb_delete_fid(uint16_t vid)
{
    int err;

    if (reserved(vid))
        return -EINVAL;

    lock();
    rtu_sw_cache();

    // Delete FDB information associated to previous VID to FID mapping (if any)
    delete_mac_entries(vid);

    // Delete VID to FID allocation
    rtu_sw_delete_fid(vid);

    // If VLAN is active, allocate VID to a new FID
    if (rtu_sw_find_vlan_entry(vid)) {
        err = rtu_sw_allocate_fid(vid);
        if (err)
            goto rollback;
    }

    // Insert FDB information with the new VID to FID mapping
    insert_mac_entries(vid);

    rtu_sw_commit();
    unlock(0);
    return err;

rollback:
    rtu_sw_uncache();
    rtu_sw_rollback();
    unlock(0);
    return err;
}

void rtu_fdb_read_fid(uint16_t vid, uint8_t *fid, int *fid_fixed)
{
    lock();
    rtu_sw_get_fid(vid, fid, fid_fixed);
    unlock(0);
}

int rtu_fdb_read_next_fid(uint16_t *vid, uint8_t *fid, int *fid_fixed)
{
    int ret;

    lock();
    ret = rtu_sw_get_next_fid(vid, fid, fid_fixed);
    unlock(0);
    return ret;
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
    struct filtering_entry_node *node, *next;
    struct filtering_entry *fe;
    unsigned long t;            // (secs)

    rtu_sw_update_aging_map();  // Work with latest access info
    rtu_sw_update_fd_age();     // Update filtering entries age

    lock();
    t = now() - aging_time;
    for (fid = 1; fid < NUM_FIDS; fid++) {  /* fid 0 is reserved */
        for (node = fd[fid]; node; node = next) {
            next = node->next;
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
