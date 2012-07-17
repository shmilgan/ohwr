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
#include <stdio.h>

#include "mac.h"

#define RTU_ENTRIES 			2048
#define RTU_BUCKETS             4
#define HTAB_ENTRIES            ((RTU_ENTRIES)/(RTU_BUCKETS))
#define LAST_HTAB_ENTRY         ((HTAB_ENTRIES)-1)
#define LAST_RTU_BUCKET 				(RTU_BUCKETS-1)

#define ENTRY_WORDS             8

#define MIN_PORT 0
#define MAX_PORT 17

// Maximum number of supported VLANs
#define NUM_VLANS               4096

#define NUM_RESERVED_ADDR       16

// Default aging time for dynamic entries at filtering database [secs]
#define DEFAULT_AGING_TIME      300
#define MIN_AGING_TIME          10
#define MAX_AGING_TIME          10000
// Default aging time resolution [secs]
#define DEFAULT_AGING_RES       20

// Keeping in mind year 2038
#define time_after(a,b) ((long)(b) - (long)(a) < 0)

#ifdef TRACE_ALL
#define TRACE_DBG(...) TRACE(__VA_ARGS__)
#else
#define TRACE_DBG(...)
#endif


/* RTU entry address */
struct rtu_addr {
  int hash;
  int bucket;
};

/**
 * \brief RTU request: input for the RTU
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
 * \brief RTU Filtering Database Entry Object
 */
struct filtering_entry {
		struct rtu_addr addr;				  // address of self in the RTU hashtable

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

		int force_remove;							// when true, the entry is to be removed immediately (
																	// aged out or destination port went down)

    uint8_t prio_src;             // priority (src MAC)
    int has_prio_src;             // priority value valid
    int prio_override_src;        // priority override (force per-MAC priority)

    uint8_t prio_dst;             // priority (dst MAC)
    int has_prio_dst;             // priority value valid
    int prio_override_dst;        // priority override (force per-MAC priority)

    int dynamic;
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
};




/**
 * \brief Copies src filtering entry body into dst filtering entry body.
 * @return pointer to dst filtering entry.
 */
static inline 
struct filtering_entry *rtu_fe_copy( struct filtering_entry *dst, 
                                     struct filtering_entry *src ) 
{
    return memcpy( dst, src, sizeof(*src) );
}

/**
 * \brief Cleans MAC entry from mirror MAC table. All entry fields are reset.
 * @param ent pointer to entry to clean (either in HCAM or HTAB)
 * @return pointer to filtering entry that was cleaned
 */
static inline 
struct filtering_entry *rtu_fe_clean(struct filtering_entry *ent) 
{
    return memset( ent, 0, sizeof(*ent) );
}

/**
 * \brief Returns number of seconds since the epoch.
 */
static inline 
unsigned long now() 
{
    return (unsigned long) time(NULL); 
}

int rtud_init_exports();
void rtud_handle_wripc();
                                    
#endif /*__WHITERABBIT_RTU_H*/
