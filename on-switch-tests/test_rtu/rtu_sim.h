/*
-------------------------------------------------------------------------------
-- Title      : Routing Table Unit Software Simulation
-- Project    : White Rabbit Switch
-------------------------------------------------------------------------------
-- File       : rtu_sim.h
-- Authors    : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Created    : 2010-04-27
-- Last update: 2010-06-30
-- Description: Definitions of data used for simulations
--
--
*/

#ifndef RTU_SIM
#define RTU_SIM

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "rtu_test_main.h"

//#define CAM_ENTRIES 128
// number of entries for each bank
#define CAM_ENTRIES 32

#define MAX_VLANS 4096

#define RTU_ENTRIES 16384
#define RTU_BUCKETS 4

//#define RTU_NUM_PORTS 10
#define RTU_NUM_PORTS PORT_NUMBER

#define MAX_FIFO_SIZE 128

//added by ML
#define ARAM_WORDS 256

#define SIM_POLYNOMIAL_DECT   0x82C48 
#define SIM_POLYNOMIAL_CCITT  0x88108 
#define SIM_POLYNOMIAL_IBM    0xC0028 

#include "rtu_hw.h"
//////////////
#define MAX_REQUEST_NUMBER 1000

typedef uint8_t mac_addr_t [6];


// RTU request: input for the RTU
typedef struct {
  char short_comment[100];
  int port_id;   // physical port identifier
  mac_addr_t src; // source MAC address 
  mac_addr_t dst; // destitation MAC address
  
  uint16_t src_high; // source MAC address
  uint32_t src_low; // source MAC address   
  uint16_t dst_high; // destitation MAC address
  uint32_t dst_low; // destitation MAC address  
  
  uint16_t vid; // VLAN ID from the packet header
  int has_vid; // non-zero: VLAN id is present, 0: untagged packet (VID = 0)
  uint8_t prio; // packet priority (either assigned by the port or extracted from packet header)
  int has_prio; // non-zero: priority present, 0: no priority defined
} rtu_request_t ;

// RTU response: output from the RTU
typedef struct {
  int port_id;
  uint32_t port_mask;  // Port mask: bits set to 1 indicate ports to which the packet must be routed
  int drop; // drop flag: non-zero: drop packet
  uint8_t prio; // final packet priority, assigned by RTU or taken from the port/packet header (depending on the configuration)
} rtu_response_t;


// MAC table entry
typedef struct {
  int valid;         // bit: 1 = entry is valid, 0: entry is invalid (empty)
  int end_of_bucket; // bit: 1 = last entry in current bucket, stop search at this point
  int is_bpdu;       // bit: 1 = BPDU (or other non-STP-dependent packet)
 
  mac_addr_t mac; // MAC address (for searching the  bucketed hashtable)
  uint8_t fid; // Filtering database ID (for searching the bucketed hashtable)


  uint32_t port_mask_src; // port mask for source MAC addresses. Bits set to 1 indicate that 
  // packet having this MAC address can be forwarded from these corresponding ports. Ports having
  // their bits set to 0 shall drop the packet.

  uint32_t port_mask_dst; // port mask for destination MAC address. Bits set to 1 indicate to which physical ports the packet with matching destination
  // MAC address shall be routed

  int drop_when_source; // bit: 1 = drop the packet when source address matches
  int drop_when_dest; // bit: 1 = drop the packet when destination address matches
  int drop_unmatched_src_ports; //bit: 1 = drop the packet when it comes from source port different than specified in port_mask_src 

  uint32_t last_access_t; // time of last access to the rule (for aging)

  uint8_t prio_src; // priority (src MAC)
  int has_prio_src; // priority value valid
  int prio_override_src; // priority override (force per-MAC priority)

  uint8_t prio_dst; // priority (dst MAC)
  int has_prio_dst; // priority value valid
  int prio_override_dst; // priority override (force per-MAC priority)

  int go_to_cam; // 1 : there are more entries outside the bucket

  uint8_t cam_addr   ; // address of the first entry in CAM memory
} mac_table_entry_t;

typedef struct {
  uint32_t port_mask; // VLAN port mask: 1 = ports assigned to thisVLAN
  uint8_t fid; // Filtering Database Index 
  uint8_t prio; // VLAN priority
  int has_prio; // priority defined;
  int prio_override; // priority override (force per-VLAN priority)
  int drop; // 1: drop the packet (VLAN not registered)
} vlan_table_entry_t;

typedef struct {
  int forward_bpdu_only; 	// forward BPDU/PTP packets only (when the port is in BLOCKING or LEARNING state)
				// works regardless of **pass_all**  value - forward_bpdu_only overrides it 
				// it allows only bpdu packages to be passed
			
  int learning_enabled; 	// enable learning
  int pass_all;			// in other words: enable this port
  //int pass_bpdu; 		// BPDU packets (with dst MAC 01:80:c2:00:00:00) are passed according to RT rules. This setting overrides 
  int fixed_prio_ena; 		//fixed prio set on this port
  int fixed_prio_val; 		//value of the fixed prio
  int b_unrec; 			//Unrecognized request behaviour
				//Sets the port behaviour for all unrecognized requests:
				//  0: packet is dropped
				//  1: packet is broadcast 
} rtu_port_config_t;

typedef struct {

  int rtu_bank; // active MAC table bank
  int cam_bank; // active CAM bank

  int global_enable; // global enable of RTU
  uint32_t hash_poly;           //POLY_VAL [read/write]: Hash Poly
				//Determines the polynomial used for hash computation. 
				 //Currently available: 0x1021, 0x8005, 0x0589 
  rtu_port_config_t port_cfg[RTU_NUM_PORTS];

} rtu_config_t;

typedef struct {

  // MAC address table, organized as hash table with 4-entry buckets.
  mac_table_entry_t buckets[RTU_ENTRIES/4][4];

} mac_table_t;



typedef struct {
  rtu_request_t data[MAX_FIFO_SIZE];
  int head, tail, count;
} request_fifo_t;

///// these variables must be accessible (RW) from the Wishbone

// MAC address table (2 banks)
mac_table_t rtu_tab[2];

// CAM lookup table (2 banks) - for RTU entries with more than 4 matches
// for the same hash value
mac_table_entry_t rtu_cam[2][CAM_ENTRIES];

// VLAN table
vlan_table_entry_t vlan_tab[MAX_VLANS];

request_fifo_t learning_queue;

rtu_config_t CFG;

//added by ML
uint32_t rtu_agr_htab[ARAM_WORDS];
uint32_t rtu_agr_hcam;


typedef struct {
  uint32_t word;
  int address;
} changed_aging_htab_word_t;

int number_of_found_entries;
int hash_addresses[RTU_ENTRIES];
int hash_address_writen_cnt;

// nasty global for number of entries writen 
// to hcam (separately for each bank
int cam_address[2];

#endif