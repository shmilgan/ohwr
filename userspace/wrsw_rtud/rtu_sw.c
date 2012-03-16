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
 * Description: RTU Filtering Database Mirror.
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
#include <hw/wrsw_rtu_wb.h>
#include <hw/trace.h>

#include "rtu_drv.h"
#include "rtu_sw.h"
#include "rtu_hash.h"

/**
 * Mirror of Aging RAM.
 */
static uint32_t agr_htab[RTU_ARAM_MAIN_WORDS];
static uint32_t agr_hcam;

/**
 * Mirror of ZBT SRAM memory MAC address table.
 * Main filtering table organized as hash table with 4-entry buckets.
 * Note both banks have the same content. Therefore SW only mirrors one bank.
 */
static struct filtering_entry  htab[HTAB_ENTRIES][RTU_BUCKETS];
static struct filtering_entry _htab[HTAB_ENTRIES][RTU_BUCKETS];

/**
 * Mirror of CAM lookup table.
 * For RTU entries with more than 4 matches
 */
static struct filtering_entry  hcam[CAM_ENTRIES];
static struct filtering_entry _hcam[CAM_ENTRIES];

/**
 * Mirror of HW VLAN table
 */
static struct vlan_table_entry  vlan_tab[NUM_VLANS];
static struct vlan_table_entry _vlan_tab[NUM_VLANS];

/**
 * Learning Constraint Sets. Set 0 (IVL) used as default.
 */
int lc_tab[NUM_LC_SETS] = {LC_INDEPENDENT};

/**
 * Default Learning Constraint Set. VLANs which are not explicitly assigned
 * to a Set, are implicitly associated to the default set. Default value (0)
 * makes the bridge to apply Independent VLAN Learning.
 */
int default_lc_set;

/**
 * Table bank to write entries to.
 * HTAB and HCAM banks will be handled according to this single bank value.
 */
static uint8_t bank;

/**
 * Holds pointers to entries that were changed at HCAM SW since last commit.
 */
static struct filtering_entry *htab_wr[RTU_ENTRIES];
static int htab_wr_head;

/**
 * Holds pointers to entries that were changed at HCAM SW since last commit.
 */
static struct filtering_entry *hcam_wr[CAM_ENTRIES];
static int hcam_wr_head;

/**
 * Holds pointers to entries that were changed at VLAN tab SW since last commit.
 */
static struct vlan_table_entry *vlan_tab_wr[NUM_VLANS];
static int vlan_tab_wr_head;

static inline
int matched(uint32_t word, int offset)
{
    return (word >> offset) & 0x00000001;
}

static inline
int empty(struct filtering_entry *fe)
{
    return !fe->valid;
}

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
void htab_link(struct filtering_entry *fe, int cam_bucket)
{
    fe->go_to_cam = 1;
    fe->cam_addr  = cam_addr(cam_bucket);
}

static inline
void htab_unlink(struct filtering_entry *fe)
{
    fe->go_to_cam = 0;
    fe->cam_addr  = 0;
}

static inline
int htab_hash(struct filtering_entry *fe) // only valid for HTAB entries!
{
    return (fe - &htab[0][0]) / RTU_BUCKETS;
}

static inline
int htab_bucket(struct filtering_entry *fe)
{
    return (fe - &htab[0][0]) % RTU_BUCKETS;
}

static inline
int hcam_contains(struct filtering_entry *fe)
{
    return (fe >= hcam) && (fe < (hcam + CAM_ENTRIES));
}


static inline
int hcam_bucket(struct filtering_entry *fe)
{
    return fe - hcam;
}

static inline
int vlan_vid(struct vlan_table_entry *ve)
{
    return ve - vlan_tab;
}

/**
 * Set the active bank both in software and hardware.
 * Both HTAB and HCAM active banks are switched at once. Bank switching is
 * delayed until MFIFO is empty (method remains blocked meanwhile).
 */
static void set_active_bank(int b)
{
    // wait until MFIFO is empty
    rtu_hw_clean_mfifo();
    // inactive bank becomes active (both banks are switched at once)
	rtu_hw_set_active_bank(b);
    // active bank becomes inactive one
    bank = (b == 0) ? 1:0;
}

/**
 * Write HCAM entry to HW. Delayed until operation is commited.
 */
static void hcam_write(struct filtering_entry *fe)
{
    int i;
    // Avoid duplicates
    for (i = 0; i < hcam_wr_head; i++)
        if (hcam_wr[i] == fe)
            return;
    hcam_wr[hcam_wr_head++] = fe;
}

/**
 * Write HTAB entry to HW. Delayed until operation is commited.
 */
static void htab_write(struct filtering_entry *fe)
{
    int i;
    // Avoid duplicates
    for (i = 0; i < htab_wr_head; i++)
        if (htab_wr[i] == fe)
            return;
    htab_wr[htab_wr_head++] = fe;
}

/**
 * Write VLAN table entry to HW. Delayed until operation is commited.
 */
static void vlan_write(struct vlan_table_entry *ve)
{
    int i;
    // Avoid duplicates
    for (i = 0; i < vlan_tab_wr_head; i++)
        if (vlan_tab_wr[i] == ve)
            return;
    vlan_tab_wr[vlan_tab_wr_head++] = ve;
}

/**
 * Find the most appropriate empty bucket to insert new hash collision
 * list. The algorithm first finds the fragment which contains the max number of
 * consecutive empty positions. Then divides this fragment into two parts: first
 * block is still available for possible increment of any existing list; The
 * second block will be available for the new list.
 * The algorithm keeps a fair and uniform distribution of fragments space.
 * @return bucket index or -1 if the HCAM table is full.
 */
static int hcam_find_empty_bucket(void)
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
 * Add a new entry at the end of a list associated to a given hash.
 * @param ent pointer to last entry of current list.
 * @return pointer to entry added
 */
static struct filtering_entry *hcam_add(struct filtering_entry *fe)
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
        hcam_write(fe);
    } else {                    // ent is last HTAB entry. Add first HCAM entry
        bucket = hcam_find_empty_bucket();
        if (bucket < 0)
            return NULL;        // FDB full
        next = hcam + bucket;
        // Update ent to point to hcam entry
        htab_link(fe, bucket);
        htab_write(fe);
    }
    next->end_of_bucket = 1;
    return next;
}

/**
 * Deletes HCAM entry.
 * Updates HTAB last entry if neccessary.
 * @param bucket CAM entry address
 */
static void hcam_delete(int bucket)
{
    struct filtering_entry *fe, *prev;
    int hash = -1;     // informs unlinking required.

    fe = &hcam[bucket];

    TRACE_DBG(
        TRACE_INFO,
        "deleting hcam entry: mac = %s, bucket = %d\n",
        mac_to_string(fe->mac),
        bucket
    );

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
                hcam_write(prev);
            } else {
                // fe was the only entry on this list. Get hash to unlink HTAB
                hash = rtu_hash(fe->mac, fe->fid);
            }
        }
    } else {
        // shift entries
        for (; !fe->end_of_bucket; fe++, bucket++) {
            rtu_fe_copy(fe, fe + 1);
            hcam_write(fe);
        }
    }
    rtu_fe_clean(fe);
    hcam_write(fe);

    // Check if we need to unlink HTAB
    if (hash >= 0) {
        fe = &htab[hash][LAST_RTU_BUCKET];
        htab_unlink(fe);
        htab_write(fe);
    }
}

/**
 * Deletes HTAB entry by shifting HTAB list.
 * If HCAM is used, it also copies first HCAM entry to last HTAB bucket.
 * @param hash hashcode for entry to remove.
 * @param bucket HTAB bucket for entry to remove
 */
static void htab_delete(uint16_t hash, int bucket)
{
    struct filtering_entry *fe, *cam_fe;

    // Shift entries
    fe = &htab[hash][bucket];

    TRACE_DBG(
        TRACE_INFO,
        "deleting htab entry: mac = %s, hash = %d, bucket = %d\n",
        mac_to_string(fe->mac),
        hash,
        bucket
    );

    for(; (bucket < LAST_RTU_BUCKET) && (fe+1)->valid; bucket++, fe++){
        rtu_fe_copy(fe, fe + 1);
        htab_write(fe);
    }
    // If HTAB was full, check if HCAM was also used.
    if(bucket == LAST_RTU_BUCKET){
        if(fe->go_to_cam){
            // go_to_cam was copied to previous entry while shifting. clean it.
            htab_unlink(fe-1);
            // copy first cam entry into last HTAB entry
            bucket  = cam_bucket(fe->cam_addr);
            cam_fe = &hcam[bucket];
            rtu_fe_copy(fe, cam_fe);
            // adjust pointer to HCAM
            if(cam_fe->end_of_bucket){
                fe->end_of_bucket = 0;
                htab_unlink(fe);
            } else {
                htab_link(fe, bucket + 1);
            }
            htab_write(fe);
            // clean HCAM entry
            rtu_fe_clean(cam_fe);
            hcam_write(cam_fe);
        } else {
            // clean last HTAB entry
            rtu_fe_clean(fe);
            htab_write(fe);
        }
    } else {
        rtu_fe_clean(fe);
        htab_write(fe);
    }
}

//------------------------------------------------------------------------------
// Filtering Entries
//------------------------------------------------------------------------------

/**
 * Searches for an entry with given mac and fid in RTU mirror (HTAB and HCAM).
 * The fe pointer will point to the found entry. If no entry found, ent will
 * point to next empty entry in HTAB if it exists. Otherwise, ent will point
 * to the last entry for the list.
 * @return 1 if found. 0 otherwise.
 */
int rtu_sw_find_entry(
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
 * Creates an entry in the FDB mirror (either at HTAB or HCAM).
 * @param fe as returned from rtu_sw_find_entry should be passed as IN
 * parameter. On return it will point to the created entry.
 * @param dynamic indicates the type of info. 1=dynamic. 0=static
 * @return 0 if entry was inserted. -ENOMEM if FDB is full.
 */
int rtu_sw_create_entry(
        struct filtering_entry **fe,
        uint8_t  mac[ETH_ALEN],
        uint16_t vid,
        uint32_t port_map,
        uint32_t use_dynamic,
        int dynamic)
{
    int htab_full = !empty(*fe);

    if(htab_full)
        *fe = hcam_add(*fe);

    if (!*fe)
        return -ENOMEM; // FDB full

    mac_copy((*fe)->mac, mac);
    (*fe)->fid           = vlan_tab[vid].fid;
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
    // Write entry to RTU HW
    if(htab_full)
        hcam_write(*fe);
    else
        htab_write(*fe);
    return 0;
}

/**
 * Updates an entry in the FDB mirror (either at HTAB or HCAM).
 * @param fe pointer to FDB entry as returned from other rtu_sw methods.
 */
void rtu_sw_update_entry(struct filtering_entry *fe)
{
    if (hcam_contains(fe))
        hcam_write(fe);
    else
        htab_write(fe);
}

void rtu_sw_update_vlan_entry(struct vlan_table_entry *ve)
{
    vlan_write(ve);
}

/**
 * Deletes a filtering entry from the FDB mirror (either at HTAB or HCAM).
 * @param fe pointer to FDB entry as returned from other rtu_sw methods.
 */
void rtu_sw_delete_entry(struct filtering_entry *fe)
{
    // Delete from HTAB or HCAM as appropriate
    if (hcam_contains(fe))
        hcam_delete(hcam_bucket(fe));
    else
        htab_delete(htab_hash(fe), htab_bucket(fe));
}

/* Delete all FDB entries for the given fid and port (e.g. after deleting a
   VLAN entry */
void rtu_sw_delete_dynamic_entries(int port, uint16_t vid)
{
    int i, j;
    uint8_t fid;
    struct filtering_entry *fe;

    /* Check that vlan is registered */
    if (vlan_tab[vid].drop)
        return;

    /* Check that port is in vlan */
    if (!is_set(vlan_tab[vid].port_mask, port))
        return;

    fid = vlan_tab[vid].fid;

    /* Note removing a dynamic entry results in a forward decision */
    for (i = 0; i < HTAB_ENTRIES; i++) {
        for (j = 0; j < RTU_BUCKETS; j++) {
            fe = &htab[i][j];
            if (fe->valid && (fe->fid == fid)) {
                if (is_set(fe->use_dynamic, port) &&
                    !is_set(fe->port_mask_dst, port)) {
                    set(&fe->port_mask_dst, port);
                    htab_write(fe);
                }
            }
        }
    }

    for (i = 0; i < CAM_ENTRIES; i++) {
        fe = &hcam[i];
        if (fe->valid && (fe->fid == fid)) {
            if (is_set(fe->use_dynamic, port) &&
                !is_set(fe->port_mask_dst, port)) {
                set(&fe->port_mask_dst, port);
                hcam_write(fe);
            }
        }
    }
}

/**
 * Filtering database initialisation.
 */
void rtu_sw_clean_fd(void)
{
    int i;

    memset(&htab, 0, sizeof(htab));
    memset(&hcam, 0, sizeof(hcam));
    for (i = 0; i < RTU_BANKS; i++) {
        set_active_bank(i);
        rtu_hw_clean_htab();
        rtu_hw_clean_hcam();
    }
}

//------------------------------------------------------------------------------
// VLAN Registration Entries
//------------------------------------------------------------------------------

int rtu_sw_get_num_vlans(void)
{
    int i, n;

    for (i = 0, n = 0; i < NUM_VLANS; i++)
        if (!vlan_tab[i].drop)
            n++;
    return n;
}

int rtu_sw_get_num_static_vlan_entries(void)
{
    int i, n;

    for (i = 0, n = 0; i < NUM_VLANS; i++)
        if (!vlan_tab[i].drop && (vlan_tab[i].dynamic != DYNAMIC))
            n++;
    return n;
}

int rtu_sw_get_num_dynamic_vlan_entries(void)
{
    int i, n;

    for (i = 0, n = 0; i < NUM_VLANS; i++)
        if (!vlan_tab[i].drop && (vlan_tab[i].dynamic != STATIC))
            n++;
    return n;
}

struct vlan_table_entry *rtu_sw_get_vlan_entry(uint16_t vid)
{
    return &vlan_tab[vid];
}

/**
 * Searches for a VLAN entry with given vid in RTU mirror.
 * @return  pointer to VLAN entry if found. NULL otherwise.
 */
struct vlan_table_entry *rtu_sw_find_vlan_entry(uint16_t vid)
{
    if ((vid >= NUM_VLANS) || (vlan_tab[vid].drop))
        return NULL;
    return &vlan_tab[vid];
}

struct vlan_table_entry *rtu_sw_find_next_ve(uint16_t *vid)
{
    uint16_t _vid;
    struct vlan_table_entry *ve;

    for (_vid = *vid + 1; _vid < NUM_VLANS; _vid++)
        if (!vlan_tab[_vid].drop)
            break;
    *vid = _vid;
    ve = rtu_sw_find_vlan_entry(_vid);
    return ve;
}

struct vlan_table_entry *rtu_sw_create_vlan_entry(
        uint16_t vid,
        uint32_t port_mask,
        uint32_t use_dynamic,
        uint32_t untagged_set,
        int dynamic)
{
    vlan_tab[vid].port_mask     = port_mask;
    vlan_tab[vid].untagged_set  = untagged_set;
    vlan_tab[vid].use_dynamic   = use_dynamic;
    vlan_tab[vid].dynamic       = dynamic;
    // TODO dynamic flag is obsolete. Use is_static instead.
    vlan_tab[vid].is_static     = (dynamic == STATIC);
    vlan_tab[vid].drop          = 0;
    vlan_tab[vid].has_prio      = 0;
    vlan_tab[vid].prio_override = 0;
    vlan_tab[vid].prio          = 0;
    vlan_tab[vid].creation_t    = now();
    vlan_write(&vlan_tab[vid]);
    return 0;
}

void rtu_sw_delete_vlan_entry(uint16_t vid)
{
    struct vlan_table_entry *vlan = &vlan_tab[vid];

    vlan->drop = 1;
    if (!vlan->fid_fixed)
        vlan->fid = 0;
    vlan_write(vlan);
}

/**
 * VLAN database initialisation. VLANs are initially marked as disabled except
 * default VLAN which is configured according to 802.1Q section 8.8.2:
 * registration fixed on all ports; All ports are marked as untagged.
 */
void rtu_sw_clean_vd(void)
{
    int i;

    rtu_hw_clean_vlan();
    for(i = 0; i < NUM_VLANS; i++)
        vlan_tab[i].drop = 1;

    // Entry with VID 1 reserved for untagged packets.
    rtu_sw_create_vlan_entry(
        DEFAULT_VID,
        0xffffffff,     // port_mask:
        0x00000000,     // use_dynamic: Registration fixed for all ports
        0xffffffff,     // untagged_set: All untagged (See 802.1Q section 8.8.2)
        STATIC
    );
    rtu_sw_allocate_fid(DEFAULT_VID);
    rtu_hw_write_vlan_entry(DEFAULT_VID, &vlan_tab[DEFAULT_VID]);
}

/**
 * Returns 1 if multiple VLANS share a given FID. Returns 0 otherwise.
 */
int rtu_sw_fid_shared(uint8_t fid)
{
    int vid, n;

    for (vid = 0, n = 0; vid < NUM_VLANS; vid++)
        if ((!vlan_tab[vid].drop) && (vlan_tab[vid].fid == fid) && (++n > 1))
                return 1;
    return 0;
}

//------------------------------------------------------------------------------
// Aging
//------------------------------------------------------------------------------

/**
 * Clean HCAM aging register and HTAB aging bitmap.
 */
void rtu_sw_clean_aging_map(void)
{
    int i;

    agr_hcam = 0x00000000;
    rtu_hw_clean_agr_hcam();

    for(i = 0; i < RTU_ARAM_MAIN_WORDS; i++)
        agr_htab[i] = 0x00000000;
    rtu_hw_clean_agr_htab();
}

/**
 * Update aging map cache with contents read from aging registers at HW.
 */
void rtu_sw_update_aging_map(void)
{
    int i;

    agr_hcam = rtu_hw_read_agr_hcam();
    for(i = 0; i < RTU_ARAM_MAIN_WORDS; i++)
        agr_htab[i] = rtu_hw_read_agr_htab(i);
}

/**
 * Updates the age of filtering entries accessed in the last period.
 */
void rtu_sw_update_fd_age(void)
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
 * Caches RTU memory content.
 */
void rtu_sw_cache(void)
{
    memcpy(_htab, htab, sizeof(htab));
    memcpy(_hcam, hcam, sizeof(hcam));
    memcpy(_vlan_tab, vlan_tab, sizeof(vlan_tab));
}

/**
 * Uncaches RTU memory content.
 */
void rtu_sw_uncache(void)
{
    memcpy(htab, _htab, sizeof(htab));
    memcpy(hcam, _hcam, sizeof(hcam));
    memcpy(vlan_tab, _vlan_tab, sizeof(vlan_tab));
}

/**
 * Commit SW entry changes to HTAB, HCAM and VLAN_TAB at HW.
 */
void rtu_sw_commit(void)
{
    int i, j, k;
    struct filtering_entry *fe;
    struct vlan_table_entry *ve;

    if ((hcam_wr_head == 0) && (htab_wr_head == 0) && (vlan_tab_wr_head == 0))
        return;

    // Commit VLAN table writes
    for (j = 0; j < vlan_tab_wr_head; j++) {
        ve = vlan_tab_wr[j];
        rtu_hw_write_vlan_entry(vlan_vid(ve), ve);
    }
    // Commit HTAB/HCAM writes for both banks
    for (i = 0; i < RTU_BANKS; i++) {
        // write entries to inactive bank
        for (j = 0; j < hcam_wr_head; j++) {
            fe = hcam_wr[j];
            rtu_hw_write_hcam_entry(cam_addr(hcam_bucket(fe)), fe);
        }
        for (k = 0; k < htab_wr_head; k++) {
            fe = htab_wr[k];
            rtu_hw_write_htab_entry(zbt_addr(htab_hash(fe), htab_bucket(fe)), fe);
        }
        // switch bank to make entries available to RTU at HW
        if ((j > 0) || (k > 0))
            if (i < (RTU_BANKS - 1))
                set_active_bank(bank);
    }
    // Reset lists
    hcam_wr_head     = 0;
    htab_wr_head     = 0;
    vlan_tab_wr_head = 0;
}

/**
 * Rolls back pending HW write operations.
 */
void rtu_sw_rollback(void)
{
    hcam_wr_head     = 0;
    htab_wr_head     = 0;
    vlan_tab_wr_head = 0;
}

//-----------------------------------------------------------------------------
// Learning Constraints
//-----------------------------------------------------------------------------

/**
 * Return learning constraint set for the given vlan. If no lc set is defined,
 * the default lc set is returned.
 */
inline static uint32_t get_lc_set(uint16_t vid)
{
    uint32_t set = vlan_tab[vid].lc_set;
    return set ? set : (1 << default_lc_set);
}

/**
 * Fetch shared and independent sets from a learning constraint set.
 */
static void fetch_lc_sets(uint32_t lc_set, uint32_t *s_set, uint32_t *i_set)
{
    int i;

    for (i = 0; i < NUM_LC_SETS; i++) {
        if (is_set(lc_set, i)) {
            switch (lc_tab[i]) {
            case LC_SHARED:      set(s_set, i); break;
            case LC_INDEPENDENT: set(i_set, i); break;
            default: break;
            }
        }
    }
}

/**
 * Check that learning constraints related to the given vlan are consistent.
 * @return 1 if lc set is consistent. 0 otherwise.
 */
static int is_lc_set_consistent(uint16_t vid)
{
    int i;
    uint8_t fid2, fid1 = vlan_tab[vid].fid;
    uint32_t lc_set, s_set = 0, i_set = 0;   /* learning constraint set masks */

    lc_set = get_lc_set(vid);
    fetch_lc_sets(lc_set, &s_set, &i_set);
    for (i = 0; i < NUM_VLANS; i++) {
        /* skip given vlan */
        if (i == vid)
            continue;

        /* Other VLANs in the same shared set can not be in another independent
           set where the given VLAN appears. Other VLANs in the same independent
           set can not be in another shared set where the given VLAN appears */
        lc_set = get_lc_set(i);
        if ((lc_set & s_set) && (lc_set & i_set))
            return 0;

        /* skip vlans with no fid */
        fid2 = vlan_tab[i].fid;
        if (!fid1 || !fid2)
            continue;

        /* Check that VLANs in the same shared set have been allocated the same
           FID and other VLANs in the same independent learning constraint set
           have not been allocated the same FID */
        if (((lc_set & s_set) && (fid2 != fid1)) ||
            ((lc_set & i_set) && (fid2 == fid1)))
            return 0;
    }
    return 1;
}

/**
 * Allocates VID to exactly one FID.
 * TODO FID reconfiguration if fid was dynamic.
 * @return 0 if FID allocated. -1 on error.
 */
int rtu_sw_allocate_fid(uint16_t vid)
{
    int i;
    int fid;
    char fbd_fids[NUM_FIDS] = {}; /* Forbidden FIDs as per I constraints */
    uint8_t req_fid = 0;          /* Required FIDs as per S constraints */
    uint32_t lc_set, s_set = 0, i_set = 0;

    if (vlan_tab[vid].fid)   /* fid already allocated */
        return 0;

    /* If a given VID appears in an S Constraint then it shall be allocated to
       the same FID as the other VID identified in the same S Constraint. If a
       given VID appears in an I Constraint, then it shall not be allocated to
       the same FID as any other VID that appears in an I Constraint with the
       same Independent Set Identifier. */
    lc_set = get_lc_set(vid);
    fetch_lc_sets(lc_set, &s_set, &i_set);
    for (i = 0; i < NUM_VLANS; i++) {
        /* skip given vlan */
        if (i == vid)
            continue;
        /* skip vlans with no fid */
        fid = vlan_tab[i].fid;
        if (!fid)
            continue;

        lc_set =  get_lc_set(i);
        if (lc_set & s_set) {
            if (req_fid) {
              if (req_fid != fid)
                return -1;
            } else {
                req_fid = fid;
            }
        }

        if (lc_set & i_set)
            fbd_fids[fid] = 1;
    }

    /* Allocate VID to any FID that appears in an S Constraint */
    if (req_fid) {
        /* Make sure the same FID is not required and forbidden at the same time */
        if (fbd_fids[req_fid])
            return -1;
        vlan_tab[vid].fid = req_fid;
        return 0;
    }

    /* Allocate VID to an FID that does not appear in any I Constraint */
    for (i = 1; i < NUM_FIDS; i++) { /* fid = 0 reserved */
        if (fbd_fids[i])
            continue;
        vlan_tab[vid].fid = (uint8_t)i;
        return 0;
    }

    /* At this point the number of FIDs is not enough to meet the constraints */
    return -1;
}

/**
 * Creates a learning constraint. If the new constraint is inconsistent with
 * those already configured in the Bridge and VID to FID allocations, then the
 * management operation shall not be performed and an error response shall be
 * returned.
 * @return 0 if lc created. -1 if inconsistency detected.
 */
int rtu_sw_create_lc(int sid, uint16_t vid, int lc_type)
{
    int cur_lc_type = lc_tab[sid];
    uint32_t cur_lc_set = vlan_tab[vid].lc_set;

    if ((cur_lc_type != LC_UNDEFINED) && (cur_lc_type != lc_type))
        return -EINVAL;

    lc_tab[sid] = lc_type;
    set(&vlan_tab[vid].lc_set, sid);

    if (!is_lc_set_consistent(vid))
        goto rollback;

    return 0;

rollback:
    lc_tab[sid] = cur_lc_type;
    vlan_tab[vid].lc_set = cur_lc_set;
    return -1;

}

/**
 * Deletes a learning constraint.
 */
void rtu_sw_delete_lc(int sid, uint16_t vid)
{
    int i;

    unset(&vlan_tab[vid].lc_set, sid);
    for (i = 0; i < NUM_VLANS; i++)
        if (is_set(vlan_tab[i].lc_set, sid))
            return;
    lc_tab[sid] = LC_UNDEFINED;
}

/**
 * Read the learning constraints set for the given vlan.
 */
void rtu_sw_read_lc(int vid, uint32_t *lc_set)
{
    *lc_set = vlan_tab[vid].lc_set;
}

/**
 * Read the learning constraint set for the VLAN following the given one.
 */
int rtu_sw_read_next_lc(uint16_t *vid, uint32_t *lc_set)
{
    for (*vid = *vid + 1; *vid < NUM_VLANS; (*vid)++) {
        if (vlan_tab[*vid].lc_set) {
            *lc_set = vlan_tab[*vid].lc_set;
            return 0;
        }
    }
    return -1;
}

/**
 * Read the learning constraint set type following the given one.
 */
void rtu_sw_get_lc_set_type(int sid, int *lc_type)
{
    *lc_type = lc_tab[sid];
}

//-----------------------------------------------------------------------------
// Default learning constraints
//-----------------------------------------------------------------------------

/**
 * Gets the default learning constraint set.
 */
void rtu_sw_get_default_lc(int *sid, int *lc_type)
{
    *sid = default_lc_set;
    *lc_type = lc_tab[default_lc_set];
}

/**
 * Sets the default learning constraint set.
 */
int rtu_sw_set_default_lc(int sid)
{
    int i;
    int cur_lc_set = default_lc_set;

    default_lc_set = sid;

    /* Check overall consistency for each pair of vlans */
    for (i = 0; i < NUM_VLANS; i++)
        if (vlan_tab[i].fid || vlan_tab[i].lc_set)
            if (!is_lc_set_consistent(i))
                goto rollback;

    return 0;

rollback:
    default_lc_set = cur_lc_set;
    return -1;
}

/**
 * Sets the default learning constraint set type.
 */
int rtu_sw_set_default_lc_type(int lc_type)
{
    int i;
    int cur_lc_type = lc_tab[default_lc_set];

    lc_tab[default_lc_set] = lc_type;

    /* Check overall consistency for each pair of vlans */
    for (i = 0; i < NUM_VLANS; i++)
        if (vlan_tab[i].fid || vlan_tab[i].lc_set)
            if (!is_lc_set_consistent(i))
                goto rollback;

    return 0;

rollback:
    lc_tab[default_lc_set] = cur_lc_type;
    return -1;
}

//-----------------------------------------------------------------------------
// VID to FID allocation
//-----------------------------------------------------------------------------

/* Allocates the VID to the given FID. If the new allocation is inconsistent
   with those already configured in the Bridge, then the management operation
   shall not be performed and an error response shall be returned. */
int rtu_sw_set_fid(uint16_t vid, uint8_t fid)
{
    vlan_tab[vid].fid = fid;
    vlan_tab[vid].fid_fixed = 1;
    if (!is_lc_set_consistent(vid))
        return -1;
    return 0;
}

void rtu_sw_get_fid(uint16_t vid, uint8_t *fid, int *fid_fixed)
{
    *fid = vlan_tab[vid].fid;
    *fid_fixed = vlan_tab[vid].fid_fixed;
}

int rtu_sw_get_next_fid(uint16_t *vid, uint8_t *fid, int *fid_fixed)
{
    for ((*vid)++; *vid < NUM_VLANS; (*vid)++) {
        if (vlan_tab[*vid].fid) {
            *fid = vlan_tab[*vid].fid;
            *fid_fixed = vlan_tab[*vid].fid_fixed;
            return 0;
        }
    }
    return -1;
}

void rtu_sw_delete_fid(uint16_t vid)
{
    vlan_tab[vid].fid = 0;
    vlan_tab[vid].fid_fixed = 0;
}
