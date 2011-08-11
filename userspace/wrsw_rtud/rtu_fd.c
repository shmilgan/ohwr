/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_rtud v1.0
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
static unsigned long aging_time = DEFAULT_AGING_TIME;

/**
 * Mirror of VLAN table
 */
static struct vlan_table_entry vlan_tab[NUM_VLANS];

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
void _link(struct filtering_entry *ent, int cam_bucket)
{
    ent->go_to_cam = 1;
    ent->cam_addr  = cam_addr(cam_bucket);
}

static inline 
void _unlink(struct filtering_entry *ent)
{
    ent->go_to_cam = 0;
    ent->cam_addr  = 0;
}

static inline 
int hcam_contains(struct filtering_entry *ent)
{
    return (ent >= hcam) && (ent < (hcam + CAM_ENTRIES));
}

static inline 
int empty(struct filtering_entry *ent)
{
    return !ent->valid;
}

static inline 
int hash(struct filtering_entry *ent) // only valid for HTAB ent!
{
    return (ent - &htab[0][0]) / RTU_BUCKETS;
}

static inline 
int htab_bucket(struct filtering_entry *ent)
{
    return (ent - &htab[0][0]) % RTU_BUCKETS;
}

static inline 
int hcam_bucket(struct filtering_entry *ent)
{
    return ent - hcam;
}

/**
 * Filtering database initialisation.
 */
static void clean_fd(void)
{
    memset(&htab, 0, sizeof(htab));
    memset(&hcam, 0, sizeof(hcam));
    rtu_hw_clean_fdb();
}

/**
 * VLAN database initialisation. VLANs are initially marked as disabled.
 */
static void clean_vd(void)
{
    int i;

    rtu_clean_vlan();
    for(i = 0; i < NUM_VLANS; i++) 
        vlan_tab[i].drop = 1;

    // Entry with VID 1 reserved for untagged packets.
    vlan_tab[1].port_mask       = 0xffffffff;
    vlan_tab[1].drop            = 0;
    vlan_tab[1].fid             = 0;
    vlan_tab[1].has_prio        = 0;
    vlan_tab[1].prio_override   = 0;
    vlan_tab[1].prio            = 0;

    rtu_write_vlan_entry(1, &vlan_tab[1]);
}

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
 * \brief Deletes HCAM entry.
 * Updates HTAB last entry if neccessary.
 * @param bucket CAM entry address
 */
static void delete_hcam_entry(int bucket)
{
    struct filtering_entry *ent, *prev;
    uint16_t hash = -1;     // informs unlinking required

    ent = &hcam[bucket];
    if (ent->end_of_bucket) {
        if (bucket == 0) {
            // this is first HCAM entry (so no more in this list). Unlink HTAB
            hash = rtu_hash(ent->mac, ent->fid);
        } else {
            prev = ent-1;
            if (prev->valid && !prev->end_of_bucket) {
                // if previous is part of this list, mark it as the new end
                prev->end_of_bucket = 1;
                rtu_hw_write_hcam_entry(cam_addr(bucket-1), prev);
            } else {
                // ent was the only entry on this list. Unlink HTAB
                hash = rtu_hash(ent->mac, ent->fid);
            }                
        }
    } else {
        // shift entries
        for(; !ent->end_of_bucket; ent++, bucket++){
            rtu_fe_copy(ent, ent+1);
            rtu_hw_write_hcam_entry(cam_addr(bucket), ent);
        }
    }
    rtu_fe_clean(ent);
    rtu_hw_clean_hcam_entry(cam_addr(bucket));

    if (hash >= 0) {
        ent  = &htab[hash][LAST_RTU_BUCKET];
        _unlink(ent);
        rtu_hw_write_htab_entry(zbt_addr(hash, LAST_RTU_BUCKET), ent);
    }
}

/**
 * \brief Deletes HTAB entry by shifting HTAB list.
 * If HCAM is used, it also copies first HCAM entry to last HTAB bucket.
 * @param hash hashcode for entry to remove.
 * @param bucket HTAB bucket for entry to remove
 */
static void delete_htab_entry(uint16_t hash, int bucket)
{
    struct filtering_entry *ent, *cam_ent; 

    // shift entries
    ent = &htab[hash][bucket];
    for(; (bucket < LAST_RTU_BUCKET) && (ent+1)->valid; bucket++, ent++){
        rtu_fe_copy(ent, ent+1);
        rtu_hw_write_htab_entry(zbt_addr(hash, bucket), ent);
    }
    // If HTAB was full, check if HCAM was also used.
    if(bucket == LAST_RTU_BUCKET){
        if(ent->go_to_cam){
            // go_to_cam was copied to previous entry while shifting. clean it.
            _unlink(ent-1);
            // copy first cam entry into last HTAB entry
            bucket  = cam_bucket(ent->cam_addr);
            cam_ent = &hcam[bucket];
            rtu_fe_copy(ent, cam_ent);
            // adjust pointer to HCAM
            if(cam_ent->end_of_bucket){
                ent->end_of_bucket = 0;
                _unlink(ent);
            } else {                 
                _link(ent, bucket+1);
            }
            rtu_hw_write_htab_entry(zbt_addr(hash, LAST_RTU_BUCKET), ent);
            // clean HCAM entry
            rtu_fe_clean(cam_ent);
            rtu_hw_clean_hcam_entry(cam_addr(bucket));
        } else {
            // clean last HTAB entry
            rtu_fe_clean(ent);
            rtu_hw_clean_htab_entry(zbt_addr(hash, LAST_RTU_BUCKET));
        }
    }
}

/**
 * \brief Searches for an entry with given mac and fid in FDB (HTAB and HCAM).
 * The ent pointer will point to the found entry. If no entry found, ent will
 * point to next empty entry in HTAB if it exists. Otherwise, ent will point 
 * to the last entry for the list.
 * @return 1 if found. 0 otherwise.
 */
static int find_entry(
        uint8_t mac[ETH_ALEN],
        uint8_t fid,
        struct filtering_entry **ent)
{
    int bucket = 0;
    uint16_t hash;

    // Check HTAB
    hash  = rtu_hash(mac, fid);
    *ent = &htab[hash][0];
    for(; bucket < RTU_BUCKETS; bucket++, (*ent)++) {
        if (empty(*ent))
            return 0;
        if (mac_equal((*ent)->mac, mac) && ((*ent)->fid == fid))
            return 1;
        if (bucket == LAST_RTU_BUCKET)
            break;
    }
    // if HCAM is used, check it also.
    if((*ent)->go_to_cam){
        bucket = cam_bucket((*ent)->cam_addr);        
        for(; bucket < CAM_ENTRIES; bucket++) {
            *ent = &hcam[bucket];
            if (mac_equal((*ent)->mac, mac) && ((*ent)->fid == fid))
                return 1; 
            if ((*ent)->end_of_bucket)
                return 0; 
        }
    } 
    return 0; // Not found in HTAB. HCAM not used yet.
}

/**
 * \brief Add a new entry at the end of a list associated to a given hash.
 * @param ent pointer to last entry of current list.
 * @return pointer to entry added 
 */
static struct filtering_entry *add_hcam_entry(struct filtering_entry *ent)
{
    struct filtering_entry *next;
    int bucket;

    if (hcam_contains(ent)) {        
        // Append entry at end of list
        if (ent == hcam + LAST_CAM_ENTRY)
            return NULL;        // Last hcam entry. no more space
        next = ent + 1;
        if(!empty(next))     
            return NULL;        // Following entry already stores another list
        ent->end_of_bucket  = 0;
        rtu_hw_write_hcam_entry(cam_addr(hcam_bucket(ent)), ent);
    } else {                    // ent is last HTAB entry
        bucket = find_empty_bucket_in_hcam();
        if (bucket < 0)         
            return NULL;        // FDB full
        next = hcam + bucket;
        // Update ent to point to hcam entry
        _link(ent, bucket);
        rtu_hw_write_htab_entry(zbt_addr(hash(ent), LAST_RTU_BUCKET), ent);
    }    
    next->end_of_bucket = 1;
    return next;
}

/**
 * \brief Writes entry to RTU HW.
 */
static void write_entry(struct filtering_entry *ent)
{
    if (hcam_contains(ent))
        rtu_hw_write_hcam_entry(cam_addr(hcam_bucket(ent)), ent);
    else
        rtu_hw_write_htab_entry(zbt_addr(hash(ent), htab_bucket(ent)), ent);
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
 * \brief Sets the aging time for dynamic filtering entries.
 * @param t new aging time value [seconds].
 * @return -EINVAL if t < 10 or t > 1000000 (802.1Q, Table 8.3); 0 otherwise.
 */
int rtu_fdb_set_aging_time(unsigned long t)
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
    int err;

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

    return 0;
}

/**
 * \brief Creates or updates a filtering entry in the filtering database.
 * @param mac MAC address specification
 * @param vid VLAN identifier
 * @param port_map a port map specification with a control element for each
 * outbound port to specify filtering for that MAC address specification and VID
 * @param dynamic it indicates whether it's a dynamic entry
 * @return 0 if entry was created or updated. -ENOMEM if no space is available.
 */
int rtu_fdb_create_entry(uint8_t mac[ETH_ALEN], uint16_t vid, uint32_t port_map, int dynamic)
{
    int ret = 0, updated = 0, fid;
    struct filtering_entry *ent;          
    uint32_t mask;                  // used to check src port mask update    

    // if VLAN is not registered ignore request
    if (vlan_tab[vid].drop)
        return 0;
    // if member set for VID does not contain at least one port, ignore request
    if (vlan_tab[vid].port_mask == 0)
        return 0;

    pthread_mutex_lock(&fd_mutex);
    fid = vlan_tab[vid].fid;
    if (find_entry(mac, fid, &ent)) {
        // update
        if (ent->port_mask_dst != port_map) {
            ent->port_mask_dst = port_map;
            updated = 1;
        }
        mask = ent->port_mask_src | vlan_tab[vid].port_mask;
        if (ent->port_mask_src != mask) {
            ent->port_mask_src = mask;
            updated = 1;
        }
    } else {
        // insert
        if(!empty(ent))     // HTAB full
            ent = add_hcam_entry(ent);

        if(ent){        
            mac_copy(ent->mac, mac);
            ent->fid           = fid;
            ent->port_mask_dst = port_map;
            ent->port_mask_src = vlan_tab[vid].port_mask;
            ent->dynamic       = dynamic;
            ent->last_access_t = now();
            ent->valid         = 1;
            updated = 1;
        } else {                
            ret = -ENOMEM;  // FDB full
        }
    }
    if (updated) {
        write_entry(ent);
        rtu_hw_commit();
    }
    pthread_mutex_unlock(&fd_mutex);
    return ret;
}


/**
 * \brief Deletes old filtering entries from filtering database to support
 * changes in active topology.
 */
void rtu_fdb_age(void)
{
    int i;                                      
    int j;                                      
    struct filtering_entry *ent;                
    unsigned long t;            // (secs)

    update_aging_map();         // Work with latest access info
    update_fdb_age();           // Update filtering entries age

    pthread_mutex_lock(&fd_mutex);
    t = now() - aging_time;
    // HCAM
    for(j = CAM_ENTRIES; j-- > 0;){
        ent = &hcam[j];
        if(ent->valid && ent->dynamic && time_after(t, ent->last_access_t)) {
            TRACE_DBG(
                TRACE_INFO,
                "deleting hcam entry: mac = %s, bucket = %d\n",
                mac_to_string(ent->mac),
                j
            );
            delete_hcam_entry(j);
        }
    }
    // HTAB
    for (i = HTAB_ENTRIES; i-- > 0;) {
        for (j = RTU_BUCKETS; j-- > 0;) {
            ent = &htab[i][j];
            if(ent->valid && ent->dynamic && time_after(t, ent->last_access_t)){
                TRACE_DBG(
                    TRACE_INFO,
                    "deleting htab entry: mac = %s, hash = %d, bucket = %d\n",
                    mac_to_string(ent->mac),
                    i,
                    j
                );
                delete_htab_entry(i, j);
            }
        }
    }    
    rtu_hw_commit();            // Commit changes
    pthread_mutex_unlock(&fd_mutex);

    clean_aging_map();          // Keep track of entries access in next period
}

