/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_rtud v1.0
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *              Maciej Lipinski (maciej.lipinski@cern.ch)
 *
 * Description: RTU data structures definition.
 *
 * Fixes:
 *              Tomasz Wlostowski
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

#ifndef __WHITERABBIT_RTU_H
#define __WHITERABBIT_RTU_H

#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <linux/if_ether.h>
#include <stdio.h>

#include "mac.h"

#define RTU_BANKS               2
#define RTU_BUCKETS             4
#define LAST_RTU_BUCKET         ((RTU_BUCKETS)-1)
#define RTU_ENTRIES             (16384/(RTU_BANKS))
#define HTAB_ENTRIES            ((RTU_ENTRIES)/(RTU_BUCKETS))
#define LAST_HTAB_ENTRY         ((HTAB_ENTRIES)-1)

#define ENTRY_WORDS             8
#define CAM_ENTRIES             (((RTU_HCAM_WORDS)/(ENTRY_WORDS))/(RTU_BANKS))
#define LAST_CAM_ENTRY          ((CAM_ENTRIES)-1)

#define NUM_PORTS               32
#define MIN_PORT                0
#define MAX_PORT                9
#define NIC_PORT                10

// Number of supported VLANs
#define NUM_VLANS               4096
// Reserved VLANS: 0x000 and 0xFFF
#define NUM_RESERVED_VLANS      2
// Maximum VLAN identifier (0xFFF reserved)
#define MAX_VID                 0xFFE

// Default VID for untagged frames
#ifdef V3
#define DEFAULT_VID             0x001
#else
#define DEFAULT_VID             0x000
#endif // V3

// Wildcard VID used by management to refer to any VID
#define WILDCARD_VID            0xFFF
// Number of Filtering database identifiers
#define NUM_FIDS                256

#define NUM_RESERVED_ADDR       16

// Default aging time for dynamic entries at filtering database [secs]
#define DEFAULT_AGING_TIME      300
#define MIN_AGING_TIME          10
#define MAX_AGING_TIME          1000000
// Default aging time resolution [secs]
#define DEFAULT_AGING_RES       20

// Keeping in mind year 2038
#define time_after(a,b) ((long)(b) - (long)(a) < 0)

#ifdef TRACE_ALL
#define TRACE_DBG(...) TRACE(__VA_ARGS__)
#else
#define TRACE_DBG(...)
#endif

// Filtering entries may be static (permanent) or dynamic (learned)
#define STATIC          0
#define DYNAMIC         1
#define STATIC_DYNAMIC  2

/**
 * RTU request: input for the RTU
 */
struct rtu_request {
    int port_id;           // physical port identifier
    uint8_t src[ETH_ALEN]; // source MAC address
    uint8_t dst[ETH_ALEN]; // destination MAC address
    uint16_t vid;          // VLAN ID from the packet header
    int has_vid;           // non-zero: VID is present,0:untagged packet (VID=0)
    uint8_t prio;          // packet priority (either assigned by the port
                           // or extracted from packet header)
    int has_prio;          // non-zero: priority present, 0:no priority defined
};

/**
 * Static Filtering Database Entry Object
 */
struct static_filtering_entry {
    uint8_t mac[ETH_ALEN];                      // MAC address
    uint16_t vid;                               // VLAN ID
    uint32_t egress_ports;                      // port map for dest address
    uint32_t forbidden_ports;                   // 1 = prevents use of dyn info
    int type;		                            // Entry storage type.
    int active;                                 // 0: non-active; 1: active.
    struct static_filtering_entry *next;	    // Double linked list (in
    struct static_filtering_entry *prev;        // lexicographic order)
    struct static_filtering_entry *next_sib;    // Next static entry with the
                                                // same MAC and a VID that maps
                                                // to the same FID
};

/**
 * RTU Filtering Database Entry Object
 */
struct filtering_entry {
    int valid;                    // bit: 1 = entry is valid, 0: entry is
                                  // invalid (empty)
    int end_of_bucket;            // bit: 1 = last entry in current bucket, stop
                                  // search at this point
    int is_bpdu;                  // bit: 1 = BPDU (or other non-STP-dependent
                                  // packet)

    uint8_t mac[ETH_ALEN];        // MAC address (for searching the  bucketed
                                  // hashtable)
    uint8_t fid;                  // Filtering database ID (for searching the
                                  // bucketed hashtable)

    uint32_t port_mask_src;       // port mask for source MAC addresses. Bits
                                  // set to 1 indicate that packet having this
                                  // MAC address can be forwarded from these
                                  // corresponding ports. Ports having their
                                  // bits set to 0 shall drop the packet.

    uint32_t port_mask_dst;       // port mask for destination MAC address. Bits
                                  // set to 1 indicate to which physical ports
                                  // the packet with matching destination MAC
                                  // address shall be routed

    int drop_when_source;         // bit: 1 = drop the packet when source
                                  // address matches
    int drop_when_dest;           // bit: 1 = drop the packet when destination
                                  // address matches
    int drop_unmatched_src_ports; // bit: 1 = drop the packet when it comes from
                                  // source port different than specified in
                                  // port_mask_src

    uint32_t last_access_t;       // time of last access to the rule (for aging)

    uint8_t prio_src;             // priority (src MAC)
    int has_prio_src;             // priority value valid
    int prio_override_src;        // priority override (force per-MAC priority)

    uint8_t prio_dst;             // priority (dst MAC)
    int has_prio_dst;             // priority value valid
    int prio_override_dst;        // priority override (force per-MAC priority)

    int go_to_cam;                // 1 : there are more entries outside the
                                  // bucket

    uint16_t cam_addr;            // address of the first entry in CAM memory
                                  // (2 words)

    int dynamic;                  // 1 = contains dynamic information. An entry
                                  // may contain both, static and dynamic info.

    uint32_t use_dynamic; 	      // Identifies the static and dynamic part of
                                  // the filtering entry
                                  // For static entries, bits set to 1 indicate
                                  // to use dynamic information (in case it
                                  // exists) to determine whether to forward or
                                  // filter (802.1Q 8.8.1).
                                  // Combines information for all the static
                    		      // entries associated to this MAC address and
                                  // FID.
    struct static_filtering_entry *static_fdb;
                                  // static entries with same MAC, and VIDs that
                                  // map to same FID. When static_fdb is not
                                  // NULL, the entry contains static info.
    struct filtering_entry_node *fdb;
                                  // associated node in lex ordered list.
};

/**
 * \brief RTU VLAN registration entry object
 */
struct vlan_table_entry {
    uint32_t port_mask;     // VLAN port mask: 1 = ports assigned to this VLAN
    uint8_t fid;            // Filtering Database Identifier
    uint8_t prio;           // VLAN priority
    int has_prio;           // priority defined;
    int prio_override;      // priority override (force per-VLAN priority)
    int drop;               // 1: drop the packet (VLAN not registered)

    uint32_t use_dynamic; 	// For static entries, bits set to 1 indicate to use
                            // dynamic information to determine the member set
    uint32_t untagged_set;  // Bits set to 1 indicate that frames shall be sent
                            // untagged in the corresponding port
    int dynamic;            // pure static:         0
                            // pure dynamic:        1
                            // dynamic and static:  2
    uint32_t creation_t;    // value of sysUpTime when this VLAN was created.
};


/**
 * \brief Copies src filtering entry body into dst filtering entry body.
 * @return pointer to dst filtering entry.
 */
static inline
struct filtering_entry *rtu_fe_copy( struct filtering_entry *dst,
                                     struct filtering_entry *src )
{
    return memcpy(dst, src, sizeof(*src));
}

/**
 * \brief Cleans MAC entry from mirror MAC table. All entry fields are reset.
 * @param ent pointer to entry to clean (either in HCAM or HTAB)
 * @return pointer to filtering entry that was cleaned
 */
static inline
struct filtering_entry *rtu_fe_clean(struct filtering_entry *ent)
{
    return memset(ent, 0, sizeof(*ent));
}

static inline
struct static_filtering_entry *rtu_sfe_copy( struct static_filtering_entry *dst,
                                             struct static_filtering_entry *src )
{
    return memcpy(dst, src, sizeof(*src));
}

/**
 * \brief Returns number of seconds since the epoch.
 */
static inline
unsigned long now()
{
    return (unsigned long) time(NULL);
}

#endif /*__WHITERABBIT_RTU_H*/
