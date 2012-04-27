/*\
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

#include <hw/rtu_regs.h>
#include <hw/trace.h>

#include "rtu_fd.h"
#include "rtu_drv.h"
#include "rtu_hash.h"

// Used to declare memory type at filtering database entry handles.
#define HTAB            0

// Used to declare HW request types.
#define HW_WRITE_REQ    0
#define HW_CLEAN_REQ    1

// Used by HTAB and HCAM lookup methods
#define FOUND               1
#define NOT_FOUND           0
#define NOT_FOUND_AND_FULL  -1
#define NOT_FOUND_AND_FIRST -1



/**
 * \brief Filtering Database entry handle. 
 */
struct fd_handle {
    int mem_type;                       // HTAB or HCAM
    uint16_t addr;                      // zbt_addr or cam_addr
    struct filtering_entry *entry_ptr;  // pointer to entry at mirror fd
};

/**
 * \brief HW (HTAB or HCAM) write request.
 */
struct hw_req {
    int type;                           // WRITE or CLEAN
    struct fd_handle handle;            // filtering database entry handle
    struct hw_req *next;                // linked list
};

/**
 * \brief HW write and clean requests list.
 * Used to temporarily store entry changes performed at SW.
 */
struct hw_req *hw_req_list;

/**
 * \brief Mirror of ZBT SRAM memory MAC address table.
 * Main filtering table organized as hash table with 4-entry buckets.
 * Note both banks have the same content. Therefore SW only mirrors one bank.
 */
static struct filtering_entry rtu_htab[HTAB_ENTRIES][RTU_BUCKETS];

/**
 * \brief Mirror of Aging RAM. 
 */
static uint32_t rtu_agr_htab[RTU_ARAM_WORDS];

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

static struct hw_req *tail(struct hw_req *head);
static void clean_list(struct hw_req *head);
static int add_hw_req(int type, int mem, uint16_t addr, struct filtering_entry *ent);

static inline int write_htab_entry(uint16_t addr, struct filtering_entry *e);

static inline int clean_htab_entry(uint16_t addr);

static inline uint16_t zbt_addr(uint16_t hash, int bucket);

static inline int cam_bucket(uint16_t cam_addr);

static inline int matched(uint32_t word, int offset);

static int htab_contains(uint8_t mac[ETH_ALEN], uint8_t fid, int *bucket, 
                         struct filtering_entry **ent);

static void clean_fd(void);
static void clean_vd(void);

static void clean_aging_map(void);
static void update_aging_map(void);

static void rtu_hw_commit(void);
static void rtu_fd_commit(void);

static void shift_htab_entries(uint16_t hash, int bucket);

static void delete_htab_entry(uint16_t hash, int bucket);

static void rtu_fd_age_out(void);
static void rtu_fd_age_update(void);


/**
 * \brief Initializes the RTU filtering database.
 * @param poly hash polinomial.
 * @param aging aging time
 */
int rtu_fd_init(uint16_t poly, unsigned long aging)
{
    int err;

    TRACE(TRACE_INFO, "clean filtering database.");
    clean_fd();        // clean filtering database
    TRACE(TRACE_INFO, "clean vlan database.");
    clean_vd();        // clean VLAN database
    TRACE(TRACE_INFO, "clean aging map.");
    clean_aging_map(); // clean aging registers
    TRACE(TRACE_INFO, "set aging time [%d].", aging);
    aging_time = aging;

    err = pthread_mutex_init(&fd_mutex, NULL);
    if (err)
        return err;

    TRACE(TRACE_INFO, "set hash poly.");
    rtu_fd_set_hash_poly(poly);

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
int rtu_fd_create_entry(uint8_t mac[ETH_ALEN], uint16_t vid, uint32_t port_map, int dynamic)
{
    struct filtering_entry *ent;            // pointer to scan hashtable
    uint16_t hash;                          // hashtable key            
    uint8_t fid;                            // Filtering database identifier
    int bucket = 0;                         // bucket loop index
    int ret = 0;                            // return value
    uint32_t mask_src, mask_dst;            // used to check port masks update

    pthread_mutex_lock(&fd_mutex);
    // if VLAN is registered (otherwise just ignore request)
    if (! vlan_tab[vid].drop) {
        // Obtain FID from VLAN database
        fid  = vlan_tab[vid].fid;
        hash = rtu_hash(mac, fid);
        // Check HTAB
        ent  = &rtu_htab[hash][bucket];
        ent->hash = (hash << 2) | bucket;
        switch(htab_contains(mac, fid, &bucket, &ent)){
        case FOUND: 
            // update            
            mask_dst = ent->port_mask_dst | port_map;
            mask_src = ent->port_mask_src | vlan_tab[vid].port_mask;
            if ((ent->port_mask_dst != mask_dst) ||
                (ent->port_mask_src != mask_src)) { // something new
                ent->port_mask_dst = mask_dst;
                ent->port_mask_src = mask_src;
                write_htab_entry(zbt_addr(hash, bucket), ent);
            }
            break;
        case NOT_FOUND: // Not found but HTAB still has empty buckets
            ent->valid         = 1;
            ent->fid           = fid;
            ent->port_mask_dst = port_map;
            ent->port_mask_src = vlan_tab[vid].port_mask;
            ent->dynamic       = dynamic;
            ent->last_access_t = now();
            mac_copy(ent->mac, mac);
            write_htab_entry(zbt_addr(hash, bucket), ent);
            break;
        case NOT_FOUND_AND_FULL: // Not found and HTAB full for this hash
            ret = -ENOMEM;
			break;
			/* fixme: allocate an entry in HTAB which is not used by any hash value */
			
        }
    }
    rtu_fd_commit();
    pthread_mutex_unlock(&fd_mutex);
    return ret;
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
void rtu_fd_set_hash_poly(uint16_t poly)
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
int rtu_fd_set_aging_time(unsigned long t)
{
    if ((t < 10) || (t > 1000000))
        return -EINVAL;
    aging_time = t;
    return 0;
}


/**
 * \brief Deletes old filtering entries from filtering database to support
 * changes in active topology.
 */
void rtu_fd_flush(void)
{    
    update_aging_map();     // Work with latest access info
    rtu_fd_age_update();    // Update filtering entries age    

    pthread_mutex_lock(&fd_mutex);
    rtu_fd_age_out();       // Remove old entries    
    pthread_mutex_unlock(&fd_mutex);

    clean_aging_map();      // Keep track of entries access in next period
}

struct filtering_entry *rtu_fd_lookup_htab_entry(int index)
{
	int i, j, n = 0;

	for(i=0;i<RTU_ENTRIES/RTU_BUCKETS;i++) {
		for(j=0;j<RTU_BUCKETS;j++)
		{
			if(rtu_htab[i][j].valid)
			{
				if(n == index) return &rtu_htab[i][j];
				n++;
			}	
		}	
    }		
	return  NULL;
}

//---------------------------------------------
// Static Methods
//---------------------------------------------

/**
 * Returns pointer to last element in hw_req_list.
 */
static struct hw_req *tail(struct hw_req *head){
    struct hw_req *ptr;

    for(ptr = head; ptr->next; ptr = ptr->next);
    return ptr;
}

/**
 * Removes all elements from the hw_req_list
 */
static void clean_list(struct hw_req *head){
    struct hw_req *ptr;

    while(head) {
        ptr  = head;
        head = head->next;
        free(ptr);
    }
}

/**
 * \brief Adds a new HW request at the end of the main hw request list.
 */
static int add_hw_req(int type, int mem, uint16_t addr, struct filtering_entry *ent)
{
    struct hw_req *req;

    req = (struct hw_req*) malloc(sizeof(struct hw_req));
    if(!req)
        return -ENOMEM;
    
    req->type             = type;
    req->handle.mem_type  = mem;
    req->handle.addr      = addr;
    req->handle.entry_ptr = ent;
    req->next             = NULL;

    if(!hw_req_list)
        hw_req_list = req;
    else
        tail(hw_req_list)->next = req;

    return 0;
}

static inline 
int write_htab_entry(uint16_t addr, struct filtering_entry *e)
{
    return add_hw_req(HW_WRITE_REQ, HTAB, addr, e);
}

static inline 
int clean_htab_entry(uint16_t addr)
{
    return add_hw_req(HW_CLEAN_REQ, HTAB, addr, NULL);
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
int matched(uint32_t word, int offset)
{
    return (word >> offset) & 0x00000001;
}

/**
 * \brief Checks whether a given pair (mac,fid) is at HTAB
 * @param mac mac address
 * @param fid filtering database identifier
 * @param bucket inout param.Returns the bucket number where the entry was found
 * @param ent pointer to entry found.
 * @return 0 if entry was not found. 1 if entry was found. -1 if not found and
 * HTAB was full for the corresponding hash. -EINVAL if bucket >= RTU_BUCKETS
 */
static int htab_contains(
        uint8_t mac[ETH_ALEN], 
        uint8_t fid, 
        int *bucket, 
        struct filtering_entry **ent)
{
    for(; *bucket < RTU_BUCKETS; (*bucket)++, (*ent)++) {
        if(!(*ent)->valid)
            return NOT_FOUND;
        if(mac_equal((*ent)->mac, mac) && ((*ent)->fid == fid))
            return FOUND;
        if(*bucket == LAST_RTU_BUCKET)
            return NOT_FOUND_AND_FULL;        
    } 
    return -EINVAL;   
}


/**
 * Filtering database initialisation.
 */
static void clean_fd(void)
{
	memset(&rtu_htab, 0, sizeof(rtu_htab));

    rtu_clean_htab();
}

/**
 * VLAN database initialisation. VLANs are initially marked as disabled. 
 */
static void clean_vd(void)
{
    int i;

    rtu_clean_vlan();
    for(i = 1; i < NUM_VLANS; i++) {
        vlan_tab[i].drop = 1;
    }

    // First entry reserved for untagged packets.
    vlan_tab[0].port_mask       = 0xffffffff;
    vlan_tab[0].drop            = 0;
    vlan_tab[0].fid             = 0;
    vlan_tab[0].has_prio        = 0;
    vlan_tab[0].prio_override   = 0;
    vlan_tab[0].prio            = 0;

    rtu_write_vlan_entry(0, &vlan_tab[0]);
}

/**
 * \brief Clean HCAM aging register and HTAB aging bitmap.
 */
static void clean_aging_map(void)
{
    int i;
    
    for(i = 0; i < RTU_ARAM_WORDS; i++) {
        rtu_agr_htab[i] = 0x00000000;
    }
    rtu_clean_agr_htab();
}

/**
 * \brief Update aging map cache with contents read from aging registers at HW.
 */
static void update_aging_map(void)
{
    int i;

    for(i = 0; i < RTU_ARAM_WORDS; i++) {
        rtu_agr_htab[i] = rtu_read_agr_htab(i);
    }
}

/**
 * \brief Updates the age of filtering entries accessed in the last period. 
 */
static void rtu_fd_age_update(void)
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
    for(i = 0; i < RTU_ARAM_WORDS; i++) {
        agr_word = rtu_agr_htab[i];
        if(agr_word != 0x00000000) {
            for(j = 0; j < 32; j++){
                if(matched(agr_word, j)) {  
                    // ((word_pos x 32) + bit_pos)                  
                    bit_cnt = ((i & 0x00FF) << 5) | (j & 0x001F);   
                    hash    = bit_cnt >> 2;             // 4 buckets per hash
                    bucket  = bit_cnt & 0x03;           // last 2 bits    
 
                    rtu_htab[hash][bucket].last_access_t = t;
                    TRACE(
                        TRACE_INFO, 
                        "updated htab entry age: mac = %s, hash = %d, bucket = %d\n, t = %d", 
                        mac_to_string(rtu_htab[hash][bucket].mac),
                        hash,
                        bucket,
                        t
                    );
                }
            }
        }
    }
}

/**
 * For each filtering entry in the filtering database, this method checks its 
 * last access time and removes it in case entry is older than the aging time. 
 */
static void rtu_fd_age_out(void)
{
    int i;                                      // loop index
    int j;                                      // bucket loop index
    struct filtering_entry *ent;                // pointer to scan tables
    unsigned long t;                            // (secs)

    t = now() - aging_time;
    // HTAB    
    for (i = HTAB_ENTRIES; i-- > 0;) {
        for (j = RTU_BUCKETS; j-- > 0;) {
            ent = &rtu_htab[i][j];
            if(ent->valid && ent->dynamic && time_after(t, ent->last_access_t)){
                TRACE(
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
    // commit changes
    rtu_fd_commit();
}

/**
 * \brief Read changes from hw_req_list and invoke RTU driver to efectively 
 * write or clean the entry.
 */
static void rtu_hw_commit(void)
{
    struct hw_req *req;     // used to scan hw_req_list

    for(req = hw_req_list; req; req = req->next){
        switch(req->type){
        case HW_WRITE_REQ:
            if(req->handle.mem_type == HTAB)
                rtu_write_htab_entry(req->handle.addr, req->handle.entry_ptr);
            break;
        case HW_CLEAN_REQ:
            if(req->handle.mem_type == HTAB)
                rtu_clean_htab_entry(req->handle.addr);
            break;            
        }
    }        
}

/**
 * \brief Commits entry changes at software to hardware HTAB and HCAM.
 */
static void rtu_fd_commit(void)
{
    if(!hw_req_list)
        return;

    rtu_hw_commit();

    // this list no longer needed
    clean_list(hw_req_list);
    hw_req_list = NULL;
}

/**
 * \brief Shifts HTAB list one position, starting at bucket. 
 */
static void shift_htab_entries(uint16_t hash, int bucket)
{
    struct filtering_entry *ent;       // entry to remove
    struct filtering_entry *next_ent;  // following entry
    int i;

    ent = &rtu_htab[hash][bucket];
    for(i = bucket; i < LAST_RTU_BUCKET; i++){
        next_ent = &rtu_htab[hash][i+1];
        if(!next_ent->valid){
            rtu_fe_clean(ent);
            clean_htab_entry(zbt_addr(hash, i));
            break;
        }
        rtu_fe_copy(ent, next_ent);
        write_htab_entry(zbt_addr(hash, i), ent);
        ent = next_ent;
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
    struct filtering_entry *ent;
    

    shift_htab_entries(hash, bucket);

    ent = &rtu_htab[hash][LAST_RTU_BUCKET];
    
    if (ent->valid) {
        // clean last HTAB entry
        rtu_fe_clean(ent);
        clean_htab_entry(zbt_addr(hash, LAST_RTU_BUCKET));
    }
}



