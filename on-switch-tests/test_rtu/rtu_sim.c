/*
-------------------------------------------------------------------------------
-- Title      : Routing Table Unit Software Simulation
-- Project    : White Rabbit Switch
-------------------------------------------------------------------------------
-- File       : rtu_sim.c
-- Authors    : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Created    : 2010-04-27
-- Last update: 2010-06-30
-- Description: - functions necessary to fill in tables representing H/W memories
--              - simulation engine
--
--
-- Revisions  :
-- Date        Version  Author          Description
-- 2010-04-27  1.0      twlostow	      Created
-- 2010-06-30  1.1      Maciej Lipinski	    changes to algorithm, functions to input
                                            data to data structures

*/


#include <stdio.h>

#include "rtu_sim.h"
#include "rtu_test_main.h"

int rtu_sim_read_hcam_entry( uint16_t i)
{
   int hcam_active_bank = (CFG.cam_bank == 0 ? 1 : 0);
 
  printf("sim_hcam[bank = %d][addr = 0x%x]: mac: 0x%2x%2x%2x%2x%2x%2x , fid: 0x%x\n",hcam_active_bank , i,rtu_cam[hcam_active_bank][i].mac[0],rtu_cam[hcam_active_bank][i].mac[1],rtu_cam[hcam_active_bank][i].mac[2],rtu_cam[hcam_active_bank][i].mac[3],rtu_cam[hcam_active_bank][i].mac[4],rtu_cam[hcam_active_bank][i].mac[5],rtu_cam[hcam_active_bank][i].fid) ;

}

/*
set active ZBT bank (0 or 1)
*/
int rtu_sim_set_active_htab_bank(uint8_t bank)
{
    if(bank != 0  && bank != 1)
      return -1;

    CFG.rtu_bank = bank;

    return 1;
}
/*
set active CAM bank (0 or 1)
*/
int rtu_sim_set_active_hcam_bank(uint8_t bank)
{
    if(bank != 0  && bank != 1)
      return -1;

    CFG.cam_bank = bank;

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
int rtu_sim_set_fixed_prio_on_port(int port, uint8_t prio, uint8_t set)
{
  
  if(set)
  {
      CFG.port_cfg[port].fixed_prio_ena = 1;
      CFG.port_cfg[port].fixed_prio_val = prio;
  }
  else
  {
      CFG.port_cfg[port].fixed_prio_ena = 0;
      CFG.port_cfg[port].fixed_prio_val = 0;
  }
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
*/
int rtu_sim_rtu_setting(uint8_t global_enable,uint8_t hcam_bank,uint8_t htab_bank, uint32_t hash_poly)
{
     
     CFG.global_enable = global_enable;
     CFG.cam_bank = (int) hcam_bank;
     CFG.rtu_bank = (int) htab_bank;
     CFG.hash_poly = hash_poly;

}

int rtu_sim_set_hash_poly(uint32_t hash_poly)
{
     
     CFG.hash_poly = hash_poly;

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
int rtu_sim_rtu_port_setting(int port, uint32_t settings)
{
	if(settings & FPGA_RTU_PCR_LEARN_EN)
	  CFG.port_cfg[port].learning_enabled = 1;
	else
	  CFG.port_cfg[port].learning_enabled = 0;
	
 	if(settings & FPGA_RTU_PCR_PASS_ALL)
	 CFG.port_cfg[port].pass_all = 1;
	else
	 CFG.port_cfg[port].pass_all = 0;
	
	if(settings & FPGA_RTU_PCR_PASS_BPDU)
	 CFG.port_cfg[port].forward_bpdu_only = 1;
	else
	 CFG.port_cfg[port].forward_bpdu_only = 0;
	
	if(settings & FPGA_RTU_PCR_B_UNREC)
	 CFG.port_cfg[port].b_unrec = 1;
	else
	 CFG.port_cfg[port].b_unrec = 0;
	
	if(settings & FPGA_RTU_PCR_FIX_PRIO)
	{
	    CFG.port_cfg[port].fixed_prio_ena = 1;
	    CFG.port_cfg[port].fixed_prio_val = 0x7 & (settings >> 4);
	}
	else
	{
	    CFG.port_cfg[port].fixed_prio_ena = 0;
	    CFG.port_cfg[port].fixed_prio_val = 0;
	}	
	
	  
	return 1;

}
/*
basic settings
*/
int rtu_sim_set()
{
  // enable learning on all ports
  int i;
  for(i=0;i<RTU_NUM_PORTS;i++)
    {
    CFG.port_cfg[i].learning_enabled = 1;
    CFG.port_cfg[i].forward_bpdu_only = 0;
    }
  CFG.cam_bank = 0;
  CFG.rtu_bank = 0;
  
  return 1;
}

int rtu_sim_read_global_setting()
{
	int i;
	
	
	printf("Reading RTU Global setting:\n");
	
	if( CFG.global_enable) 
	  printf("RTU enabled; ");
	else
	  printf("RTU disabled; ");
	
	if(CFG.cam_bank) 
	  printf("hcam_bank = 1; ");
	else
	  printf("hcam_bank = 0; ");
	
	if(CFG.rtu_bank) 
	  printf("htab_bank = 1; ");
	else
	  printf("htab_bank = 0; ");
	
	printf("poly= 0x%x\n",CFG.hash_poly);
	
}

/*
  Writes entry to a table representing zbt sram in software.
  
*/

int rtu_sim_write_htab_entry(int      valid,         int      end_of_bucket,    int     is_bpdu,        int go_to_cam , \
		             uint8_t  fid,           uint16_t mac_hi,           uint32_t mac_lo,  \
		             uint8_t  cam_addr,      int      drop_when_source, int      drop_when_dest, int drop_when_unmatched_src_ports, \
		             uint8_t  prio_src,      uint8_t  has_prio_src,     uint8_t  prio_override_src, \
		             uint8_t  prio_dst,      int      has_prio_dst,     int      prio_override_dst, \
                             uint16_t port_mask_src, uint16_t port_mask_dst,    uint32_t last_access ,   uint32_t zbt_addr /*, int bank */, int bucket)
{
  int addr   = zbt_addr >> 5;


  int bank;
  if(CFG.rtu_bank == 1 )      bank = 0;
  else if(CFG.rtu_bank == 0 ) bank = 1;
  else {printf("ERROR\n");    return -1;}
 
  #ifdef ML_DBG
  printf("writing data to zbt: bank = %d, htab_addr = 0x%3x [zbt_addr = 0x%3x], bucket = %d\n", bank,addr,zbt_addr,bucket);
  #endif 
  
  
  rtu_tab[bank].buckets[addr][bucket].valid                      = valid;        
  rtu_tab[bank].buckets[addr][bucket].end_of_bucket              = end_of_bucket; 
  rtu_tab[bank].buckets[addr][bucket].is_bpdu                    = is_bpdu;
  rtu_tab[bank].buckets[addr][bucket].mac[0] = (mac_hi >> 8);
  rtu_tab[bank].buckets[addr][bucket].mac[1] = (0xFF & mac_hi);
  rtu_tab[bank].buckets[addr][bucket].mac[2] = (0xFF & (mac_lo >> 24));
  rtu_tab[bank].buckets[addr][bucket].mac[3] = (0xFF & (mac_lo >> 16));
  rtu_tab[bank].buckets[addr][bucket].mac[4] = (0xFF & (mac_lo >> 8));
  rtu_tab[bank].buckets[addr][bucket].mac[5] = (0xFF & mac_lo) ; 
  rtu_tab[bank].buckets[addr][bucket].fid                        = fid; 
  rtu_tab[bank].buckets[addr][bucket].port_mask_src              = port_mask_src; 
  rtu_tab[bank].buckets[addr][bucket].port_mask_dst              = port_mask_dst; 
  rtu_tab[bank].buckets[addr][bucket].drop_when_source           = drop_when_source;
  rtu_tab[bank].buckets[addr][bucket].drop_when_dest	         = drop_when_dest;             
  rtu_tab[bank].buckets[addr][bucket].drop_unmatched_src_ports   = drop_when_unmatched_src_ports; 
  rtu_tab[bank].buckets[addr][bucket].last_access_t              = last_access; 
  rtu_tab[bank].buckets[addr][bucket].prio_src                   = prio_src; 
  rtu_tab[bank].buckets[addr][bucket].has_prio_src               = has_prio_src;
  rtu_tab[bank].buckets[addr][bucket].prio_override_src          = prio_override_src; 
  rtu_tab[bank].buckets[addr][bucket].prio_dst                   = prio_dst;
  rtu_tab[bank].buckets[addr][bucket].has_prio_dst               = has_prio_dst; 
  rtu_tab[bank].buckets[addr][bucket].prio_override_dst          = prio_override_dst;
  rtu_tab[bank].buckets[addr][bucket].go_to_cam                  = go_to_cam;
  rtu_tab[bank].buckets[addr][bucket].cam_addr                   = cam_addr;
  
  return 1;
}
/*
  Writes entry to a table representing hcam in software.
*/
int rtu_sim_write_hcam_entry(int   valid,         int      end_of_bucket,    int     is_bpdu,        int go_to_cam , \
		uint8_t  fid,           uint16_t mac_hi,           uint32_t mac_lo,  \
		uint8_t  cam_addr,      int      drop_when_source, int      drop_when_dest, int drop_when_unmatched_src_ports, \
		uint8_t  prio_src,      uint8_t  has_prio_src,     uint8_t  prio_override_src, \
		uint8_t  prio_dst,      int      has_prio_dst,     int      prio_override_dst, \
                uint16_t port_mask_src, uint16_t port_mask_dst,    uint32_t last_access ,   uint16_t hcam_addr /*, int bank */)
{
  
  int addr   = hcam_addr/8;
  int bank;
  if(CFG.cam_bank == 1 )      bank = 0;
  else if(CFG.cam_bank == 0 ) bank = 1;
  else {printf("ERROR\n");    return -1;}

  #ifdef ML_DBG
  printf("writing  data to cam: bank = %d, entry number = %d [hcam_addr = 0x%x]\n", bank,addr,hcam_addr);
  #endif

  rtu_cam[bank][addr].valid                      = valid;        
  rtu_cam[bank][addr].end_of_bucket              = end_of_bucket; 
  rtu_cam[bank][addr].is_bpdu                    = is_bpdu;
  rtu_cam[bank][addr].mac[0] = (mac_hi >> 8);
  rtu_cam[bank][addr].mac[1] = (0xFF & mac_hi);
  rtu_cam[bank][addr].mac[2] = (0xFF & (mac_lo >> 24));
  rtu_cam[bank][addr].mac[3] = (0xFF & (mac_lo >> 16));
  rtu_cam[bank][addr].mac[4] = (0xFF & (mac_lo >> 8));
  rtu_cam[bank][addr].mac[5] = (0xFF & mac_lo) ; 
  rtu_cam[bank][addr].fid                        = fid; 
  rtu_cam[bank][addr].port_mask_src              = port_mask_src; 
  rtu_cam[bank][addr].port_mask_dst              = port_mask_dst; 
  rtu_cam[bank][addr].drop_when_source           = drop_when_source;
  rtu_cam[bank][addr].drop_when_dest	         = drop_when_dest;             
  rtu_cam[bank][addr].drop_unmatched_src_ports   = drop_when_unmatched_src_ports; 
  rtu_cam[bank][addr].last_access_t              = last_access; 
  rtu_cam[bank][addr].prio_src                   = prio_src; 
  rtu_cam[bank][addr].has_prio_src               = has_prio_src;
  rtu_cam[bank][addr].prio_override_src          = prio_override_src; 
  rtu_cam[bank][addr].prio_dst                   = prio_dst;
  rtu_cam[bank][addr].has_prio_dst               = has_prio_dst; 
  rtu_cam[bank][addr].prio_override_dst          = prio_override_dst;
  rtu_cam[bank][addr].go_to_cam                  = go_to_cam;
  rtu_cam[bank][addr].cam_addr                   = cam_addr;
  
  return 1;
}
/*
  Writes entry to a table representing vlan in software.
*/
int rtu_sim_write_vlan_entry(int addr, uint32_t port_mask,uint8_t fid,uint8_t prio,int has_prio,int prio_override,int drop)
{
  #ifdef ML_DBG
  printf("writing data to vlan: addr = 0x%x]\n", addr);
  #endif
  vlan_tab[addr].port_mask      = port_mask; 
  vlan_tab[addr].fid            = fid; 
  vlan_tab[addr].prio           = prio; 
  vlan_tab[addr].has_prio       = has_prio;
  vlan_tab[addr].prio_override  = prio_override; 
  vlan_tab[addr].drop           = drop;


  return 1;
}

int rtu_sim_read_arg_htab_changes(changed_aging_htab_word_t sim_agr_htab[])
{
   int i;
   int  sim_cnt;
   uint32_t tmp;
   
  
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
  
    return sim_cnt;
}

//////////////////////////////////// cleaning mems ////////////////////////////////////
/*
 Cleans tables repesenting H/W memories (zbt sram, vlan, hcam, 
*/
int rtu_sim_clean_mems()
{
  int i, j;
  for(i=0;i<RTU_ENTRIES/RTU_BUCKETS;i++)
    for(j=0;j<RTU_BUCKETS;j++)
      {
	rtu_tab[0].buckets[i][j].valid = 0;
	rtu_tab[0].buckets[i][j].end_of_bucket = 0; 
	rtu_tab[0].buckets[i][j].is_bpdu = 0;   
	rtu_tab[0].buckets[i][j].mac[0] = 0;
	rtu_tab[0].buckets[i][j].mac[1] = 0;
	rtu_tab[0].buckets[i][j].mac[2] = 0;
	rtu_tab[0].buckets[i][j].mac[3] = 0;
	rtu_tab[0].buckets[i][j].mac[4] = 0;
	rtu_tab[0].buckets[i][j].mac[5] = 0;
	rtu_tab[0].buckets[i][j].fid = 0; 
	rtu_tab[0].buckets[i][j].port_mask_src = 0; 
	rtu_tab[0].buckets[i][j].port_mask_dst = 0; 
	rtu_tab[0].buckets[i][j].drop_when_source = 0; 
	rtu_tab[0].buckets[i][j].drop_when_dest = 0; 
	rtu_tab[0].buckets[i][j].drop_unmatched_src_ports = 0;
	rtu_tab[0].buckets[i][j].last_access_t = 0; 
	rtu_tab[0].buckets[i][j].prio_src = 0; 
	rtu_tab[0].buckets[i][j].has_prio_src = 0; 
	rtu_tab[0].buckets[i][j].prio_override_src = 0;
	rtu_tab[0].buckets[i][j].prio_dst = 0; 
	rtu_tab[0].buckets[i][j].has_prio_dst = 0; 
	rtu_tab[0].buckets[i][j].prio_override_dst = 0; 
	rtu_tab[0].buckets[i][j].go_to_cam = 0;
	rtu_tab[0].buckets[i][j].cam_addr = 0   ; 
	
	rtu_tab[1].buckets[i][j].valid = 0;
	rtu_tab[1].buckets[i][j].valid = 0;         
	rtu_tab[1].buckets[i][j].end_of_bucket = 0; 
	rtu_tab[1].buckets[i][j].is_bpdu = 0;   
	rtu_tab[1].buckets[i][j].mac[0] = 0;
	rtu_tab[1].buckets[i][j].mac[1] = 0;
	rtu_tab[1].buckets[i][j].mac[2] = 0;
	rtu_tab[1].buckets[i][j].mac[3] = 0;
	rtu_tab[1].buckets[i][j].mac[4] = 0;
	rtu_tab[1].buckets[i][j].mac[5] = 0;
	rtu_tab[1].buckets[i][j].fid = 0; 
	rtu_tab[1].buckets[i][j].port_mask_src = 0; 
	rtu_tab[1].buckets[i][j].port_mask_dst = 0; 
	rtu_tab[1].buckets[i][j].drop_when_source = 0; 
	rtu_tab[1].buckets[i][j].drop_when_dest = 0; 
	rtu_tab[1].buckets[i][j].drop_unmatched_src_ports = 0;
	rtu_tab[1].buckets[i][j].last_access_t = 0; 
	rtu_tab[1].buckets[i][j].prio_src = 0; 
	rtu_tab[1].buckets[i][j].has_prio_src = 0; 
	rtu_tab[1].buckets[i][j].prio_override_src = 0;
	rtu_tab[1].buckets[i][j].prio_dst = 0; 
	rtu_tab[1].buckets[i][j].has_prio_dst = 0; 
	rtu_tab[1].buckets[i][j].prio_override_dst = 0; 
	rtu_tab[1].buckets[i][j].go_to_cam = 0;
	rtu_tab[1].buckets[i][j].cam_addr = 0; 	
	
	//rtu_tab[1].buckets[i][j].valid = 0;
	
	
	
      }

  for(i=0;i<CAM_ENTRIES;i++)
    {
 //     rtu_cam[0][i].valid = 0;
   //   rtu_cam[1][i].valid = 0;
      
      	 rtu_cam[0][i].valid = 0;
	 rtu_cam[0][i].end_of_bucket = 0; 
	 rtu_cam[0][i].is_bpdu = 0;   
	 rtu_cam[0][i].mac[0] = 0;
	 rtu_cam[0][i].mac[1] = 0;
	 rtu_cam[0][i].mac[2] = 0;
	 rtu_cam[0][i].mac[3] = 0;
	 rtu_cam[0][i].mac[4] = 0;
	 rtu_cam[0][i].mac[5] = 0;
	 rtu_cam[0][i].fid = 0; 
	 rtu_cam[0][i].port_mask_src = 0; 
	 rtu_cam[0][i].port_mask_dst = 0; 
	 rtu_cam[0][i].drop_when_source = 0; 
	 rtu_cam[0][i].drop_when_dest = 0; 
	 rtu_cam[0][i].drop_unmatched_src_ports = 0;
	 rtu_cam[0][i].last_access_t = 0; 
	 rtu_cam[0][i].prio_src = 0; 
	 rtu_cam[0][i].has_prio_src = 0; 
	 rtu_cam[0][i].prio_override_src = 0;
	 rtu_cam[0][i].prio_dst = 0; 
	 rtu_cam[0][i].has_prio_dst = 0; 
	 rtu_cam[0][i].prio_override_dst = 0; 
	 rtu_cam[0][i].go_to_cam = 0;
	 rtu_cam[0][i].cam_addr = 0   ; 
	
      	 rtu_cam[1][i].valid = 0;
	 rtu_cam[1][i].end_of_bucket = 0; 
	 rtu_cam[1][i].is_bpdu = 0;   
	 rtu_cam[1][i].mac[0] = 0;
	 rtu_cam[1][i].mac[1] = 0;
	 rtu_cam[1][i].mac[2] = 0;
	 rtu_cam[1][i].mac[3] = 0;
	 rtu_cam[1][i].mac[4] = 0;
	 rtu_cam[1][i].mac[5] = 0;
	 rtu_cam[1][i].fid = 0; 
	 rtu_cam[1][i].port_mask_src = 0; 
	 rtu_cam[1][i].port_mask_dst = 0; 
	 rtu_cam[1][i].drop_when_source = 0; 
	 rtu_cam[1][i].drop_when_dest = 0; 
	 rtu_cam[1][i].drop_unmatched_src_ports = 0;
	 rtu_cam[1][i].last_access_t = 0; 
	 rtu_cam[1][i].prio_src = 0; 
	 rtu_cam[1][i].has_prio_src = 0; 
	 rtu_cam[1][i].prio_override_src = 0;
	 rtu_cam[1][i].prio_dst = 0; 
	 rtu_cam[1][i].has_prio_dst = 0; 
	 rtu_cam[1][i].prio_override_dst = 0; 
	 rtu_cam[1][i].go_to_cam = 0;
	 rtu_cam[1][i].cam_addr = 0   ; 
      
    }

  learning_queue.head = 0;
  learning_queue.tail = MAX_FIFO_SIZE-1;
  learning_queue.count = 0;


  // by default, VLANs are disabled
  for(i=1;i<MAX_VLANS;i++)
    {
      vlan_tab[i].drop = 1;
    }
  // clean main aging memory  
  for(i=0;i< ARAM_WORDS; i++)
     rtu_agr_htab[i]= 0x00000000;
  
  //clean hcam aging reg
  rtu_agr_hcam= 0x00000000;
  

  return 1;
}



char * mac_2_string(mac_addr_t mac)
{
  char str[80];
  sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return strdup(str);
}

int cmp_macs(mac_addr_t mac_1,mac_addr_t mac_2)
{
  int i;
  for (i=0;i<6;i++)
     if(mac_1[i]!=mac_2[i]) return 0;
  
  return 1;



}
/******************************************************************************/
/* FUNCTIONS BELOW CONTAIN THE RTU ALGORITHM TO BE IMPLEMENTED IN VHDL        */
/******************************************************************************/
/* CHANGES TO THE ALGORITHM BY maciej.lipinski@cern.ch                
 - if message dropped, port_mask and prio are set to 0x0
 - request is sent to learning queue only once during match, 
   initially, if the source entry was not found, the request was sent to 
   learning queue, subsequently if the destinatin entry was not found as well,
   the (same) request was sent to learning queue again


                                                                              */
/******************************************************************************/

void add_to_learning_queue(rtu_request_t rq, int verbose)
{

  if(verbose) printf("add_to_learning_queue: src=%s dst=%s vid=%d port=%d prio_has=%d prio=%d\n", mac_2_string(rq.src), mac_2_string(rq.dst), rq.has_vid?rq.vid:0,  rq.port_id,rq.has_prio, rq.prio);


  if(learning_queue.count == MAX_FIFO_SIZE) // Learning FIFO is full? - drop the request.
    return;

  learning_queue.count++;
  learning_queue.head++;

  if(learning_queue.head == MAX_FIFO_SIZE) learning_queue.head = 0;

  learning_queue.data[learning_queue.head] = rq;
}


// hash function - use your invention here, it doesn't need to use CRC
// mac - mac address, fid = Filtering Database ID
uint16_t hash_function(mac_addr_t mac, uint8_t fid)
{
  uint16_t ret_hash ;

  uint16_t mac_hi = (uint16_t)((mac[0] << 8) | mac[1]);
  uint32_t mac_lo = (mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) | mac[5];
  
  ret_hash = hash(mac_hi, mac_lo, fid);
  
/*
  // needed to change here to my functions which is 
  // synchronized with VHDL CRC
  
  hash = crc16(hash, (0xFFFF & vlan));
  hash = crc16(hash, ((uint16_t)mac[0] << 8) | mac[1]);
  hash = crc16(hash, ((uint16_t)mac[2] << 8) | mac[3]);
  hash = crc16(hash, ((uint16_t)mac[4] << 8) | mac[5]);
*/

  return ret_hash & 0xfff; //clip it to the MAC table size
}

int mac_table_lookup(mac_addr_t mac, uint8_t fid, mac_table_entry_t **found,int verbose)
{
  // 1st step: calculate hash of MAC + FID pair  
  uint16_t hash = hash_function(mac, fid);
  mac_table_entry_t *ent;
  int i;
  
  char str[80];
  sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  if(verbose) printf("hash [fid=0x%x mac=%s] = 0x%x bank = %d\n",fid, str, hash,CFG.rtu_bank);
  
  // get the entry with matching hash in MAC table
  ent = rtu_tab[CFG.rtu_bank].buckets[hash];
  
  // 2nd step: scan the bucket array for matching MAC+FID pair
  for(i =0; i<RTU_BUCKETS && ent->valid;i++,ent++)
    {
      //printf("\tlooking for: mac=%s fid=0x%x ,have: mac=%02x:%02x:%02x:%02x:%02x:%02x fid=0x%x [addr=0x%x], bucket = %d\n\n",str,fid,ent->mac[0], ent->mac[1], ent->mac[2], ent->mac[3], ent->mac[4], ent->mac[5],ent->fid,hash,i);
      //if(ent->mac == mac && ent->fid == fid) // found matching rule in the current bucket?
      if(cmp_macs(ent->mac, mac)==1 && ent->fid == fid) // found matching rule in the current bucket?
	{
	  #ifdef ML_DBG
	  printf("\t----------------------\n");
	  printf("\nBINGO,entry found in htab for addr=0x%x, bucket = %d\n\n",hash,i);
	  printf("\trtu_agr_htab[0x%x] = 0x%8x\n",(hash >> 3),0x1 << (((0x7 & hash) << 2) | (0x3 & i)));
	  printf("\t----------------------\n");
	  #endif
	  if(verbose) printf("> found in htab, bucket=%d <\n",i);
	  
	  number_of_found_entries++;
	  
	  //added by ML
	  if(verbose) printf("\n\nbefore: 0x%x , hash=0x%x, i=%i\n",rtu_agr_htab[(hash >> 3)],hash,i);
	  //if(verbose) printf("\n\nrtu_agr_htab[(hash >> 3)]= ( 0x1 << (((0x7 & hash) << 2) | (0x00000003 & (unsigned int)i) )) | rtu_agr_htab[(hash >> 3)] \n");
	  //if(verbose) printf("\n\n     0x%8x      =        1 << (0x%8x   |     0x%8x )    |   0x%8x      \n", \
				( 0x1 << (((0x7 & hash) << 2) | (0x00000003 & (unsigned int)i)) ) | rtu_agr_htab[(hash >> 3)],  	\
				 0x1 << ((0x7 & hash) << 2) ,        								\
				 (0x00000003 & (unsigned int)i) ,								\
				 rtu_agr_htab[(hash >> 3)]);
	  rtu_agr_htab[(hash >> 3)]= ( 0x1 << (((0x7 & hash) << 2) | (0x00000003 & (unsigned int)i)) ) | rtu_agr_htab[(hash >> 3)];
	  
	  if(verbose) printf("after: 0x%x \n\n",rtu_agr_htab[(hash >> 3)]);
	  /////
	  
	  *found = ent;
	  return 1;
	} else if (ent->go_to_cam) // the bucket is bigger than RTU_BUCKETS entries - go find the matching one in CAM memory.
	{
	 // printf("go to cam, cam_addr: 0x%x \n",ent->cam_addr);
	  int j = ent->cam_addr; // cam_addr in the last entry in the bucket points to the location of the next rule in CAM memory

	  // 3rd step: traverse the CAM memory until we find a matching entry or reach end of the bucket
	  for(; j<CAM_ENTRIES /*ML: && !rtu_cam[CFG.cam_bank][j].end_of_bucket */; j++) 
	  {
	    #ifdef ML_DBG
	    printf("[addr = 0x%x]looking in hcam foe: mac=%s fid=0x%x ,have: mac=%02x:%02x:%02x:%02x:%02x:%02x fid=0x%x [addr=0x%x],end=%d \n\n",j,str,fid,rtu_cam[CFG.cam_bank][j].mac[0], rtu_cam[CFG.cam_bank][j].mac[1], rtu_cam[CFG.cam_bank][j].mac[2], rtu_cam[CFG.cam_bank][j].mac[3], rtu_cam[CFG.cam_bank][j].mac[4], rtu_cam[CFG.cam_bank][j].mac[5],rtu_cam[CFG.cam_bank][j].fid,j,rtu_cam[CFG.cam_bank][j].end_of_bucket);
	    #endif
	    if( cmp_macs(rtu_cam[CFG.cam_bank][j].mac, mac) == 1 && rtu_cam[CFG.cam_bank][j].fid == fid)
	      {
		#ifdef ML_DBG
		printf("\t----------------------\n");
		printf("\nBINGO,entry foundin hcam for addr=0x%x, bucket = %d\n\n",hash,j);
		printf("\trtu_agr_hcam] = 0x%8x\n",(0x1 << j));
		printf("\t----------------------\n");
		#endif
		if(verbose) printf("> found in hcam <\n");
		number_of_found_entries++;
		*found = &rtu_cam[CFG.cam_bank][j]; // found matching entry in CAM
	        if(verbose) printf("\n\nbefore: 0x%x , hash=0x%x, i=%i\n",rtu_agr_hcam,hash,i);
		
		rtu_agr_hcam = (0x1 << j) | rtu_agr_hcam;
                
		if(verbose) printf("after: 0x%x \n\n",rtu_agr_hcam);
		return 1;
	      }
	      else if(rtu_cam[CFG.cam_bank][j].end_of_bucket == 1)
		break; //end if the bucket, nothing found
	  }
	}
	
    }

  // not found (neither in MAC table nor in CAM): return 0
  return 0;
}


#define PRIO_SOURCE 0
#define PRIO_DESTINATION 1
#define PRIO_VID 2
#define PRIO_PORT 3

#define NOT_DEFINED -1

// main rtu function: decides what to do with packet "rq"

int rtu_sim_match(rtu_request_t rq, rtu_response_t *rsp, int verbose )
{
  //added by ML:
  int already_in_learning_queue = 0;
  ///////////
  
  if(verbose) printf("rtu_match: src=%s dst=%s vid=0x%x port=%d port_prio=%d , has_prio = 0x%x, has_vid = 0x%x\n", mac_2_string(rq.src), mac_2_string(rq.dst), rq.has_vid?rq.vid:0,  rq.port_id, rq.prio,rq.has_prio,rq.has_vid);

  
  rsp->port_id = rq.port_id;

  //added by ML
  if(CFG.global_enable == 0) 
    {
      if(verbose)  printf("rtu_match: dropped (RTU disabled)\n");
      rsp->drop = 1;
      
      rsp->prio = 0;
      rsp->port_mask = 0;
      
      return 0;
    }  
  //ML: pass_all is in general port_enable,
  // port disabling by setting pass_all=0 can be overriden by setting forward_bpdu_only=1
  // so, regardless of pass_all's value, forward_bpdu_only causes only bpdu packages to be 
  // passed through
  if(CFG.port_cfg[rq.port_id].pass_all == 0 && CFG.port_cfg[rq.port_id].forward_bpdu_only == 0) 
    {
      if(verbose)  printf("rtu_match: dropped (port %d disabled)\n",rq.port_id);
      rsp->drop = 1;
      
      rsp->prio = 0;
      rsp->port_mask = 0;
      
      return 0;
    }
  /*
  implementing it this way, results in the fact that the changed
  priority is learned
  */
    // check if packet has per-port assigned priority
  if(CFG.port_cfg[rq.port_id].fixed_prio_ena)
  {
    rq.prio     = CFG.port_cfg[rq.port_id].fixed_prio_val;    
    rq.has_prio = 0x1;
    if(verbose) printf("setting to fixed prio(new data): has_prio = %d, prio = %d\n",CFG.port_cfg[rq.port_id].fixed_prio_ena, CFG.port_cfg[rq.port_id].fixed_prio_val);
  }
  /////////////
  
  // helper tables for priority selection.
  // priority from the most specific to least specific definitions (e.g, per tag -> per source MAC -> per destination MAC -> per VLAN )
  int prio_lookup[4] = {NOT_DEFINED, NOT_DEFINED, NOT_DEFINED, NOT_DEFINED};
  int prio_override[4] = {0,0,0,0};
  
  vlan_table_entry_t vent;

  //////////////////////////////////////////////////////////////////////////////
  // 1st step: determine the VLAN table entry for the packet VID
  //////////////////////////////////////////////////////////////////////////////

  uint16_t vid = (rq.has_vid ? rq.vid : 0); // if the packet has the VLAN id, use it, otherwise - use VID for untagged packets (0)

 
  vent = vlan_tab[vid];
  
  if(vent.drop) // VLAN is illegal - drop the packet and return
    {
      if(verbose)  printf("rtu_match: dropped (blocked VLAN)\n");
      rsp->drop = 1;
      //added by ML
      rsp->prio = 0;
      rsp->port_mask = 0;
      /////////////
      return 0;
    }

  if(vent.has_prio) // VLAN has defined priority - add it to priority selection table
    {
      prio_lookup[PRIO_VID] = vent.prio;
      prio_override[PRIO_VID] = vent.prio_override;
    }

  ////////////////////////////////////////////////
  // 2nd step: find the source MAC in MAC table
  /////////////////////////////////////////////////

  mac_table_entry_t *entry_src;
  uint32_t port_mask_src;
  int drop_unmatched_src_ports;


  if(mac_table_lookup(rq.src, vent.fid, &entry_src,verbose)) // lookup the source MAC address in the table
    { // found:
      port_mask_src = entry_src->port_mask_src;

      if(entry_src->drop_when_source) // source MAC address is blocked? - drop the packet
	{
	  if(verbose) printf("rtu_match: dropped (blocked source MAC)\n");
	  //added by ML
	  rsp->prio = 0;
	  rsp->port_mask = 0;
	  /////////////
	  rsp->drop = 1;
	  return 0;
	}

      if(entry_src->has_prio_src) // check if the source MAC has predefined priority and eventually add it to the selection table
	{
	  prio_lookup[PRIO_SOURCE] = entry_src->prio_src;
	  prio_override[PRIO_SOURCE] = entry_src->prio_override_src;;
	}

      drop_unmatched_src_ports = entry_src->drop_unmatched_src_ports;

    } else { // source MAC not found: assume it can come from any port, put the entry into learning queue
    port_mask_src = 0xffffffff; 
    drop_unmatched_src_ports = 0;

    if(CFG.port_cfg[rq.port_id].learning_enabled && (already_in_learning_queue==0)) 
    {
      add_to_learning_queue(rq,verbose);
      already_in_learning_queue = 1;
    }
  }

  // check if the packet with given source MAC can come from this port.
  if(! ((1<<rq.port_id) & port_mask_src))
    {
      if(drop_unmatched_src_ports) { // if the MAC address is locked to source port, drop the paket
	  if(verbose) printf("rtu_match: dropped (source MAC doesn't match source port)\n");
	  //added by ML
	  rsp->prio = 0;
	  rsp->port_mask = 0;
	  /////////////	  
	  rsp->drop =1;
	return 0;    
      } else { // otherwise add it to the learning queue - perhaps device has been reconnected to another port and topology info needs to be updated
	if(CFG.port_cfg[rq.port_id].learning_enabled && (already_in_learning_queue==0)) 
	{
	  if(verbose) printf("\n-----------------------\n");
	  if(verbose) printf("we have problem here???\n");
	  if(verbose) printf("(1<<rq.port_id) = 0x%x\n",(1<<rq.port_id));
	  if(verbose) printf("port_mask_src   = 0x%x\n",port_mask_src);
	  if(verbose) printf("-----------------------\n\n");
	  add_to_learning_queue(rq,verbose);
	  already_in_learning_queue = 1;
	}
      }
    }

  ////////////////////////////////////////////////////
  // 3rd step: find the destination MAC in MAC table
  ////////////////////////////////////////////////////

  mac_table_entry_t *entry_dst = NULL;
  uint32_t port_mask_dst;
  int is_bpdu;

  if(mac_table_lookup(rq.dst, vent.fid, &entry_dst,verbose)) // lookup the destination MAC address in the table
    { // found:
      port_mask_dst = entry_dst->port_mask_dst;


      if(entry_dst->drop_when_dest) // destination MAC address is blocked? - drop the packet
	{
	  if(verbose) printf("rtu_match: dropped (blocked destination MAC)\n");
	  //added by ML
	  rsp->prio = 0;
	  rsp->port_mask = 0;
	  /////////////	  
	  rsp->drop = 1;
	  return 0;
	}

//      if(entry_src->has_prio_dst) // check if the destination MAC has predefined priority and eventually add it to the selection table
//////////////
//ML: bug detected??
//////////////////
	if(entry_dst->has_prio_dst) // check if the destination MAC has predefined priority and eventually add it to the selection table
	{
	  prio_lookup[PRIO_DESTINATION] = entry_dst->prio_dst;
	  prio_override[PRIO_DESTINATION] = entry_dst->prio_override_dst;
	}
      is_bpdu = entry_dst->is_bpdu;
    } 
    else { // destination MAC not found: broadcast the packet, put the entry into learning queue
      
      
      	if(CFG.port_cfg[rq.port_id].learning_enabled && (already_in_learning_queue==0)) 
	{
	    add_to_learning_queue(rq,verbose);
	    already_in_learning_queue = 1;
	}
	
      
	if(CFG.port_cfg[rq.port_id].b_unrec == 0) // unrecongized request behaviour: drop packages
	{
	  if(verbose) printf("rtu_match: dropped (urecognized packages dropped)\n");
	  rsp->prio = 0;
	  rsp->port_mask = 0;
	  /////////////	  
	  rsp->drop = 1;
	  return 0;
	
	}
	else 	
	  port_mask_dst = 0xffffffff; 
  
	is_bpdu = 0; // BPDU addresses are always registered in MAC table
  }

  ////////////////////////////////////////////
  // 4th step: make the final routing decision:
  ////////////////////////////////////////////
  
  if(CFG.port_cfg[rq.port_id].forward_bpdu_only == 1 && !is_bpdu) // STP port blocked or learning
    {
      if(verbose) printf("rtu_match: dropped (non-BPDU packet on STP BLOCKED/LEARNING port)\n");
      //added by ML
      rsp->prio = 0;
      rsp->port_mask = 0;
      /////////////
      rsp->drop = 1;
      return 0;
    }

  rsp->port_mask = vent.port_mask & port_mask_dst; // generate the final port mask by anding the MAC-assigned destination ports with ports
  // registered in current VLAN


  //ML: why we don't do here like in case of prio src/dst??
  //    I mean: set prio_override only if has_prio=true
  if(rq.has_prio) 
      prio_lookup[PRIO_PORT] = rq.prio;

  // evaluate the final priority of the packet

  rsp->prio = 0;
  rsp->drop = 0;

  int found = 0;
  int i;

  for(i=0; i<4; i++) // check for overriding priorities 
    if(prio_override[i]) 
      {
	rsp->prio = prio_lookup[i];
	found = 1;
	break;
      }

  if(!found)
    {
      for(i=0;i<4;i++)
	if(prio_lookup[i] >= 0)
	  {
	    rsp->prio = prio_lookup[i];
	    break;
	  }
    }

  if(verbose) printf("rsp->port_mask[0x%x] = vent.port_mask[0x%x] & port_mask_dst[0x%x]\n", rsp->port_mask,vent.port_mask , port_mask_dst);
  if(verbose)  printf("port_mask_src = 0x%x\n", port_mask_src);
#ifdef DEBUG_SIMULATION
  printf("prio_override[PRIO_SOURCE     ] = 0x%x prio_lookup[PRIO_SOURCE     ] = 0x%x\n",prio_override[0], prio_lookup[0]);
  printf("prio_override[PRIO_DESTINATION] = 0x%x prio_lookup[PRIO_DESTINATION] = 0x%x\n",prio_override[1], prio_lookup[1]);
  printf("prio_override[PRIO_VID        ] = 0x%x prio_lookup[PRIO_VID        ] = 0x%x\n",prio_override[2], prio_lookup[2]);
  printf("prio_override[PRIO_PORT       ] = 0x%x prio_lookup[PRIO_PORT       ] = 0x%x\n",prio_override[3], prio_lookup[3]);
#endif
  
  if(verbose) printf("rtu_match: accepted (portmask 0x%08x, priority: %d)\n", rsp->port_mask, rsp->prio);
  
  return 0;
}


void set_mac(mac_addr_t mac, uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f)
{
  mac[0]=a;
  mac[1]=b;
  mac[2]=c;
  mac[3]=d;
  mac[4]=e;
  mac[5]=f;
}



