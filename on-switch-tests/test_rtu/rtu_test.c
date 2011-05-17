/*
-------------------------------------------------------------------------------
-- Title      : Routing Table Unit Test Interface
-- Project    : White Rabbit Switch
-------------------------------------------------------------------------------
-- File       : rtu_test.c
-- Authors    : Maciej Lipinski (maciej.lipinski@cern.ch)
-- Company    : CERN BE-CO-HT
-- Created    : 2010-06-30
-- Last update: 2010-06-30
-- Description: This file stores entire test interface, which includes:
--               - functions used of data entry (VLAN, HCAM, HTAB, CONFIG) in both: 
--                 simulation and hardware
--               - functions used for request management


*/

#include <stdio.h>
#include <hw/switch_hw.h>
#include <hw/clkb_io.h>
#include "rtu_sim.h"
#include <math.h>
#include "rtu_test_main.h"
#include "rtu_hw.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//				FUNCTIONS USED FOR DATA ENTRY (VLAN, HCAM, HTAB, CONFIG)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
set active ZBT bank (0 or 1)
*/
int rtu_set_active_htab_bank(uint8_t bank)
{
    
    if(rtu_sim_set_active_htab_bank(bank) < 0) return -1;
    if(rtu_hw_set_active_htab_bank(bank) < 0) return -1;
    printf("set htab_bank: %d\n", bank);
    return 1;
}
/*
set active CAM bank (0 or 1)
*/
int rtu_set_active_hcam_bank(uint8_t bank)
{
    
    if(rtu_sim_set_active_hcam_bank(bank) < 0) return -1;
    if(rtu_hw_set_active_hcam_bank(bank) < 0) return -1;
    printf("set hcam_bank: %d\n", bank);
    return 1;
}


/*
setting: 
# FIX_PRIO [read/write]: Fix priority
1: Port has fixed priority of value PRIO_VAL. It overrides the priority coming from the endpoint
0: Use priority from the endpoint
# PRIO_VAL [read/write]: Priority value
Fixed priority value for the port. Used instead the endpoint-assigned priority when FIX_PRIO = 1 
*/
int rtu_set_fixed_prio_on_port(int port, uint8_t prio, uint8_t set)
{
    rtu_sim_set_fixed_prio_on_port(port, prio,set);
    rtu_hw_set_fixed_prio_on_port(port, prio,set);
}
/*
Set basic RTU configuration
    * HT_BSEL [read/write]: Main table bank select
      Selects active bank of RTU hashtable (ZBT).
      0: bank 0 is used by lookup engine and bank 1 can be accessed using MFIFO
      1: bank 1 is used by lookup engine and bank 0 can be accessed using MFIFO
    * HCAM_BSEL [read/write]: Hash collision table (HCAM) bank select
      Selects active bank of RTU extra memory for colliding hashes.
      0: bank 0 is used by lookup engine
      1: bank 1 is used by lookup engine
    * G_ENA [read/write]: RTU Global Enable
      Global RTU enable bit. Overrides all port settings.
      0: RTU is disabled. All packets are dropped.
      1: RTU is enabled. 
     *POLY_VAL [read/write]: Hash Poly
      Determines the polynomial used for hash computation. Currently available: 0x1021, 0x8005, 0x0589 
*/
int rtu_setting(uint8_t global_enable,uint8_t hcam_bank,uint8_t htab_bank, uint16_t hash_poly)
{
  uint32_t sim_hash_poly;
  
  rtu_hw_rtu_setting(global_enable,hcam_bank,htab_bank,hash_poly);
  
  //translate hash
       if(hash_poly == HW_POLYNOMIAL_DECT)  sim_hash_poly = SIM_POLYNOMIAL_DECT;
  else if(hash_poly == HW_POLYNOMIAL_CCITT) sim_hash_poly = SIM_POLYNOMIAL_CCITT;
  else if(hash_poly == HW_POLYNOMIAL_IBM)   sim_hash_poly = SIM_POLYNOMIAL_IBM;
  else                                      sim_hash_poly = SIM_POLYNOMIAL_CCITT;
  
  rtu_sim_rtu_setting(global_enable,hcam_bank,htab_bank,sim_hash_poly);
}

int rtu_set_hash_poly(uint16_t hash_poly)
{
  uint32_t sim_hash_poly;
  rtu_hw_set_hash_poly(hash_poly);
  
       if(hash_poly == HW_POLYNOMIAL_DECT)  sim_hash_poly = SIM_POLYNOMIAL_DECT;
  else if(hash_poly == HW_POLYNOMIAL_CCITT) sim_hash_poly = SIM_POLYNOMIAL_CCITT;
  else if(hash_poly == HW_POLYNOMIAL_IBM)   sim_hash_poly = SIM_POLYNOMIAL_IBM;
  else                                      sim_hash_poly = SIM_POLYNOMIAL_CCITT;
  
  rtu_sim_set_hash_poly(sim_hash_poly);
  
}

/*
Set basic RTU configuration
for a given port, possible settings:
    * LEARN_EN [read/write]: Learning enable
      1: enables learning process on this port. Unrecognized requests will be put into UFIFO
      0: disables learning. Unrecognized requests will be either broadcast or dropped.
    * PASS_ALL [read/write]: Pass all packets
      1: all packets are passed (depending on the rules in RT table).
      0: all packets are dropped on this port.
    * PASS_BPDU [read/write]: Pass BPDUs
      1: BPDU packets (with dst MAC 01:80:c2:00:00:00) are passed according to RT rules. This setting overrides PASS_ALL.
      0: BPDU packets are dropped.
    * FIX_PRIO [read/write]: Fix priority
      1: Port has fixed priority of value PRIO_VAL. It overrides the priority coming from the endpoint
      0: Use priority from the endpoint
    * PRIO_VAL [read/write]: Priority value
      Fixed priority value for the port. Used instead the endpoint-assigned priority when FIX_PRIO = 1
    * B_UNREC [read/write]: Unrecognized request behaviour
      Sets the port behaviour for all unrecognized requests:
      0: packet is dropped
      1: packet is broadcast   
*/
int rtu_port_setting(int port, uint32_t setting)
{
  rtu_sim_rtu_port_setting(port,setting);
  rtu_hw_rtu_port_setting(port,setting);

}

// /*
// basic settings
// */
// int rtu_set()
// {
// 	int i;
// 	// global settings
// 	rtu_setting( 1 /*global_enable*/, \
// 	             0 /*hcam_bank*/    , \ 
// 	             0 /*htab_bank*/     );
// 	//settings for each port: enable ports and learning on ports
// 	for(i=0;i<PORT_NUMBER;i++)
// 	   rtu_port_setting(i, FPGA_RTU_PCR_LEARN_EN | FPGA_RTU_PCR_PASS_ALL);
//     
//   
//   
//   //rtu_sim_set();
//   //rtu_hw_set();
// 
// }
/*
write hcam entry.
the problem of writing data to hcam because there is no free bucket etc. needs to be handled by hand. it is just test programm


*/
rtu_write_hcam_entry(int   valid,         int      end_of_bucket,    int     is_bpdu,        int go_to_cam , \
		uint8_t  fid,           uint16_t mac_hi,           uint32_t mac_lo,  \
		uint8_t  cam_addr,      int      drop_when_source, int      drop_when_dest, int drop_when_unmatched_src_ports, \
		uint8_t  prio_src,      uint8_t  has_prio_src,     uint8_t  prio_override_src, \
		uint8_t  prio_dst,      int      has_prio_dst,     int      prio_override_dst, \
                uint16_t port_mask_src, uint16_t port_mask_dst,    uint32_t last_access ,   uint16_t hcam_addr)
{


  //write soft
  rtu_sim_write_hcam_entry(valid,end_of_bucket,is_bpdu,go_to_cam , fid,mac_hi,mac_lo, cam_addr, drop_when_source, drop_when_dest, drop_when_unmatched_src_ports, \
		           prio_src, has_prio_src, prio_override_src, prio_dst, has_prio_dst, prio_override_dst,port_mask_src, port_mask_dst, last_access , \
		           hcam_addr,0);  
  //write hardware			   
  rtu_hw_write_hcam_entry(valid,end_of_bucket,is_bpdu,go_to_cam , fid,mac_hi,mac_lo, cam_addr, drop_when_source, drop_when_dest, drop_when_unmatched_src_ports, \
		           prio_src, has_prio_src, prio_override_src, prio_dst, has_prio_dst, prio_override_dst,port_mask_src, port_mask_dst, last_access , \
		           hcam_addr); 

}
/*
write htab entry. The address is calculated based on MAC and FID stored in the entry

*/

int rtu_write_htab_entry(int      valid,         int      end_of_bucket,    int     is_bpdu,        int go_to_cam , \
		         uint8_t  fid,           uint16_t mac_hi,           uint32_t mac_lo,  \
		         uint8_t  cam_addr,      int      drop_when_source, int      drop_when_dest, int drop_when_unmatched_src_ports, \
		         uint8_t  prio_src,      uint8_t  has_prio_src,     uint8_t  prio_override_src, \
		         uint8_t  prio_dst,      int      has_prio_dst,     int      prio_override_dst, \
                         uint16_t port_mask_src, uint16_t port_mask_dst,    uint32_t last_access /*,  int bank */, int bucket)
{
 
  uint32_t zbt_addr; 
  uint32_t htab_addr;
  int i,j;
  int all_buckets_full = 1;
  uint16_t p_hcam_addr;
  int hash_and_cam_full = 0;
  int ret = 0; // 0 - normal, 1- writtent o CAM
  /*if the bucket is defined, it means that it should be like that, otherwise we:
  1) check if the bucket is free
  2) write to first free bucket
  3) write to cam if all 4 buckets are full
  
  */
  
  int htab_bank;
  if(CFG.rtu_bank == 1 )      htab_bank = 0;
  else if(CFG.rtu_bank == 0 ) htab_bank = 1;
  else                            return -1;

  int hcam_bank;
  if(CFG.cam_bank == 1 )      hcam_bank = 0;
  else if(CFG.cam_bank == 0 ) hcam_bank = 1;
  else                            return -1;


  if(bucket == 0) // this is what we want, the other case should not be used
  {
    //find empty bucket
    
    for(i=0;i<RTU_BUCKETS;i++)
    {
      //printf("looking for empty bucket\n");
      //hw address
      zbt_addr = zbt_addr_get(/*bank,*/ i, mac_hi, mac_lo, fid);
      
      //sw address
      htab_addr = zbt_addr >> 5;
        
      if( rtu_tab[htab_bank].buckets[htab_addr][i].valid == 0)
      {
	//printf("empty bucket found: %d\n",i);
	//found empty bucket
	bucket = i;
	all_buckets_full=0;
	break;
      }
    
    }
    // no places in bucket, we need to write to cam but first we check whether there is enough in cam
    
    if(all_buckets_full == 1  ) 
    { 
          //there is space in CAM
	  if(cam_address[hcam_bank] < 8 * CAM_ENTRIES)
	  {
	      //for convenience
	      bucket = RTU_BUCKETS - 1 ; //set the last bucket since we may need to change the last bucket in htab
	      
	      #ifdef DEBUG_HTAB
	      printf("all bukcets full, writing to hcam\n"); 
	      #endif
	      end_of_bucket = 0x1;
	      go_to_cam     - 0x0;
	      rtu_write_hcam_entry(valid,end_of_bucket,is_bpdu,go_to_cam,fid,mac_hi,mac_lo,cam_addr, drop_when_source,\
				  drop_when_dest,drop_when_unmatched_src_ports,prio_src, has_prio_src, prio_override_src, \
				  prio_dst,has_prio_dst,prio_override_dst,port_mask_src, port_mask_dst, last_access , cam_address[hcam_bank] );
	      
	      ret = 1;
	      // if this is not the beginning of cam memory, then we need to zero end_of_bucket in the last entry since we
	      // are adding another entry
	      
	      if( (cam_address[hcam_bank]-8) >= 0)
	      {
	      
		  //in simulation
		  p_hcam_addr = (cam_address[hcam_bank]-8)/8;
		  rtu_cam[hcam_bank][p_hcam_addr].end_of_bucket=0x0 ;
		  
		  //in hw
		  uint16_t mac_hi = ( rtu_cam[hcam_bank][p_hcam_addr].mac[0] << 8  ) | (rtu_cam[hcam_bank][p_hcam_addr].mac[1]   );
		  uint32_t mac_lo = ( rtu_cam[hcam_bank][p_hcam_addr].mac[2] << 24 ) | (rtu_cam[hcam_bank][p_hcam_addr].mac[3] << 16 ) |\
				    ( rtu_cam[hcam_bank][p_hcam_addr].mac[4] << 8  ) | (rtu_cam[hcam_bank][p_hcam_addr].mac[5] );

		  
		  rtu_write_hcam_entry(rtu_cam[hcam_bank][p_hcam_addr].valid,				\
				      0x0 /*end_of_bucket*/, 					\
				      rtu_cam[hcam_bank][p_hcam_addr].is_bpdu,				\
				      0x0 /*go_to_cam*/,						\
				      rtu_cam[hcam_bank][p_hcam_addr].fid,				\
				      mac_hi,				\
				      mac_lo,				\
				      rtu_cam[hcam_bank][p_hcam_addr].cam_addr, 				\
				      rtu_cam[hcam_bank][p_hcam_addr].drop_when_source,			\
				      rtu_cam[hcam_bank][p_hcam_addr].drop_when_dest,			\
				      rtu_cam[hcam_bank][p_hcam_addr].drop_unmatched_src_ports,		\
				      rtu_cam[hcam_bank][p_hcam_addr].prio_src, 				\
				      rtu_cam[hcam_bank][p_hcam_addr].has_prio_src,			\
				      rtu_cam[hcam_bank][p_hcam_addr].prio_override_src, 			\
				      rtu_cam[hcam_bank][p_hcam_addr].prio_dst,				\
				      rtu_cam[hcam_bank][p_hcam_addr].has_prio_dst,			\
				      rtu_cam[hcam_bank][p_hcam_addr].prio_override_dst,			\
				      rtu_cam[hcam_bank][p_hcam_addr].port_mask_src, 			\
				      rtu_cam[hcam_bank][p_hcam_addr].port_mask_dst, 			\
				      rtu_cam[hcam_bank][p_hcam_addr].last_access_t , 			\
				      (cam_address[hcam_bank] -8) );
	      }
	  

	      
	      // if this is the first cam entry associated with this hash,
	      // we need to indicate in htab that lookup needs to search in hcam
	      if(rtu_tab[htab_bank].buckets[htab_addr][bucket].go_to_cam == 0)
	      {
		  // in simulation:
		  rtu_tab[htab_bank].buckets[htab_addr][bucket].go_to_cam = 0x1;
		  rtu_tab[htab_bank].buckets[htab_addr][bucket].cam_addr = (uint16_t)(cam_address[hcam_bank]/8);
		  #ifdef DEBUG_HTAB
		  printf("\nrtu_tab[%d][0x%x][0x%x].cam_addr = %d / 8 =  %d\n",htab_bank,htab_addr,bucket,(uint16_t)cam_address[hcam_bank],(uint16_t)cam_address[hcam_bank]/8);
		  #endif
		  // in hardware:
		    uint16_t mac_hi = (rtu_tab[htab_bank].buckets[htab_addr][bucket].mac[0] << 8  ) | \
				      (rtu_tab[htab_bank].buckets[htab_addr][bucket].mac[1]   );
		    uint32_t mac_lo = (rtu_tab[htab_bank].buckets[htab_addr][bucket].mac[2] << 24 ) | \
				      (rtu_tab[htab_bank].buckets[htab_addr][bucket].mac[3] << 16 ) | \
				      (rtu_tab[htab_bank].buckets[htab_addr][bucket].mac[4] << 8  ) | \
				      (rtu_tab[htab_bank].buckets[htab_addr][bucket].mac[5] );
				      
		  rtu_hw_write_htab_entry(	rtu_tab[htab_bank].buckets[htab_addr][bucket].valid  ,			\
					      0x0, /*end_of_bucket*/							\
					      rtu_tab[htab_bank].buckets[htab_addr][bucket].is_bpdu,			\
					      0x1, /*go_to_cam*/							\
					      rtu_tab[htab_bank].buckets[htab_addr][bucket].fid, 			\
					      mac_hi, 								\
					      mac_lo, 								\
					      cam_address[hcam_bank],/*write addr*/						\
					      rtu_tab[htab_bank].buckets[htab_addr][bucket].drop_when_source, 	\
					      rtu_tab[htab_bank].buckets[htab_addr][bucket].drop_when_dest, 		\
					      rtu_tab[htab_bank].buckets[htab_addr][bucket].drop_unmatched_src_ports, \
					      rtu_tab[htab_bank].buckets[htab_addr][bucket].prio_src, 		\
					      rtu_tab[htab_bank].buckets[htab_addr][bucket].has_prio_src, 		\
					      rtu_tab[htab_bank].buckets[htab_addr][bucket].prio_override_src, 	\
					      rtu_tab[htab_bank].buckets[htab_addr][bucket].prio_dst, 		\
					      rtu_tab[htab_bank].buckets[htab_addr][bucket].has_prio_dst, 		\
					      rtu_tab[htab_bank].buckets[htab_addr][bucket].prio_override_dst, 	\
					      rtu_tab[htab_bank].buckets[htab_addr][bucket].port_mask_src, 		\
					      rtu_tab[htab_bank].buckets[htab_addr][bucket].port_mask_dst, 		\
					      rtu_tab[htab_bank].buckets[htab_addr][bucket].last_access_t, 		\
					      zbt_addr); 	     
	      
	      }
		      //increment global variable storing cam addre
	      cam_address[hcam_bank] = cam_address[hcam_bank] + 8;
	  
	  }
	  // there is no space in CAM
	  // in general, just don't learn/remember
	  else 
	  {
	   // printf("\n\n\n");
	   // printf("============================\n");
	   //  printf("      NOT IMPLEMENTED\n");
	   //  printf("       CAUSE ERRORS\n");
	   //  printf("         (cam full)\n");
	   //  printf("============================\n");
	   //  printf("\n\n\n");
	    hash_and_cam_full = 1;
	  }
    }
    else // there is place in one of four buckets
    {
	// it is important to clear thsi if entry placed in hcam
	go_to_cam = 0x0;
	cam_addr = 0x0;
	end_of_bucket = 0x0;
	hash_addresses[hash_address_writen_cnt++] = zbt_addr;

	rtu_sim_write_htab_entry(valid,end_of_bucket,is_bpdu,go_to_cam , fid,mac_hi,mac_lo, cam_addr, drop_when_source, drop_when_dest, drop_when_unmatched_src_ports, \
				  prio_src, has_prio_src, prio_override_src, prio_dst, has_prio_dst, prio_override_dst,port_mask_src, port_mask_dst, last_access , \
				  zbt_addr /*, bank */, bucket);
			   
	rtu_hw_write_htab_entry(valid,end_of_bucket,is_bpdu,go_to_cam , fid,mac_hi,mac_lo, cam_addr, drop_when_source, drop_when_dest, drop_when_unmatched_src_ports, \
				  prio_src, has_prio_src, prio_override_src, prio_dst, has_prio_dst, prio_override_dst,port_mask_src, port_mask_dst, last_access , \
				  zbt_addr);  
    
    }
  
   
  
  
  }
  // bucket != 0 : for some reason bucket number was defined "by hand", so use it, don't bother any checks
  else
  {
  
    hash_addresses[hash_address_writen_cnt++] = zbt_addr;
    rtu_sim_write_htab_entry(valid,end_of_bucket,is_bpdu,go_to_cam , fid,mac_hi,mac_lo, cam_addr, drop_when_source, drop_when_dest, drop_when_unmatched_src_ports, \
			    prio_src, has_prio_src, prio_override_src, prio_dst, has_prio_dst, prio_override_dst,port_mask_src, port_mask_dst, last_access , \
			    zbt_addr/*, bank */, bucket);
			   
    rtu_hw_write_htab_entry(valid,end_of_bucket,is_bpdu,go_to_cam , fid,mac_hi,mac_lo, cam_addr, drop_when_source, drop_when_dest, drop_when_unmatched_src_ports, \
			    prio_src, has_prio_src, prio_override_src, prio_dst, has_prio_dst, prio_override_dst,port_mask_src, port_mask_dst, last_access , \
			    zbt_addr);  			   
			   
  }
  #ifdef DEBUG_HTAB
  int bank;
  uint16_t hs = hash(mac_hi, mac_lo, fid);
  fprintf(stderr, "\n");
  if(hash_and_cam_full == 1)
  {
    fprintf(stderr, "|=============== cam & htab full @hash = 0x%x======================================|\n",zbt_addr,hs);
    bank = -1;
  }
  else if(all_buckets_full==0) // writing to htab
  {
    fprintf(stderr, "|=============== zbt_addr = 0x%x   hash = 0x%x=====================================|\n",zbt_addr,hs);
    bank = htab_bank;
  }
  else
  {
    fprintf(stderr, "|=============== cam_addr = 0x%x   hash = 0x%x=====================================|\n",cam_address[hcam_bank] - 8,hs);
    bank = hcam_bank;
  }
  fprintf(stderr, "|valid = 0x%x              | end_of_bucket = 0x%x          | is_bpdu = 0x%x              |\n",valid,end_of_bucket,is_bpdu);
  fprintf(stderr, "|fid              = 0x%4x|mac           = 0x%4x%8x| cam_addr= 0x%2x             |\n",fid,mac_hi,mac_lo,cam_addr);
  fprintf(stderr, "|drop_when_source = 0x%x   |drop_when_dest= 0x%x           |dp_w_unmatc_s_p = 0x%x       | \n",drop_when_source,drop_when_dest,drop_when_unmatched_src_ports);
  fprintf(stderr, "|prio_src         = 0x%x   |has_prio_src  = 0x%x           |prio_oride_src  = 0x%x       |\n",prio_src,has_prio_src,prio_override_src); 
  fprintf(stderr, "|prio_dst         = 0x%x   |has_prio_dst  = 0x%x           |prio_oride_dst  = 0x%x       |\n",prio_dst,has_prio_dst,prio_override_dst); 
  fprintf(stderr, "|port_mask_src    = 0x%4x|port_mask_dst = 0x%4x        |last_access     = 0x%8x|\n",port_mask_src,port_mask_dst,last_access);
  fprintf(stderr, "|bank             = 0x%x   |bucket        = 0x%x           | go_to_cam = 0x%x            |\n",bank,bucket,go_to_cam);
  fprintf(stderr, "|^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^|\n");
  #endif
  return ret;	   
}

/*
write htab entry. The address is calculated based on different MAC and FID than stored in the entry.
it is used to write entry which is not to be found (only for test purpose)
*/

int rtu_write_htab_entry_cheat(	int      valid,         int      end_of_bucket,    int     is_bpdu,        int go_to_cam , \
				uint8_t  fid,           uint16_t mac_hi,           uint32_t mac_lo,  \
				uint8_t  cam_addr,      int      drop_when_source, int      drop_when_dest, int drop_when_unmatched_src_ports, \
				uint8_t  prio_src,      uint8_t  has_prio_src,     uint8_t  prio_override_src, \
				uint8_t  prio_dst,      int      has_prio_dst,     int      prio_override_dst, \
				uint16_t port_mask_src, uint16_t port_mask_dst,    uint32_t last_access /*,  int bank */, int bucket, \
				uint8_t  addr_fid,      uint16_t addr_mac_hi,      uint32_t addr_mac_lo)
{
  
  //get the address using CRC16
  uint32_t zbt_addr = zbt_addr_get(/*bank,*/ bucket, addr_mac_hi,addr_mac_lo, addr_fid);
  
  //write software
  rtu_sim_write_htab_entry(valid,end_of_bucket,is_bpdu,go_to_cam , fid,mac_hi,mac_lo, cam_addr, drop_when_source, drop_when_dest, drop_when_unmatched_src_ports, \
		           prio_src, has_prio_src, prio_override_src, prio_dst, has_prio_dst, prio_override_dst,port_mask_src, port_mask_dst, last_access , \
		           zbt_addr/*, bank */, bucket);  
	
  //write hardware
  rtu_hw_write_htab_entry(valid,end_of_bucket,is_bpdu,go_to_cam , fid,mac_hi,mac_lo, cam_addr, drop_when_source, drop_when_dest, drop_when_unmatched_src_ports, \
		           prio_src, has_prio_src, prio_override_src, prio_dst, has_prio_dst, prio_override_dst,port_mask_src, port_mask_dst, last_access , \
		           zbt_addr); 			   
}

/*
write vlan entry

*/
rtu_write_vlan_entry(int addr, uint32_t port_mask,uint8_t fid,int has_prio,uint8_t prio,int prio_override,int drop)
{

  // write soft
  rtu_sim_write_vlan_entry(addr, port_mask,fid,prio,has_prio,prio_override,drop);
  
  //write hardware
  rtu_hw_write_vlan_entry(addr, port_mask,fid,prio,has_prio,prio_override,drop);

#ifdef DEBUG_VLAN
  fprintf(stderr, "\n");
  fprintf(stderr, "|===============  addr = 0x%x   ==========================|\n",addr);
  fprintf(stderr, "|port_mask = 0x%4x | fid           = 0x%4x | prio = 0x%x  |\n",port_mask,fid,prio);
  fprintf(stderr, "|has_prio  = 0x%x    | prio_override = 0x%x   | drop = 0x%x   |\n",has_prio,prio_override,drop);
  fprintf(stderr, "|^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^|\n");
#endif
  
}


int rtu_read_arg_hcam(uint32_t *sim_agr_hcam, uint32_t *hw_gr_hcam)
{

  *hw_gr_hcam   = rtu_hw_read_arg_hcam();
  *sim_agr_hcam = rtu_agr_hcam;
  //printf("not implemented for simulation: rtu_read_arg_hcam()\n");
  return 1;

}

int rtu_write_agr_htab(uint8_t addr, uint32_t data)
{
   rtu_hw_write_agr_htab(addr, data);
   printf("not implemented for simulation: rtu_write_agr_htab()\n");
}

int rtu_read_arg_htab_changes(changed_aging_htab_word_t sim_agr_htab[],changed_aging_htab_word_t hw_agr_htab[])
{
   int i;
   int hw_cnt, sim_cnt;
   uint32_t tmp;
   
   hw_cnt=0;
   
   // remembering changed words from agr_htab memory in hardware
   for(i=0;i<256;i++)
   {
    tmp=rtu_hw_read_agr_htab(i);
    if(tmp != 0x00000000)
    {
      //fprintf(stderr, "agr_htab[0x%x] = 0x%8x\n",i,tmp); 
      hw_agr_htab[hw_cnt].address = i;
      hw_agr_htab[hw_cnt].word = tmp;
      hw_cnt++;
    }
  }
  
  // remembering changed words from agr_htab memory in simulation
  sim_cnt=0;
  for(i=0;i<256;i++)
   {
    
    if(rtu_agr_htab[i] != 0x00000000)
    {
      //fprintf(stderr, "agr_htab[0x%x] = 0x%8x\n",i,rtu_agr_htab[i]); 
      sim_agr_htab[sim_cnt].address = i;
      sim_agr_htab[sim_cnt].word = rtu_agr_htab[i];
      sim_cnt++;
    }
  }
  
  printf("HARDWARE  : numbers of agr_htab words changed: %d\n",hw_cnt);
  printf("SIMULATION: numbers of agr_htab words changed: %d\n",sim_cnt);
  
  if(hw_cnt>sim_cnt)
    return hw_cnt;
  else
    return sim_cnt;
}

/*
clean memories
*/
int rtu_clean_mems(int active_htab_bank,int active_hcam_bank)
{
  cam_address[0] = 0;
  cam_address[1] = 0;
  // do in software
  rtu_sim_clean_mems();
  
  // do in hardware
   rtu_hw_clean_mems(active_htab_bank,active_hcam_bank);
  return 1;
}

#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//				FUNCTIONS USED FOR REQUEST COMPOSITION
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

rtu_write_request_entry_to_req_table(rtu_request_t *rq, int port,uint16_t smac_hi, uint32_t smac_lo, uint16_t dmac_hi,uint32_t dmac_lo,  uint16_t vid, uint8_t prio ,uint8_t has_vid, uint8_t has_prio,char *comment )
{

    strcpy(rq->short_comment,comment);
    rq->port_id  = port;
    
    rq->src[0] = (smac_hi >> 8);
    rq->src[1] = (0xFF & smac_hi);
    rq->src[2] = (0xFF & (smac_lo >> 24));
    rq->src[3] = (0xFF & (smac_lo >> 16));
    rq->src[4] = (0xFF & (smac_lo >> 8));
    rq->src[5] = (0xFF & smac_lo) ; 
   
    rq->dst[0] = (dmac_hi >> 8);
    rq->dst[1] = (0xFF & dmac_hi);
    rq->dst[2] = (0xFF & (dmac_lo >> 24));
    rq->dst[3] = (0xFF & (dmac_lo >> 16));
    rq->dst[4] = (0xFF & (dmac_lo >> 8));
    rq->dst[5] = (0xFF & dmac_lo) ;     
    
    rq->src_high = smac_hi;
    rq->src_low  = smac_lo;
    rq->dst_high = dmac_hi;
    rq->dst_low  = dmac_lo;
    rq->vid      = vid;
    rq->has_vid  = has_vid;
    rq->prio     = prio;
    rq->has_prio = has_prio;
    
#ifdef DEBUG_REQUEST
  fprintf(stderr, "\n");
  fprintf(stderr, "|===============  port = %2d   =============================|\n",port);
  fprintf(stderr, "|smac =   0x%4x%8x    | dmac =    0x%4x%8x     |\n",smac_hi,smac_lo,dmac_hi,dmac_lo);
  fprintf(stderr, "|----------------------------------------------------------|\n");
  fprintf(stderr, "|vid  = 0x%2x |has_vid = 0x%x | prio = 0x%x | has_prio = 0x%x  |\n",vid,has_vid, prio, has_prio);
  fprintf(stderr, "|^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^|\n");
#endif    
    
}


rtu_printf_entry_from_req_table(rtu_request_t rq)
{

	
	printf("\tDst MAC : 0x%x%8x\n", 0xFFFF & rq.dst_high,rq.dst_low);
	printf("\tSrc MAC : 0x%x%8x\n", 0xFFFF & rq.src_high,rq.src_low);
	if(rq.has_prio)
	  printf("\tHas VID : 0x%x\n", rq.vid);
	else
	  printf("\tNO valid VID\n");
	if(rq.has_vid)
	  printf("\tHas PRIO: 0x%x\n", rq.prio);
	else
	  printf("\tNO valid PRIO\n");
	printf("===============================\n");


}

rtu_printf_entry_from_resp_table(rtu_response_t resp)
{


	
	if(0x1 & resp.drop)
	  printf("\tDROP\n");
	else
	  printf("\tDO NOT DROP\n");
 
	printf("\tMASK: 0x%x\n",(0x7FF & resp.port_mask));
	printf("\tPRIO: 0x%x\n",(0x7 & resp.prio));
	printf("===================================\n");


}
int rtu_read_learning_queue(request_fifo_t *learning_queue_hw)
{
  int i;
  //request_fifo_t learning_queue_hw;
  learning_queue_hw->count = rtu_hw_read_learning_queue_cnt();
  
  printf("HARDWARE   learning_queue word's cnt = %d\n",learning_queue_hw->count);
  for(i=0;i<=learning_queue_hw->count;i++)
      rtu_hw_read_learning_queue(&(learning_queue_hw->data[i]));
  
  printf("SIMULATION learning_queue word's cnt = %d\n",learning_queue.count);
  
  if(learning_queue_hw->count > learning_queue.count)
    return learning_queue_hw->count;
  else
    return learning_queue.count;
  
  printf("----------------------------------\n");
  printf("SIMULATION learning_queue word's cnt = %d\n",learning_queue.count);
  printf("head =  %d, tail = %d, cnt = %d \n",learning_queue.head,learning_queue.tail,learning_queue.count);
  for(i=0;i<learning_queue.head;i++)
      rtu_printf_entry_from_req_table(learning_queue.data[i]);
   

}
int rtu_dump_learning_queue(rtu_request_t sim_rq,rtu_request_t hw_rq)
{
  
   int ret = 0;
    
  if(sim_rq.port_id != sim_rq.port_id)                                         ret = -1;
  if((sim_rq.src_high != hw_rq.src_high) || (sim_rq.src_low != hw_rq.src_low)) ret = -1;
  if(sim_rq.has_vid != hw_rq.has_vid)                                          ret = -1;
  if(sim_rq.has_prio != hw_rq.has_prio)                                        ret = -1;
  if((sim_rq.dst_high != hw_rq.dst_high) || (sim_rq.dst_low != hw_rq.dst_low)) ret = -1;
   
#ifndef DEBUG_RESULTS
  if(ret == 0) return 0;
#endif
   
  static show_top = 1;

  if(show_top) printf("\n\n        COMPARING LEARNING QUEUES \n\n"); 
  if(show_top) printf("|---------------------------------------------|\n");
  if(show_top) printf("|                queue entries                |\n");
  if(show_top) printf("|---------------------------------------------|\n");
  if(show_top) printf("|         | simulation      |        h/w      |\n");
  if(show_top) printf("|=============================================|\n");
               printf("| port    |      %d          |        %d        |",sim_rq.port_id ,hw_rq.port_id);
  
  if(sim_rq.port_id != sim_rq.port_id)
    printf(" ERR \n");
  else
    printf(" OK \n");

  
               printf("| src mac | 0x%4x%8x  | 0x%4x%8x  |",sim_rq.src_high,sim_rq.src_low,hw_rq.src_high,hw_rq.src_low);
	       
  if((sim_rq.src_high != hw_rq.src_high) || (sim_rq.src_low != hw_rq.src_low))
    printf(" ERR \n");
  else
    printf(" OK \n");

                printf("| dst mac | 0x%4x%8x  | 0x%4x%8x  |",sim_rq.dst_high,sim_rq.dst_low,hw_rq.dst_high,hw_rq.dst_low);
	       
  if((sim_rq.dst_high != hw_rq.dst_high) || (sim_rq.dst_low != hw_rq.dst_low))
    printf(" ERR \n");
  else
    printf(" OK \n");

	       
               
   
  if(sim_rq.has_vid)
	  printf("| VID     |     0x%3x       |", sim_rq.vid);
	else
	  printf("| VID     |    NOT valid    |");
  if(hw_rq.has_vid)
	  printf("      0x%3x      |", hw_rq.vid);
	else
	  printf("    NOT valid    |");

  if(sim_rq.has_vid != hw_rq.has_vid)
    printf(" ERR \n");
  else
    printf(" OK \n");

	
  if(sim_rq.has_prio)
	  printf("| PRIO    |     0x%x         |", sim_rq.prio);
	else
	  printf("| PRIO    |    NOT valid    |");

  if(hw_rq.has_prio)
	  printf("      0x%x        |", hw_rq.prio);
	else
	  printf("    NOT valid    |");

  if(sim_rq.has_prio != hw_rq.has_prio)
    printf(" ERR \n");
  else
    printf(" OK \n");
	
  printf("|=============================================|\n");
  show_top=0;

  return ret;

}

int rtu_dump_aging_mems(changed_aging_htab_word_t sim_agr,changed_aging_htab_word_t hw_agr)
{
  int ret = 0;
  int i=0;
  uint32_t w = sim_agr.word;
  while(w!=0)
  {
    w=w>>1;
    if(i==32) break;
    i++;
  }
  if(i>0) i=i-1;
  
  uint16_t sim_htab_addr = ((sim_agr.address << 5) + i )>>2 ;
  w = hw_agr.word;
  i=0;
  while(w!=0)
  {
    w=w>>1;
    if(i==32) break;
    i++;
  }
  if(i>0) i=i-1;
  
  uint16_t  hw_htab_addr = ((hw_agr.address << 5) + i)>>2 ;
  
  
  if((sim_agr.address != hw_agr.address) || (sim_agr.word != hw_agr.word)) ret = -1;
#ifndef DEBUG_RESULTS
  if(ret == 0) return 0;
#endif
  static show_top = 1;

  if(show_top) printf("\n\n                      COMPARING AGING MEMORIES \n\n"); 
  if(show_top) printf("|-----------------------------------------------------------------------------|\n");
  if(show_top) printf("|                                memory words                                 |\n");
  if(show_top) printf("|-----------------------------------------------------------------------------|\n");
  if(show_top) printf("|              simulation              |                  h/w                 |\n");
  if(show_top) printf("|-----------------------------------------------------------------------------|\n");
  if(show_top) printf("|agr_htab_addr|htab_addr|    content   |agr_htab_addr|htab_addr|   content    |\n");
  if(show_top) printf("|=============================================================================|\n");
               printf("|    0x%3x    | 0x%4x   |0x%8x   |    0x%3x    | 0x%4x   | 0x%8x  |",sim_agr.address,sim_htab_addr, sim_agr.word,hw_agr.address,hw_htab_addr, hw_agr.word);
  
  if((sim_agr.address != hw_agr.address) || (sim_agr.word != hw_agr.word))
    printf(" ERR \n");
  else
    printf(" OK \n");

  printf("|=============================================================================|\n");
  show_top = 0;
  return ret;
}

int rtu_dump_agr_hcam(uint32_t sim_agr_hcam,uint32_t hw_gr_hcam)
{
  int ret = 0;
  
  if(sim_agr_hcam != hw_gr_hcam) ret = -1;
#ifndef DEBUG_RESULTS
  if(ret == 0) return 0;
#endif  
  
  printf("|agr_cam      | 0x%8x             |agr_cam       | 0x%8x            |",sim_agr_hcam,hw_gr_hcam);
  
  if(sim_agr_hcam != hw_gr_hcam)
    printf(" ERR \n");
  else
    printf(" OK \n");

  printf("|=============================================================================|\n");

  return ret;
}
int rtu_dump_results(rtu_request_t rq,rtu_response_t sim_resp,rtu_response_t hw_resp,int req_id)
{
  
  int ret = 0;
  if((0x7FF & sim_resp.port_mask)!=(0x7FF & hw_resp.port_mask)) ret = -1;
  if((0x7   & sim_resp.prio)!=(0x7   & hw_resp.prio))           ret = -1;
  if((0x1 & sim_resp.drop)!=(0x1 & hw_resp.drop))               ret = -1;
  if(sim_resp.port_id != hw_resp.port_id )	                ret = -1;
     
#ifndef DEBUG_RESULTS  
  if(ret == 0) return 0;
#endif
    
    
   static show_top = 1;
   
   
  if(show_top) printf("\n\n                     COMPARING RESULTS OF SIMULATION AND RTU \n\n"); 
  if(show_top) printf("|------------------------------------------------------------------------|-----|\n");
  if(show_top) printf("|                         |                    response                  |  R  |\n");
  if(show_top) printf("|       request           |----------------------------------------------|  E  |\n");
  if(show_top) printf("|                         |       | simulation      |        h/w         |  S  |\n");
  if(show_top) printf("|========================================================================|=====|\n");
               printf("|comment : %61s | [%2d]|\n",rq.short_comment,req_id);
               printf("|------------------------------------------------------------------------|-----|\n");
               printf("| port   : %d              | MASK  |      0x%3x      |       0x%3x        |",rq.port_id            ,(0x7FF & sim_resp.port_mask),(0x7FF & hw_resp.port_mask));
  
  if((0x7FF & sim_resp.port_mask)!=(0x7FF & hw_resp.port_mask))
    printf(" ERR |\n");
  else
    printf(" OK  |\n");
  
               printf("| src mac: 0x%4x%8x | PRIO  |      0x%x        |        0x%x         |",rq.src_high,rq.src_low,(0x7   & sim_resp.prio)     ,(0x7   & hw_resp.prio) );
	       
  if((0x7   & sim_resp.prio)!=(0x7   & hw_resp.prio))
    printf(" ERR |\n");
  else
    printf(" OK  |\n");
	       
               printf("| dst mac: 0x%4x%8x | DROP  |",rq.dst_high,rq.dst_low);
  if(0x1 & sim_resp.drop)
    printf("      YES        |");
  else
    printf("       NO        |");

  if(0x1 & hw_resp.drop)
    printf("        YES         |");
  else
    printf("         NO         |");
  
  if((0x1 & sim_resp.drop)!=(0x1 & hw_resp.drop))
    printf(" ERR |\n");
  else
    printf(" OK  |\n");
  
  if(rq.has_vid)
	  printf("| VID    : 0x%3x          |", rq.vid);
	else
	  printf("| VID    : NOT valid      |");
	
  printf(" PORT  |      %2d         |         %2d         |",sim_resp.port_id,hw_resp.port_id);	
  
  if(sim_resp.port_id != hw_resp.port_id )
    printf(" ERR |\n");
  else
    printf(" OK  |\n");
  
  //printf("                                              |     |\n");	
  if(rq.has_prio)
	  printf("| PRIO   : 0x%x            |", rq.prio);
	else
	  printf("| PRIO   : NOT valid      |");
  printf("                                              |     |\n");
  
  printf("| S_HASH : 0x%3x          | ",hash(rq.src_high,rq.src_low, vlan_tab[rq.vid].fid));
  printf("                                             |     |\n");
  printf("| D_HASH : 0x%3x          |",hash(rq.dst_high,rq.dst_low, vlan_tab[rq.vid].fid));
  printf("                                              |     |\n");
  printf("|========================================================================|=====|\n");
  show_top=0;
  return ret;
}

int summary(int err)
{
  	printf("\n\n\n\n");
	printf("\t\t ======================= \n");
	printf("\t\t ======  SUMMARY  ====== \n");
	printf("\t\t ======================= \n");
	printf("\t\t =                     = \n");
	if(err < -300)
	{
	  printf("\t\t = TEST *BADLY* FAILED = \n");
	  printf("\t\t =                     = \n");
	  printf("\t\t =      At least       = \n");
	  printf("\t\t =     >> %2d  <<      = \n",abs(err));
	  printf("\t\t =   errors detected   = \n");
	  printf("\t\t =                     = \n");
	  printf("\t\t =   THIS IS REALLY    = \n");
	  printf("\t\t =   > > B A D < <     = \n");
	  printf("\t\t =                     = \n");	  
	  printf("\t\t =   HAVE YOU LOADED   = \n");	  
	  printf("\t\t =   IMAGE INTO FPGA   = \n");	  	  
	  printf("\t\t =        ????         = \n");	  
	  printf("\t\t =                     = \n");	  
	  printf("\t\t =        ;-)          = \n");	  

	}
	else if(err < 0)
	{
	  printf("\t\t =    TEST FAILED      = \n");
	  printf("\t\t =                     = \n");
	  printf("\t\t =      At least       = \n");
	  printf("\t\t =     >> %2d  <<       = \n",abs(err));
	  printf("\t\t =   errors detected   = \n");


	}
	else
	{
	  printf("\t\t =    TEST SUCCEDDED   = \n");
	  printf("\t\t =                     = \n");
	  printf("\t\t =    Congratulation   = \n");

	}
	printf("\t\t =                     = \n");
	printf("\t\t ======================= \n");
	printf("\n\n\n\n");
}
int rtu_dbg_hcam()
{
  	printf("\nnot implemented for simulation\n");
	printf("\n==== cam =====\n\n");
	int z;
	for(z=0;z<8*6;z=z+8)
	{
	  printf("\n==== w%d =====\n",z/8);
	  rtu_hw_dbg_hcam(z);	
	}
}
int clean_output_fifos()
{
        int j;
	int ret=0;
	rtu_response_t response_tab_hw;
	int err_i = 0;
	printf("----Clean output fifos------\n");
	while(ret != 0)
	{
	  //check each port
	  ret = 0;
	  for(j=0;j<PORT_NUMBER;j++)
	  {
	    // check if there is any answer on the portF
	    if(rtu_hw_check_if_answer_on_port( j ))
	    {
		printf("read from output fifo\n");
		rtu_hw_read_response_from_port(&response_tab_hw,j);
		printf("PORT = %d, mask = 0x%x, drop=%d, prio=%d\n",response_tab_hw.port_id,response_tab_hw.port_mask,response_tab_hw.drop, response_tab_hw.prio);
		ret = 1;
	    }
	      
	  }
	  
	  if(err_i++ > 100000) {printf("problem with reading responses (hanged on reading hw-responses)\n"); break;}
	
	}




}


int rtu_info()
{
      int i,j;
      int cnt;
      int test;
      rtu_response_t response_tab_hw;
    //queue
     int hw_learning_queue_cnt  = rtu_hw_read_learning_queue_cnt();
     int sim_learning_queue_cnt = learning_queue.count;
     rtu_hw_read_global_setting();
     rtu_sim_read_global_setting();
     printf("Learning queue cnt : sim = %3d  | hw = %d \n",sim_learning_queue_cnt,hw_learning_queue_cnt);
     printf("Learning queue tail: sim = %3d  | ------- \n",learning_queue.tail);
     printf("Learning queue head: sim = %3d  | ------- \n",learning_queue.head);

     for (i=0;i<PORT_NUMBER;i++)
     {
	  printf("PORT[%d].request_fifo_cnt = %d \n",i, rtu_hw_check_request_fifo_cnt_on_port(i));
     }

     for (i=0;i<PORT_NUMBER;i++)
     {
	  cnt = rtu_hw_check_response_fifo_cnt_on_port(i);
	  test = rtu_hw_read_test_reg_on_port(i);
	  printf("PORT[%d].response_fifo_cnt = %d , test = 0x%x\n",i,cnt,test);
	  for(j=0;j<cnt;j++)
	  {
	    if(rtu_hw_check_if_answer_on_port(i ))
	      {
		  rtu_hw_read_response_from_port(&response_tab_hw,i);
		  printf("PORT = %d, mask = 0x%x, drop=%d, prio=%d\n",response_tab_hw.port_id,response_tab_hw.port_mask,response_tab_hw.drop, response_tab_hw.prio);
		  
	      }
	  }
     }
}

rtu_read_hcam_entries()
{
	int i;
	printf("\n  ===== hcam dump ==== \n");
	for (i=0;i<32;i++)
	{
	  rtu_hw_read_hcam_entry( 8*i );
	  rtu_sim_read_hcam_entry(i);
	}
	printf("\n  ===== ==== \n");


}