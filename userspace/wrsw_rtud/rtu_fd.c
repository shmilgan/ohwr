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
 *              Filtering database management related operations and filtering
 *              database mirror. Note there is a single Filtering Database
 *              object per Bridge (See 802.1Q - 12.7.1)
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

#include <hw/wrsw_rtu_wb.h>
#include <hw/trace.h>

#include <hal_client.h>

#include "rtu_fd.h"
#include "rtu_drv.h"
#include "rtu_hw.h"
#include "rtu_hash.h"

/**
 * \brief Mirror of ZBT SRAM memory MAC address table.
 * Main filtering table organized as hash table with 4-entry buckets.
 * Note both banks have the same content. Therefore SW only mirrors one bank.
 */
static struct filtering_entry htab[HTAB_ENTRIES][RTU_BUCKETS];

/**
 * \brief Mirror of CAM lookup table.
 * For RTU entries with more than 4 matches
 */
static struct filtering_entry hcam[CAM_ENTRIES];

/**
 * \brief Mirror of Aging RAM.
 */
static uint32_t agr_htab[RTU_ARAM_MAIN_WORDS];
static uint32_t agr_hcam;

/**
 * \brief Max time that a dynamic MAC entry can remain
 * in the MAC table after being used. [seconds]
 */
static unsigned long aging_time;

/**
 * Mirror of HW VLAN table
 */
static struct vlan_table_entry vlan_tab[NUM_VLANS];

/**
 * Keeps track of number of dynamic entries per filtering database
 */
uint16_t num_dynamic_entries[NUM_FIDS];

/**
 * Keeps track of learned entry discards (when FDB is full)
 */
uint64_t num_learned_entry_discards[NUM_FIDS];

/**
 * Keeps track of vlan deletes, regardless of the reason
 */
uint64_t num_vlan_deletes;

/**
 * \brief Filtering Database (SW oriented model)
 * Used to support lexicographically ordered lookups in the filtering database.
 * NULL indicates the FID is not in use.
 */
struct filtering_entry *fdb[NUM_FIDS];

/**
 * \brief Static Filtering Database.
 * Unicast and multicast entries stored in separated linked lists
 * to simplify management algorithms (identified by first array dimension:
 * 0 = Unicast, 1 = Multicast.)
 * It also gives support to lexicographically ordered lookups in static FDB.
 * Note: NULL indicates the VID is not in use.
 */
struct static_filtering_entry *static_fdb[2][NUM_VLANS];

/**
 * \brief Mutex used to synchronise concurrent access to the filtering database.
 */
static pthread_mutex_t fd_mutex;

static inline
uint16_t zbt_addr(uint16_t hash, int bucket)
{
    return (( 0x07FF & hash ) << 5 ) | ((0x0003 & bucket) << 3);
}

static inline
uint16_t cam_addr(int bucket)
{
    return ((0x001F & bucket) << 3);
}

static inline
int cam_bucket(uint16_t cam_addr)
{
    return ((cam_addr >> 3) & 0x001F);
}

static inline
int matched(uint32_t word, int offset)
{
    return (word >> offset) & 0x00000001;
}

static inline
void _link(struct filtering_entry *fe, int cam_bucket)
{
    fe->go_to_cam = 1;
    fe->cam_addr  = cam_addr(cam_bucket);
}

static inline
void _unlink(struct filtering_entry *fe)
{
    fe->go_to_cam = 0;
    fe->cam_addr  = 0;
}

static inline
int hcam_contains(struct filtering_entry *fe)
{
    return (fe >= hcam) && (fe < (hcam + CAM_ENTRIES));
}

static inline
int empty(struct filtering_entry *fe)
{
    return !fe->valid;
}

static inline
int hash(struct filtering_entry *fe) // only valid for HTAB entries!
{
    return (fe - &htab[0][0]) / RTU_BUCKETS;
}

static inline
int htab_bucket(struct filtering_entry *fe)
{
    return (fe - &htab[0][0]) % RTU_BUCKETS;
}

static inline
int hcam_bucket(struct filtering_entry *fe)
{
    return fe - hcam;
}

static inline
int reserved(int vid)
{
    return (vid == 0) || (vid == WILDCARD_VID);
}

//------------------------------------------------------------------------------
// Aging
//------------------------------------------------------------------------------

/**
 * \brief Clean HCAM aging register and HTAB aging bitmap.
 */
static void clean_aging_map(void)
{
    int i;

    agr_hcam = 0x00000000;
    rtu_clean_agr_hcam();
    for(i = 0; i < RTU_ARAM_MAIN_WORDS; i++)
        agr_htab[i] = 0x00000000;

    rtu_clean_agr_htab();
}

/**
 * \brief Update aging map cache with contents read from aging registers at HW.
 */
static void update_aging_map(void)
{
    int i;

    agr_hcam = rtu_read_agr_hcam();
    for(i = 0; i < RTU_ARAM_MAIN_WORDS; i++)
        agr_htab[i] = rtu_read_agr_htab(i);
}

/**
 * \brief Updates the age of filtering entries accessed in the last period.
 */
static void update_fdb_age(void)
{
    int i;                              // Aging Bitmap word loop index
    int j;                              // Word bits loop index
    uint32_t agr_word;                  // Aux var for manipulating aging RAM
    uint16_t hash;                      // HTAB entry hash (index)
    int bucket;                         // HTAB entry bucket
    int bit_cnt;                        // Absolute bit counter
    unsigned long t;                    // Time since epoch (secs)

    // Update 'last access time' for accessed entries
    t = now();
    // HTAB
    for(i = 0; i < RTU_ARAM_MAIN_WORDS; i++) {
        agr_word = agr_htab[i];
        if(agr_word != 0x00000000) {
            for(j = 0; j < 32; j++){
                if(matched(agr_word, j)) {
                    // ((word_pos x 32) + bit_pos)
                    bit_cnt = ((i & 0x00FF) << 5) | (j & 0x001F);
                    hash    = bit_cnt >> 2;             // 4 buckets per hash
                    bucket  = bit_cnt & 0x03;           // last 2 bits

                    htab[hash][bucket].last_access_t = t;
                    TRACE_DBG(
                        TRACE_INFO,
                        "updated htab entry age: mac = %s, hash = %d, bucket = %d\n, t = %d",
                        mac_to_string(htab[hash][bucket].mac),
                        hash,
                        bucket,
                        t
                    );
                }
            }
        }
    }
    // HCAM
    agr_word = agr_hcam;
    for(j = 0; j < 32; j++){
        if(matched(agr_word, j)) {
            hcam[j].last_access_t = t;
            TRACE_DBG(
                TRACE_INFO,
                "updated hcam entry age: mac = %s, bucket = %d\n",
                mac_to_string(hcam[j].mac),
                j
            );
        }
    }
}

//------------------------------------------------------------------------------
// FDB - Double Linked List (lexicografically ordered)
//------------------------------------------------------------------------------

/**
 * \brief Links a filtering entry to in the fdb double link list in
 * lexicographic order. Used to support management operations.
 */
static void link_fdb_node(struct filtering_entry *fe)
{
    int fid;
    struct filtering_entry *node;

    fid = fe->fid;
    node = fdb[fid];
    if (!node) {                                    // first node for this FID
        fdb[fid]         = fe;
        fe->prev         = NULL;
        fe->next         = NULL;
    } else if (mac_cmp(fe->mac, node->mac) < 0) {   // entry should be first
        node->prev       = fe;
        fe->prev         = NULL;
        fe->next         = node;
        fdb[fid]         = fe;
    } else {                                        // find place to insert node
        for (;
             node->next && (mac_cmp(fe->mac, node->next->mac) > 0);
             node = node->next);

        fe->next         = node->next;
        fe->prev         = node;
        node->next->prev = fe;
        node->next       = fe;
    }
}

/**
 * \brief Unlinks a filtering entry from the the fdb double linked list.
 */
static void unlink_fdb_node(struct filtering_entry *fe)
{
    if(fe->prev)
        fe->prev->next = fe->next;
    else                            // first node in fdb double linked list
        fdb[fe->fid] = fe->next;

    if(fe->next)
        fe->next->prev = fe->prev;
}

/**
 * \brief Fixes the fdb double linked list when a filtering entry gets shifted
 * (as a consequence of HTAB or HCAM deletes).
 */
static void fix_fdb_node(struct filtering_entry *fe)
{
    if(fe->prev)
        fe->prev->next = fe;
    else
        fdb[fe->fid] = fe;

    if(fe->next)
        fe->next->prev = fe;
}

/**
 * \brief Finds the filtering entry with indicated FID and MAC.
 * @return pointer to found entry. NULL if no entry found.
 */
static struct filtering_entry *find_fdb_node(int fid, uint8_t mac[ETH_ALEN])
{
    struct filtering_entry *fe;

    for (fe = fdb[fid]; fe && !mac_equal(mac, fe->mac); fe = fe->next);
    return fe;
}

/**
 * \brief Finds the filtering entry following the one with indicated FID and
 * MAC, in lexicographic order.
 * @return pointer to entry found. NULL if no entry follows the one indicated.
 */
static struct filtering_entry *find_next_fdb_node(int fid,uint8_t mac[ETH_ALEN])
{
    struct filtering_entry *fe;

    fe = find_fdb_node(fid, mac);
    if (fe && fe->next)
        fe = fe->next;
    else                                // check entries for following FIDs
        for (fe = NULL, fid = fid + 1; (fid < NUM_FIDS) && !fe; fid++)
            fe = fdb[fid];

    return fe;
}

//------------------------------------------------------------------------------
// Static FDB - Double Linked List (lexicografically ordered)
//------------------------------------------------------------------------------

/**
 * \brief Links a static filtering entry to in the static fdb double link list
 * in lexicographic order. Used to support management operations.
 */
static void link_static_fdb_node(struct static_filtering_entry *sfe)
{
    int vid, xcast;
    struct static_filtering_entry *node;

    vid     = sfe->vid;
    xcast   = mac_multicast(sfe->mac);
    node    = static_fdb[xcast][vid];
    if (!node) {                                    // first node for this VID
        static_fdb[xcast][vid]  = sfe;
        sfe->prev               = NULL;
        sfe->next               = NULL;
    } else if (mac_cmp(sfe->mac, node->mac) < 0) {  // entry should be first
        node->prev              = sfe;
        sfe->prev               = NULL;
        sfe->next               = node;
        static_fdb[xcast][vid]  = sfe;
    } else {                                        // find place to insert node
        for (;
             node->next && (mac_cmp(sfe->mac, node->next->mac) > 0);
             node = node->next);

        sfe->next               = node->next;
        sfe->prev               = node;
        node->next->prev        = sfe;
        node->next              = sfe;
    }
}

/**
 * \brief Unlinks a static filtering entry from the static fdb double linked
 * list.
 */
static void unlink_static_fdb_node(struct static_filtering_entry *sfe)
{
    int xcast;

    xcast = mac_multicast(sfe->mac);
    if(sfe->prev)
        sfe->prev->next = sfe->next;
    else                            // first node in fdb double linked list
        static_fdb[xcast][sfe->vid] = sfe->next;

    if(sfe->next)
        sfe->next->prev = sfe->prev;
}

/**
 * \brief Finds the static filtering entry with indicated VID and MAC.
 * @return pointer to found entry. NULL if no entry found.
 */
static struct static_filtering_entry *find_static_fdb_node(
        int vid,
        uint8_t mac[ETH_ALEN],
        int xcast)
{
    struct static_filtering_entry *sfe;

    for (sfe = static_fdb[xcast][vid];
         sfe && !mac_equal(mac, sfe->mac);
         sfe = sfe->next);
    return sfe;
}

/**
 * \brief Finds the static filtering entry following the one with indicated VID
 * and MAC, in lexicographic order.
 * @return pointer to entry found. NULL if no entry follows the one indicated.
 */
static struct static_filtering_entry *find_next_static_fdb_node(
        int vid,
        uint8_t mac[ETH_ALEN])
{
    int xcast;                              // unicast or multicast
    struct static_filtering_entry *sfe;

    xcast = mac_multicast(mac);
    sfe = find_static_fdb_node(vid, mac, xcast);
    if (sfe && sfe->next)
        sfe = sfe->next;
    else                                    // check entries for following VIDs
        for (sfe = NULL, vid = vid + 1; (vid < NUM_VLANS) && !sfe; vid++)
            sfe = static_fdb[xcast][vid];

    return sfe;
}

//------------------------------------------------------------------------------
// Static FDB
//------------------------------------------------------------------------------

/**
 * \brief Searches for an entry with given mac and vid in static FDB.
 * The sfe pointer will point to the found entry. If no entry is found, sfe
 * will be NULL.
 * @return 1 if found. 0 otherwise.
 */
static int find_static_entry(
        uint8_t mac[ETH_ALEN],
        uint8_t vid,
        struct static_filtering_entry **sfe)
{
    *sfe = find_static_fdb_node(vid, mac, mac_multicast(mac));
    return *sfe ? 1:0;
}

/**
 * \brief Updates static FDB entry.
 * @return 1 if entry was updated. 0 otherwise.
 */
static int update_static_entry(
            struct static_filtering_entry *sfe,
            enum filtering_control port_map[NUM_PORTS],
            int active)
{
    int ret = 0;
    if (sfe->type == Permanent)
        return 0;                           // Entry can NOT be modified
    if (sfe->active != active) {
        sfe->active = active;
        ret = 1;
    }
    if (memcmp(sfe->port_map, port_map, NUM_PORTS)) {
        memcpy(sfe->port_map, port_map, NUM_PORTS);
        ret = 1;
    }
    return ret;
}

/**
 * \brief Inserts an entry in the static FDB linked list.
 * @return 1 if entry was inserted. 0 otherwise (i.e. no more memory available)
 */
static int insert_static_entry(
            struct static_filtering_entry **sfe,
            uint8_t  mac[ETH_ALEN],
            uint16_t vid,
            enum filtering_control port_map[NUM_PORTS],
            enum storage_type type,
            int active)
{
    *sfe = (struct static_filtering_entry*)
            malloc(sizeof(struct static_filtering_entry));

    if (!*sfe)
        return 0;

    mac_copy((*sfe)->mac, mac);
    memcpy((*sfe)->port_map, port_map, NUM_PORTS);
    (*sfe)->vid            = vid;
    (*sfe)->type           = type;
    (*sfe)->active         = active;
    (*sfe)->next_sib       = NULL;

    link_static_fdb_node(*sfe);         // lexicographically ordered
    return 1;
}

/**
 * \brief Deletes an entry from the static FDB linked list.
 */
static void delete_static_entry(struct static_filtering_entry *sfe)
{
    unlink_static_fdb_node(sfe);
    free(sfe);
}


/**
 * Get tail of list of static filtering entries associated to an FDB entry.
 */
static struct static_filtering_entry *tail(struct static_filtering_entry *sfe)
{
    struct static_filtering_entry *ptr;

    for (ptr = sfe; ptr && ptr->next_sib; ptr = ptr->next_sib);
    return ptr;
}

//------------------------------------------------------------------------------
// FDB - HCAM
//------------------------------------------------------------------------------

/**
 * \brief Find the most appropriate empty bucket to insert new hash collision
 * list. The algorithm first finds the fragment which contains the max number of
 * consecutive empty positions. Then divides this fragment into two parts: first
 * block is still available for possible increment of any existing list; The
 * second block will be available for the new list.
 * The algorithm keeps a fair and uniform distribution of fragments space.
 * @return bucket index or -1 if the HCAM table is full.
 */
static int find_empty_bucket_in_hcam(void)
{
    int bucket = 0; // bucket loop index
    int res    = 0; // result bucket
    int empty  = 0; // consecutive empty buckets
    int max    = 0; // max consecutive empty buckets

    // First obtain position with max consecutive empty space
    for(; bucket < CAM_ENTRIES; bucket++) {
        if (hcam[bucket].valid) {
            if (empty > max) {
                max = empty;
                res = bucket - empty;
            }
            empty = 0;
        } else {
            empty++;
        }
    }
    // Update max consecutive empty buckets if necessary
    if (empty > max) {
        max = empty;
        res = bucket - empty;
    }

    if(max == 0)                   // bank is full
        return -1;
    else if(max == CAM_ENTRIES)    // bank is empty
        return 0;
    else    // Divide max space in two blocks and take address of second block
        return res + max/2;
}

/**
 * \brief Add a new entry at the end of a list associated to a given hash.
 * @param ent pointer to last entry of current list.
 * @return pointer to entry added
 */
static struct filtering_entry *add_hcam_entry(struct filtering_entry *fe)
{
    struct filtering_entry *next;
    int bucket;

    if (hcam_contains(fe)) {
        // Add entry at end of list
        if (fe == hcam + LAST_CAM_ENTRY)
            return NULL;        // Last hcam entry. no more space
        next = fe + 1;
        if(!empty(next))
            return NULL;        // Following entry already stores another list
        fe->end_of_bucket  = 0;
        rtu_hw_write_hcam_entry(cam_addr(hcam_bucket(fe)), fe);
    } else {                    // ent is last HTAB entry. Add first HCAM entry
        bucket = find_empty_bucket_in_hcam();
        if (bucket < 0)
            return NULL;        // FDB full
        next = hcam + bucket;
        // Update ent to point to hcam entry
        _link(fe, bucket);
        rtu_hw_write_htab_entry(zbt_addr(hash(fe), LAST_RTU_BUCKET), fe);
    }
    next->end_of_bucket = 1;
    return next;
}

/**
 * \brief Deletes HCAM entry.
 * Updates HTAB last entry if neccessary.
 * @param bucket CAM entry address
 */
static void delete_hcam_entry(int bucket)
{
    struct filtering_entry *fe, *prev;
    uint16_t hash = -1;     // informs unlinking required

    fe = &hcam[bucket];
    // Unlink node from lexicographically ordered list
    unlink_fdb_node(fe);
    if (fe->end_of_bucket) {
        if (bucket == 0) {
            // this is first HCAM entry (so no more in this list). Get hash to
            // unlink HTAB
            hash = rtu_hash(fe->mac, fe->fid);
        } else {
            prev = fe-1;
            if (prev->valid && !prev->end_of_bucket) {
                // if previous is part of this list, mark it as the new end
                prev->end_of_bucket = 1;
                rtu_hw_write_hcam_entry(cam_addr(bucket-1), prev);
            } else {
                // fe was the only entry on this list. Get hash to unlink HTAB
                hash = rtu_hash(fe->mac, fe->fid);
            }
        }
    } else {
        // shift entries
        for (; !fe->end_of_bucket; fe++, bucket++) {
            rtu_fe_copy(fe, fe + 1);
            rtu_hw_write_hcam_entry(cam_addr(bucket), fe);
            // Fix lexicographically ordered list
            fix_fdb_node(fe);
        }
    }
    rtu_fe_clean(fe);
    rtu_hw_clean_hcam_entry(cam_addr(bucket));
    // Check if we need to unlink HTAB
    if (hash >= 0) {
        fe = &htab[hash][LAST_RTU_BUCKET];
        _unlink(fe);
        rtu_hw_write_htab_entry(zbt_addr(hash, LAST_RTU_BUCKET), fe);
    }
}


//------------------------------------------------------------------------------
// FDB - HTAB
//------------------------------------------------------------------------------

/**
 * \brief Deletes HTAB entry by shifting HTAB list.
 * If HCAM is used, it also copies first HCAM entry to last HTAB bucket.
 * @param hash hashcode for entry to remove.
 * @param bucket HTAB bucket for entry to remove
 */
static void delete_htab_entry(uint16_t hash, int bucket)
{
    struct filtering_entry *fe, *cam_fe;

    // Shift entries
    fe = &htab[hash][bucket];
    // Unlink node from lexicographically ordered list
    unlink_fdb_node(fe);
    for(; (bucket < LAST_RTU_BUCKET) && (fe+1)->valid; bucket++, fe++){
        rtu_fe_copy(fe, fe + 1);
        rtu_hw_write_htab_entry(zbt_addr(hash, bucket), fe);
        // Fix lexicographically ordered list
        fix_fdb_node(fe);
    }
    // If HTAB was full, check if HCAM was also used.
    if(bucket == LAST_RTU_BUCKET){
        if(fe->go_to_cam){
            // go_to_cam was copied to previous entry while shifting. clean it.
            _unlink(fe-1);
            // copy first cam entry into last HTAB entry
            bucket  = cam_bucket(fe->cam_addr);
            cam_fe = &hcam[bucket];
            rtu_fe_copy(fe, cam_fe);
            // adjust pointer to HCAM
            if(cam_fe->end_of_bucket){
                fe->end_of_bucket = 0;
                _unlink(fe);
            } else {
                _link(fe, bucket + 1);
            }
            rtu_hw_write_htab_entry(zbt_addr(hash, LAST_RTU_BUCKET), fe);
            // Fix lexicographically ordered list
            fix_fdb_node(fe);
            // clean HCAM entry
            rtu_fe_clean(cam_fe);
            rtu_hw_clean_hcam_entry(cam_addr(bucket));
        } else {
            // clean last HTAB entry
            rtu_fe_clean(fe);
            rtu_hw_clean_htab_entry(zbt_addr(hash, LAST_RTU_BUCKET));
        }
    }
}

//------------------------------------------------------------------------------
// FDB - HTAB + HCAM
//------------------------------------------------------------------------------

/**
 * \brief Writes entry to RTU HW (HTAB or HCAM).
 */
static void write_entry(struct filtering_entry *fe)
{
    if (hcam_contains(fe))
        rtu_hw_write_hcam_entry(cam_addr(hcam_bucket(fe)), fe);
    else
        rtu_hw_write_htab_entry(zbt_addr(hash(fe), htab_bucket(fe)), fe);
}

/**
 * \brief Deletes entry from FDB (HTAB or HCAM).
 */
static void delete_entry(struct filtering_entry *fe)
{
    if (hcam_contains(fe))
        delete_hcam_entry(cam_addr(hcam_bucket(fe)));
    else
        delete_htab_entry(hash(fe), htab_bucket(fe));
}

//------------------------------------------------------------------------------
// FDB - VLAN TABLE
//------------------------------------------------------------------------------

/**
 * \brief VLAN database initialisation. VLANs are initially marked as disabled.
 */
static void clean_vd(void)
{
    int i;

    rtu_clean_vlan();
    for(i = 0; i < NUM_VLANS; i++)
        vlan_tab[i].drop = 1;

    // Entry with VID 1 reserved for untagged packets.
    vlan_tab[DEFAULT_VID].port_mask       = 0xffffffff;
    vlan_tab[DEFAULT_VID].drop            = 0;
    vlan_tab[DEFAULT_VID].fid             = 0;
    vlan_tab[DEFAULT_VID].has_prio        = 0;
    vlan_tab[DEFAULT_VID].prio_override   = 0;
    vlan_tab[DEFAULT_VID].prio            = 0;

    rtu_write_vlan_entry(DEFAULT_VID, &vlan_tab[DEFAULT_VID]);
}

/**
 * \brief Obtains de port_mask and use_dynamic masks from the VLAN member_set.
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
 * \brief Fills up the member_set combining info from the port_map and
 * use_dynamic masks.
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

//------------------------------------------------------------------------------
// FDB
//------------------------------------------------------------------------------

/**
 * \brief Filtering database initialisation.
 */
static void clean_fd(void)
{
    memset(&htab, 0, sizeof(htab));
    memset(&hcam, 0, sizeof(hcam));
    rtu_hw_clean_fdb();
}

/**
 * \brief Searches for an entry with given mac and fid in FDB (HTAB and HCAM).
 * The fe pointer will point to the found entry. If no entry found, ent will
 * point to next empty entry in HTAB if it exists. Otherwise, ent will point
 * to the last entry for the list.
 * @return 1 if found. 0 otherwise.
 */
static int find_entry(
        uint8_t mac[ETH_ALEN],
        uint8_t fid,
        struct filtering_entry **fe)
{
    int bucket = 0;
    uint16_t hash;

    // Check HTAB
    hash  = rtu_hash(mac, fid);
    *fe = &htab[hash][0];
    for(; bucket < RTU_BUCKETS; bucket++, (*fe)++) {
        if (empty(*fe))
            return 0;
        if (mac_equal((*fe)->mac, mac) && ((*fe)->fid == fid))
            return 1;
        if (bucket == LAST_RTU_BUCKET)
            break;
    }
    // if HCAM is used, check it also.
    if((*fe)->go_to_cam){
        bucket = cam_bucket((*fe)->cam_addr);
        for(; bucket < CAM_ENTRIES; bucket++) {
            *fe = &hcam[bucket];
            if (mac_equal((*fe)->mac, mac) && ((*fe)->fid == fid))
                return 1;
            if ((*fe)->end_of_bucket)
                return 0;
        }
    }
    return 0; // Not found in HTAB. HCAM not used yet.
}

/**
 * \brief Inserts an entry in the FDB (either at HTAB or HCAM). The fe pointer
 * will point to the inserted entry.
 * @param dynamic indicates the type of info. 1=dynamic.
 * @return 1 if entry was inserted. 0 otherwise (i.e. FDB is full)
 */
static int insert_entry(
            struct filtering_entry **fe,
            uint8_t  mac[ETH_ALEN],
            uint16_t vid,
            uint32_t port_map,
            uint32_t use_dynamic,
            int dynamic)
{
    int fid;

    if(!empty(*fe))     // HTAB full
        *fe = add_hcam_entry(*fe);

    if (!*fe)
        return 0;       // FDB full

    mac_copy((*fe)->mac, mac);
    fid                  = vlan_tab[vid].fid;
    (*fe)->fid           = fid;
    (*fe)->port_mask_src = vlan_tab[vid].port_mask;
    // If entry is static, apply default unicast/group filtering behaviour to
    // the dynamic part (if exists) (802.1Q 8.8.6)
    // (note when inserting new static entry no other dynamic info is available)
    (*fe)->port_mask_dst = dynamic ? port_map:(port_map | use_dynamic);
    (*fe)->last_access_t = now();
    (*fe)->dynamic       = dynamic;
    (*fe)->use_dynamic   = use_dynamic;
    (*fe)->static_fdb    = NULL;
    (*fe)->valid         = 1;
    num_dynamic_entries[fid]++;
    link_fdb_node(*fe); // lexicographically ordered list
    return 1;
}

/**
 * \brief Updates the entry src and dst port masks.
 * @return 1 if any of the masks was updated. 0 otherwise
 */
static int update_masks(
            struct filtering_entry *fe,
            uint32_t mask_src,
            uint32_t mask_dst)
{
    int update = 0;

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
 * \brief Updates FDB entry with learnt information. If entry is pure dynamic,
 * it just overrides previous info. If entry contains static info, it checks
 * whether dynamic info can be used and updates entry only if permitted.
 * @param port_map entry port map. Method combines available static and dynamic
 * info to come to a final forward vector.
 * @param use_dynamic dynamic mask. Ignored if info is dynamic.
 * @param dynamic indicates the type of info. 1=dynamic.
 * @return 1 if entry masks were updated. 0 otherwise.
 */
static int update_entry(
            struct filtering_entry *fe,
            uint16_t vid,
            uint32_t port_map,
            uint32_t use_dynamic,
            int dynamic)
{
    int update = 0;
    uint32_t mask_src, mask_dst;

    if (dynamic) {                          // Dynamic info provided
        if (fe->static_fdb && !fe->use_dynamic)
            return 0;                       // ... but is not permitted
        mask_src = fe->port_mask_src | vlan_tab[vid].port_mask;
        // Override dynamic part of port map
        mask_dst = fe->use_dynamic ?
               ((fe->port_mask_dst & !fe->use_dynamic)|(port_map & fe->use_dynamic)):
               port_map;
        update = update_masks(fe, mask_src, mask_dst);
        if (!fe->dynamic) {
            fe->dynamic = DYNAMIC;          // now entry contains dyn info
            num_dynamic_entries[fe->fid]++;
        }
    } else {                                // Static info
        mask_src = fe->port_mask_src | vlan_tab[vid].port_mask;
        mask_dst = port_map;                // Static part of forward vector
        if (use_dynamic) {                  // If dynamic info can be used...
            if (fe->dynamic)                // ...and entry contains dyn info
                // Add dynamic part of forward vector.
                mask_dst |= ((fe->port_mask_dst & fe->use_dynamic) & use_dynamic);
            else
                // Apply default unicast/group behavior
                mask_dst |= use_dynamic;
        }
        update = update_masks(fe, mask_src, mask_dst);
        fe->use_dynamic = use_dynamic;
    }
    return update;
}

static void delete_dynamic_entry(struct filtering_entry *fe)
{
    num_dynamic_entries[fe->fid]--;
    if (fe->static_fdb) {                   // Entry contains static info.
        // Reset dynamic part (set all dynamic bits to 1 = Forward)
        fe->port_mask_dst |= fe->use_dynamic;
        fe->dynamic  = STATIC;
        write_entry(fe);
    } else {
        delete_entry(fe);
    }
}

//------------------------------------------------------------------------------
// FDB - Static Info
//------------------------------------------------------------------------------

/**
 * \brief Combines port_map information from a list of static entries to get
 * a forward decision for each port.
 * @param sfdb pointer to the list of static entries that must be combined to
 * get the forward vector.
 */
static void calculate_forward_vector(
            struct static_filtering_entry *sfdb,
            uint32_t *port_map,
            uint32_t *use_dynamic)
{
    int i;

    // If any entry specifies Forward for this port, then result is Forward
    // else if any entry specifies Filter for this port, then result is Filter
    // else use Dynamic info
    *use_dynamic = 0xFFFFFFFF;
    *port_map    = 0x00000000;
    for (; sfdb; sfdb = sfdb->next_sib) {
        if (!sfdb->active)                  // Only active entries are used to
            continue;                       // compute the forward vector
        for (i = 0; i < NUM_PORTS; i++) {
            if (sfdb->port_map[i] == Forward) {
                *port_map    |=  (0x01 << i);
                *use_dynamic &= !(0x01 << i);
            } else if (sfdb->port_map[i] == Filter) {
                *use_dynamic &= !(0x01 << i);
            }
        }
    }
    // Note that algorithm also processes wildcard-VID entries (if any exists).
}

/**
 * \brief Inserts a static entry in the filtering database.
 * @return 1 if entry was inserted. 0 otherwise (i.e. FDB is full)
 */
static int insert_static_entry_in_fdb(
            uint16_t vid,
            struct static_filtering_entry *sfe,
            int was_created)
{
    int fid;
    struct filtering_entry *fe;
    uint32_t _port_map, _use_dynamic;       // final forward vector

    fid = vlan_tab[vid].fid;
    if (find_entry(sfe->mac, fid, &fe)) {
        // Register static entry in static fdb list associated to entry.
        // If the static entry was only updated, it must have been registered
        // before, so we only need to register new static entries that were
        // actually created.
        if (was_created) {
            if(fe->static_fdb) {
                if (sfe->vid == WILDCARD_VID) {
                    // wildcard entry added to tail
                    tail(fe->static_fdb)->next_sib = sfe;
                } else {
                    // non-wildcard entry added to head
                    sfe->next_sib  = fe->static_fdb;
                    fe->static_fdb = sfe;
                }
            } else {
                fe->static_fdb = sfe;
            }
        }
        // Combine all static info.
        calculate_forward_vector(fe->static_fdb, &_port_map, &_use_dynamic);
        // Combine static and dynamic info and update
        if (!update_entry(fe, vid, _port_map, _use_dynamic, STATIC))
            return 0;
    } else {
        // insert pure static entry
        calculate_forward_vector(fe->static_fdb, &_port_map, &_use_dynamic);
        if (!insert_entry(&fe, sfe->mac, vid, _port_map, _use_dynamic, STATIC))
            return -ENOMEM;
        fe->static_fdb = sfe;               // Register static entry in the list
    }
    write_entry(fe);
    return 1;
}


/**
 * \brief Removes a static entry from the filtering database.
 * @return 1 if entry was modified. 0 otherwise
 */
static int delete_static_entry_from_fdb(
            uint16_t vid,
            struct static_filtering_entry *sfe)
{
    int fid;
    struct filtering_entry *fe;
    struct static_filtering_entry *prev;
    uint32_t _port_map, _use_dynamic;       // final forward vector

    fid = vlan_tab[vid].fid;
    if (find_entry(sfe->mac, fid, &fe)) {
        // Unregister static entry from static fdb list associated to entry.
        // Note: this is also valid to unregister static entry for wildcard VID.
        if (fe->static_fdb == sfe)
            fe->static_fdb = sfe->next_sib;
        else
            for (prev = fe->static_fdb; prev; prev = prev->next_sib)
                if (prev->next_sib == sfe)
                    prev->next_sib = sfe->next_sib;

        if (!fe->static_fdb && !fe->dynamic) {   // no info remains!
            delete_entry(fe);
        } else {
            // Combine all remaining static info (if any).
            calculate_forward_vector(fe->static_fdb, &_port_map, &_use_dynamic);
            // Combine static and dynamic info and update
            if (!update_entry(fe, vid, _port_map, _use_dynamic, STATIC))
                return 0;
            write_entry(fe);
        }
        return 1;
    }
    return 0;
}

/**
 * \brief Creates static entries for reserved MAC addresses in the filtering
 * database. Should be called on FDB (re)initialisation. These entries are
 * permanent and can not be modified.
 * @return error code
 */
static int create_permanent_entries()
{
    uint8_t bcast_mac[]         = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    uint8_t slow_proto_mac[]    = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x01};
    hexp_port_list_t plist;
    hexp_port_state_t pstate;
    int i, err;
    enum filtering_control port_map[NUM_PORTS];

    memset(port_map, 0x00, NUM_PORTS);
    port_map[NIC_PORT] = Forward;

    // VLAN-aware Bridge reserved addresses (802.1Q-2005 Table 8.1)
    TRACE(TRACE_INFO,"adding static routes for slow protocols...");
    for(i = 0; i < NUM_RESERVED_ADDR; i++) {
        slow_proto_mac[5] = i;
        err = rtu_fdb_create_static_entry(slow_proto_mac, WILDCARD_VID,
            port_map, Permanent, ACTIVE);
        if(err)
            return err;
    }

    // packets addressed to WR card interfaces are forwarded to NIC virtual port
    halexp_query_ports(&plist);
    for(i = 0; i < plist.num_ports; i++) {
        halexp_get_port_state(&pstate, plist.port_names[i]);
        TRACE(
            TRACE_INFO,
            "adding static route for port %s index %d [mac %s]",
            plist.port_names[i],
            pstate.hw_index,
            mac_to_string(pstate.hw_addr)
        );
		err = rtu_fdb_create_static_entry(pstate.hw_addr, WILDCARD_VID,
		    port_map, Permanent, ACTIVE);
        if(err)
            return err;
    }

    // Broadcast MAC
    TRACE(TRACE_INFO,"adding static route for broadcast MAC...");
    for(i = 0; i < NUM_PORTS; i++)
        port_map[i] = Forward;
    err = rtu_fdb_create_static_entry(bcast_mac, WILDCARD_VID,
        port_map, Permanent, ACTIVE);
    if(err)
        return err;

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
    int i, n;
    for(n = 0, i = 0; i < NUM_VLANS; i++)
        if (!vlan_tab[i].drop)
            n++;
    return n;
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
 * \brief Find the next filtering database identifier in use, starting from
 * the indicated fid.
 * Used to support management operations that traverse the filtering database.
 * @return next fid in use. NUM_FIDS if no next fid.
 */
uint8_t  rtu_fdb_get_next_fid(uint8_t fid)
{
    int i;

    for (i = fid; i < NUM_FIDS; i++)
        if(fdb[i])
            return i;
}


/**
 * \brief Set the polynomial used for hash calculation.
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
    rtu_write_hash_poly(poly);
    rtu_hash_set_poly(poly);
    pthread_mutex_unlock(&fd_mutex);
}

/**
 * \brief Gets the aging time for dynamic filtering entries.
 * @param fid filtering database identifier (ignored currently)
 * @return aging time value [seconds]
 */
unsigned long rtu_fdb_get_aging_time(uint8_t fid)
{
    return aging_time;
}


/**
 * \brief Sets the aging time for dynamic filtering entries.
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
 * \brief Initializes the RTU filtering database.
 * @param poly hash polinomial.
 * @param aging aging time
 */
int rtu_fdb_init(uint16_t poly, unsigned long aging)
{
    int err, i;

    TRACE_DBG(TRACE_INFO, "clean filtering database.");
    clean_fd();
    TRACE_DBG(TRACE_INFO, "clean vlan database.");
    clean_vd();
    TRACE_DBG(TRACE_INFO, "clean aging map.");
    clean_aging_map();
    TRACE_DBG(TRACE_INFO, "set aging time [%d].", aging);
    aging_time = aging;

    err = pthread_mutex_init(&fd_mutex, NULL);
    if (err)
        return err;

    TRACE_DBG(TRACE_INFO, "set hash poly [%d].", poly);
    rtu_fdb_set_hash_poly(poly);

    // create static filtering entries for reserved MAC addresses
    err = create_permanent_entries();
    if(err)
        return err;

    for (i = 0; i < NUM_FIDS; i++)
        fdb[i] = NULL;

    for (i = 0; i < NUM_VLANS; i++)
        static_fdb[0][i] = static_fdb[1][i] = NULL;

    return 0;
}

/**
 * \brief Creates or updates a dynamic filtering entry in filtering database.
 * @param mac MAC address specification
 * @param vid VLAN identifier
 * @param port_map a port map specification with a control element for each
 * outbound port to specify filtering for that MAC address specification and VID
 * @return 0 if entry was created or updated. -ENOMEM if no space is available.
 */
int rtu_fdb_create_dynamic_entry(
            uint8_t  mac[ETH_ALEN],
            uint16_t vid,
            uint32_t port_map)
{
    int ret = 0, fid;
    struct filtering_entry *fe;

    // if VLAN is not registered ignore request
    if (vlan_tab[vid].drop)
        return 0;
    // if member set for VID does not contain at least one port, ignore request
    if (vlan_tab[vid].port_mask == 0)
        return 0;

    pthread_mutex_lock(&fd_mutex);
    fid = vlan_tab[vid].fid;
    port_map &= vlan_tab[vid].port_mask;    // Apply VLAN mask on port map
    if (find_entry(mac, fid, &fe)) {
        if (!update_entry(fe, vid, port_map, 0, DYNAMIC))
            goto unlock_d;
    } else {
        if (!insert_entry(&fe, mac, vid, port_map, 0, DYNAMIC)) {
            num_learned_entry_discards[fid]++;
            ret = -ENOMEM;                  // FDB full
            goto unlock_d;
        }
    }
    // Commit FDB changes
    write_entry(fe);
    rtu_hw_commit();

unlock_d:
    pthread_mutex_unlock(&fd_mutex);
    return ret;
}

/**
 * \brief Reads an entry from the filtering database
 * @param mac MAC address specification
 * @param vid VLAN identifier
 * @param port_map (OUT) port map specification for the filtering entry
 * @param entry_type (OUT) filtering entry type. DYNAMIC if the entry
 * contains dynamic information. STATIC if no dynamic info is being used.
 * @return 1 if entry was found and could be read. 0 otherwise.
 */
int rtu_fdb_read_entry(
           uint8_t mac[ETH_ALEN],
           uint8_t fid,
           uint32_t *port_map,
           int *entry_type)
{
    int ret = 0;
    struct filtering_entry *fe;

    pthread_mutex_lock(&fd_mutex);
    if (find_entry(mac, fid, &fe)) {
        *port_map   = fe->port_mask_dst;
        *entry_type = fe->dynamic;
        ret = 1;
    }
    pthread_mutex_unlock(&fd_mutex);
    return ret;
}

/**
 * \brief Reads the next entry in the filtering database, following the one with
 * indicated MAC and FID,  in lexicographic order. Returns the entry MAC
 * address, FID, port map and entry type.
 * @return 1 if next entry exists. 0 otherwise.
 */
int rtu_fdb_read_next_entry(
           uint8_t (*mac)[ETH_ALEN],                            // inout
           uint8_t *fid,                                        // inout
           uint32_t *port_map,                                  // out
           int *entry_type)                                     // out
{
    int ret = 0;
    struct filtering_entry *fe;

    pthread_mutex_lock(&fd_mutex);
    fe = find_next_fdb_node(*fid, *mac);
    if (fe) {
        mac_copy(*mac, fe->mac);
        *fid         = fe->fid;
        *port_map    = fe->port_mask_dst;
        *entry_type  = fe->dynamic;
        ret = 1;
    }
    pthread_mutex_unlock(&fd_mutex);
    return ret;
}

/**
 * \brief Creates or updates a static filtering entry in filtering database.
 * @param type
 * @param mac MAC address specification
 * @param vid VLAN identifier
 * @param port_map a port map specification with a control element for each
 * outbound port to specify filtering for that MAC address specification and VID
 * @return 0 if entry was created or updated. -ENOMEM if no space is available.
 */
int  rtu_fdb_create_static_entry(
            uint8_t mac[ETH_ALEN],
            uint16_t vid,
            enum filtering_control port_map[NUM_PORTS],
            enum storage_type type,
            int active)
{
    int i, j;
    int ret = 0, was_created = 0;
    struct static_filtering_entry *sfe;

    // if VLAN is not registered ignore request
    // (but the wildcard VID can still be used for management)
    if ((vid != WILDCARD_VID) && vlan_tab[vid].drop)
        return 0;

    pthread_mutex_lock(&fd_mutex);
    // Static FDB
    if (find_static_entry(mac, vid, &sfe)) {
        if (!update_static_entry(sfe, port_map, active))
            goto unlock_s;
    } else {
        if (!insert_static_entry(&sfe, mac, vid, port_map, type, active)) {
            ret = -ENOMEM;
            goto unlock_s;
        }
        was_created = 1;
    }
    // FDB
    if (vid == WILDCARD_VID) {
        // Register wildcard static entry for each VID that is currently in use
        for (i = 0; i < NUM_VLANS; i++) {
            if (!vlan_tab[i].drop) {
                if (!insert_static_entry_in_fdb(i, sfe, was_created)) {
                    // No memory... but some entries might have been correctly
                    // set up, so rollback changes to ensure consistency.
                    for (j = 0; j < i; j++)
                        if (!vlan_tab[i].drop)
                            delete_static_entry_from_fdb(j, sfe);
                    rtu_hw_rollback();
                    // Also remove static entry from static FDB
                    delete_static_entry(sfe);
                    ret = -ENOMEM;
                    goto unlock_s;
                }
            }
        }
    } else {
        ret = insert_static_entry_in_fdb(vid, sfe, was_created);
        if (ret == -ENOMEM) {
            // Rollback: just remove static entry from static FDB
            delete_static_entry(sfe);
            goto unlock_s;
        }
    }
    rtu_hw_commit();

unlock_s:
    pthread_mutex_unlock(&fd_mutex);
    return ret;
}


/**
 * TODO include support to wildcard VID
 * @return -EINVAL if vid is reserved.
 */
int rtu_fdb_create_static_vlan_entry(
            uint16_t vid,
            uint8_t fid,
        	enum registrar_control member_set[NUM_PORTS],
        	uint32_t untagged_set)
{
    uint32_t port_mask, use_dynamic;

    if (reserved(vid))
        return -EINVAL;

    calculate_vlan_vector(member_set, &port_mask, &use_dynamic);

    if (vlan_tab[vid].drop)         // VLAN not registered
        vlan_tab[vid].port_mask = port_mask;
    else
        // Combine static and dynamic part
        vlan_tab[vid].port_mask = port_mask |
          ((vlan_tab[vid].port_mask & vlan_tab[vid].use_dynamic) & use_dynamic);

    vlan_tab[vid].use_dynamic   = use_dynamic;
    vlan_tab[vid].untagged_set  = untagged_set;
    vlan_tab[vid].fid           = fid;
    vlan_tab[vid].has_prio      = 0;
    vlan_tab[vid].prio_override = 0;
    vlan_tab[vid].prio          = 0;
    vlan_tab[vid].drop          = 0;

    rtu_write_vlan_entry(vid, &vlan_tab[vid]);
    return 0;
}


/**
 * \brief Removes a static filtering entry from the filtering database.
 * @param mac MAC address specification
 * @param vid VLAN identifier
 * @return 0 if entry was deleted.-EPERM if entry removal is not permitted (i.e.
 * entry is Permanent or Read-Only)
 */
int rtu_fdb_delete_static_entry(uint8_t mac[ETH_ALEN], uint16_t vid)
{
    int i, ret = 0;
    struct static_filtering_entry *sfe;

    if ((vid != WILDCARD_VID) && vlan_tab[vid].drop)
        return 0;

    pthread_mutex_lock(&fd_mutex);
    if (find_static_entry(mac, vid, &sfe)) {
        // Permanent entries can not be removed
        if (sfe->type == Permanent) {
            ret = -EPERM;
            goto unlock_ds;
        }
        // Unregister entry from FDB
        if (vid == WILDCARD_VID) {          // ...for all VIDs in use
            for(i = 0; i < NUM_VLANS; i++)
                if (!vlan_tab[i].drop)
                    delete_static_entry_from_fdb(i, sfe);
        } else {
            delete_static_entry_from_fdb(vid, sfe);
        }
        // Remove from static fdb
        delete_static_entry(sfe);
    }
    rtu_hw_commit();

unlock_ds:
    pthread_mutex_unlock(&fd_mutex);
    return ret;
}


/**
 * \brief Reads an static entry from the static filtering database
 * @param mac MAC address specification
 * @param vid VLAN identifier
 * @param port_map (OUT) port map specification for the filtering entry
 * @param type (OUT) storage type for the filtering entry (Volatile, Permanent,)
 * @return 1 if entry was found and could be read. 0 otherwise.
 */
int rtu_fdb_read_static_entry(
            uint8_t mac[ETH_ALEN],
            uint16_t vid,
            enum filtering_control (*port_map)[NUM_PORTS],
            enum storage_type *type,
            int *active)
{
    int ret = 0;
    struct static_filtering_entry *sfe;

    pthread_mutex_lock(&fd_mutex);
    if (find_static_entry(mac, vid, &sfe)) {
        memcpy(*port_map, sfe->port_map, NUM_PORTS);
        *type   = sfe->type;
        *active = sfe->active;
        ret     = 1;
    }
    pthread_mutex_unlock(&fd_mutex);
    return ret;
}

/**
 * \brief Reads the next static entry in the static filtering database,
 * following the one with indicated MAC and FID,  in lexicographic order.
 * Returns the entry MAC address, FID, port map and entry type.
 * @return 1 if next entry exists. 0 otherwise.
 */
int rtu_fdb_read_next_static_entry(
            uint8_t (*mac)[ETH_ALEN],                           // inout
            uint16_t *vid,                                      // inout
            enum filtering_control (*port_map)[NUM_PORTS],      // out
            enum storage_type *type,                            // out
            int *active)                                        // out
{
    int ret = 0;
    struct static_filtering_entry *sfe;

    pthread_mutex_lock(&fd_mutex);
    sfe = find_next_static_fdb_node(*vid, *mac);
    if (sfe) {
        mac_copy(*mac, sfe->mac);
        memcpy(*port_map, sfe->port_map, NUM_PORTS);
        *vid    = sfe->vid;
        *type   = sfe->type;
        *active = sfe->active;
        ret     = 1;
    }
    pthread_mutex_unlock(&fd_mutex);
    return ret;
}


/**
 * \brief Reads a static vlan entry from the VLAN database.
 * @param vid VLAN identifier
 * @param member_set
 * @param untagged_set
 * @return 1 if entry was found and could be read. 0 otherwise.
 */
int rtu_fdb_read_static_vlan_entry(
            uint16_t vid,
	        enum registrar_control (*member_set)[NUM_PORTS],
	        uint32_t *untagged_set)
{
    if (vlan_tab[vid].drop)
        return 0;

    calculate_member_set(
            member_set,
            vlan_tab[vid].port_mask,
            vlan_tab[vid].use_dynamic
    );
    *untagged_set = vlan_tab[vid].untagged_set;
    return 1;
}

/**
 * \brief Deletes old filtering entries from filtering database to support
 * changes in active topology.
 */
void rtu_fdb_age_dynamic_entries(void)
{
    int i;
    int j;
    struct filtering_entry *fe;
    unsigned long t;            // (secs)

    update_aging_map();         // Work with latest access info
    update_fdb_age();           // Update filtering entries age

    pthread_mutex_lock(&fd_mutex);
    t = now() - aging_time;
    // HCAM
    for(j = CAM_ENTRIES; j-- > 0;){
        fe = &hcam[j];
        if(fe->valid && fe->dynamic && time_after(t, fe->last_access_t)) {
            TRACE_DBG(
                TRACE_INFO,
                "deleting hcam entry: mac = %s, bucket = %d\n",
                mac_to_string(fe->mac),
                j
            );
            delete_dynamic_entry(fe);
        }
    }
    // HTAB
    for (i = HTAB_ENTRIES; i-- > 0;) {
        for (j = RTU_BUCKETS; j-- > 0;) {
            fe = &htab[i][j];
            if(fe->valid && fe->dynamic && time_after(t, fe->last_access_t)){
                TRACE_DBG(
                    TRACE_INFO,
                    "deleting htab entry: mac = %s, hash = %d, bucket = %d\n",
                    mac_to_string(fe->mac),
                    i,
                    j
                );
                delete_dynamic_entry(fe);
            }
        }
    }
    rtu_hw_commit();            // Commit changes
    pthread_mutex_unlock(&fd_mutex);

    clean_aging_map();          // Keep track of entries access in next period
}

