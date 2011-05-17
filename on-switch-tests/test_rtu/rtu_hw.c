/*
-------------------------------------------------------------------------------
-- Title      : Routing Table Unit Hardware Access Functions
-- Project    : White Rabbit Switch
-------------------------------------------------------------------------------
-- File       : rtu_hw.c
-- Authors    : Maciej Lipinski (maciej.lipinski@cern.ch)
-- Company    : CERN BE-CO-HT
-- Created    : 2010-06-30
-- Last update: 2010-06-30
-- Description: This file contain functions which interface H/W

*/
#include <stdio.h>
#include <hw/switch_hw.h>
#include <hw/clkb_io.h>
#include "wrsw_rtu.h"
#include "rtu_sim.h"
#include "rtu_hw.h"
#include "rtu_test_main.h"


int hw_test_port(int port)
{
 
	uint32_t value_tab[10];
	uint32_t value;
	int i,j;
	
	 printf("Read MACIC: 0x%x\n",_fpga_readl(0x00000 + 0x0));
	 printf("Read REVID: 0x%x\n",_fpga_readl(0x00000 + 0x4));
	 printf("Read HASH : 0x%x\n",_fpga_readl(0x00000 + 0x8));
	 
	

	
	printf("====== T E S T=======\n");
	for(j=0x20000;j<0xe0000;j=j+0x20000)
	{
	  printf("======= next =======\n");
	  for(i=j;i<0x20000;i=i+4)
	  {
	    value = _fpga_readl(j + i);
	    usleep(10000);
	    if(value != 0x0)
	      printf("Read value[addr=0x%x] = 0x%x\n",i,value);
	  }
	}	  

	
	
}

/*
set active ZBT bank (0 or 1)
*/
int rtu_hw_set_active_htab_bank(uint8_t bank)
{
	if(bank != 0  && bank != 1)
	  return -1;
  
	uint32_t setting = _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_GCR);
	
	if(bank) 
	  setting = FPGA_RTU_GCR_HT_BASEL_1 | setting;
	else
	  setting = (~FPGA_RTU_GCR_HT_BASEL_1) & setting ;
	
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_GCR, setting ) ;
	
	usleep(1000);
	
	// read the settings
	//printf("RTU Global setting: 0x%x\n", _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_GCR));
	
	return 1;
}
/*
set active CAM bank (0 or 1)
*/
int rtu_hw_set_active_hcam_bank(uint8_t bank)
{
	if(bank != 0  && bank != 1)
	  return -1;
  
	uint32_t setting = _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_GCR);
	
	if(bank) 
	  setting = FPGA_RTU_GCR_HCAM_BSEL_1 | setting;
	else
	  setting = (~FPGA_RTU_GCR_HCAM_BSEL_1) & setting ;
	
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_GCR, setting ) ;
	
	usleep(1000);
	
	// read the settings
	//printf("RTU Global setting: 0x%x\n", _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_GCR));
	
	return 1;
}

/*
write aging htab
*/
int rtu_hw_write_agr_htab(uint32_t addr, uint32_t data)
{
  _fpga_writel(FPGA_BASE_RTU + FPGA_RTU_ARAM_MAIN + 4*addr, data) ;
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
int rtu_hw_set_fixed_prio_on_port(int port, uint8_t prio, uint8_t set)
{
    	uint32_t port_reg_addr;
	uint32_t setting;
	switch(port){
	  case 0:
	    port_reg_addr = FPGA_RTU_PCR0;
	    break;
	  case 1:
	    port_reg_addr = FPGA_RTU_PCR1;
	    break;
	  case 2:
	    port_reg_addr = FPGA_RTU_PCR2;
	    break;
	  case 3:
	    port_reg_addr = FPGA_RTU_PCR3;
	    break;
	  case 4:
	    port_reg_addr = FPGA_RTU_PCR4;
	    break;
	  case 5:
	    port_reg_addr = FPGA_RTU_PCR5;
	    break;
	  case 6:
	    port_reg_addr = FPGA_RTU_PCR6;
	    break;
	  case 7:
	    port_reg_addr = FPGA_RTU_PCR7;
	    break;
	  case 8:
	    port_reg_addr = FPGA_RTU_PCR8;
	    break;
	  case 9:
	    port_reg_addr = FPGA_RTU_PCR9;
	    break;
	  default:
	    return -1;
	    break;
	};
	setting = _fpga_readl(FPGA_BASE_RTU + port_reg_addr);
	
	if(set)
	{
	    setting = ((0x7 & prio) << 4 ) | ( 0x1 << 3 ) | setting;
	}
	else
	{
	    setting = (( 0x1 << 7 ) | 0x7 ) & setting;
	}
	//write setting to RTU to register representing port
	_fpga_writel(FPGA_BASE_RTU + port_reg_addr , setting);

	return 1;
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
int rtu_hw_rtu_setting(uint8_t global_enable,uint8_t hcam_bank,uint8_t htab_bank, uint16_t hash_poly)
{
	int i;
	uint32_t setting;
	
	if(global_enable) 
	  global_enable = FPGA_RTU_GCR_G_ENA;
	else
	  global_enable = FPGA_RTU_GCR_G_DIS;
	
	if(hcam_bank) 
	  hcam_bank = FPGA_RTU_GCR_HCAM_BSEL_1;
	else
	  hcam_bank = FPGA_RTU_GCR_HCAM_BSEL_0;
	
	if(htab_bank) 
	  htab_bank = FPGA_RTU_GCR_HT_BASEL_1;
	else
	  htab_bank = FPGA_RTU_GCR_HT_BASEL_0;
	
	setting = (hash_poly << 8) | global_enable | hcam_bank | htab_bank;
	
	// enable RTU and enable learning 
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_GCR, setting ) ;
	
	usleep(1000);
	
	// read the settings
	printf("RTU Global setting: 0x%x\n", _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_GCR));
}

int rtu_hw_read_global_setting()
{
	int i;
	uint32_t setting = _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_GCR ) ;;
	
	printf("Reading RTU Global setting:\n");
	
	if(setting & FPGA_RTU_GCR_G_ENA) 
	  printf("RTU enabled; ");
	else
	  printf("RTU disabled; ");
	
	if(setting & FPGA_RTU_GCR_HCAM_BSEL_1) 
	  printf("hcam_bank = 1; ");
	else
	  printf("hcam_bank = 0; ");
	
	if(setting & FPGA_RTU_GCR_HT_BASEL_1) 
	  printf("htab_bank = 1; ");
	else
	  printf("htab_bank = 0; ");
	
	printf("poly= 0x%x\n",0xFFFF & (setting >> 8) );	
}
int rtu_hw_set_hash_poly(uint16_t hash_poly)
{
	int i;
	uint32_t setting = _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_GCR);
	
	
	setting = 0xFF0000FF & setting;
	setting = (hash_poly << 8) | setting;
	
	// enable RTU and enable learning 
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_GCR, setting ) ;
	
	usleep(1000);
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
int rtu_hw_rtu_port_setting(int port, uint32_t setting)
{
	
  	uint32_t port_reg_addr;
	switch(port){
	  case 0:
	    port_reg_addr = FPGA_RTU_PCR0;
	    break;
	  case 1:
	    port_reg_addr = FPGA_RTU_PCR1;
	    break;
	  case 2:
	    port_reg_addr = FPGA_RTU_PCR2;
	    break;
	  case 3:
	    port_reg_addr = FPGA_RTU_PCR3;
	    break;
	  case 4:
	    port_reg_addr = FPGA_RTU_PCR4;
	    break;
	  case 5:
	    port_reg_addr = FPGA_RTU_PCR5;
	    break;
	  case 6:
	    port_reg_addr = FPGA_RTU_PCR6;
	    break;
	  case 7:
	    port_reg_addr = FPGA_RTU_PCR7;
	    break;
	  case 8:
	    port_reg_addr = FPGA_RTU_PCR8;
	    break;
	  case 9:
	    port_reg_addr = FPGA_RTU_PCR9;
	    break;
	  default:
	    return -1;
	    break;
	};
	
	//write setting to RTU to register representing port
	_fpga_writel(FPGA_BASE_RTU + port_reg_addr , setting);
  	
	usleep(1000);
	
	printf("RTU setting for port %d: 0x%x \n",port,_fpga_readl(FPGA_BASE_RTU + port_reg_addr));
	
	return 1;

}

/*
Set basic RTU configuration
TODO: make nice setting functions, put setting to data file
TODO: enabling learning should be separate for each port
*/
int rtu_hw_set()
{
	int i;
	// enable RTU and enable learning 
	rtu_hw_rtu_setting( 1 /*global_enable*/, \
	                    0 /*hcam_bank*/    , \
	                    0 /*htab_bank*/    , \
	                    HW_POLYNOMIAL_CCITT);
	
	for(i=0;i<PORT_NUMBER;i++)
	   rtu_hw_rtu_port_setting(i, FPGA_RTU_PCR_LEARN_EN | FPGA_RTU_PCR_PASS_ALL);
	  
	return 1;

}


/*
Writes htab entry into zbt sram 

*/
int rtu_hw_write_htab_entry(	uint8_t  valid,         uint8_t end_of_bucket,    uint8_t  is_bpdu,        uint8_t go_to_cam , \
		uint8_t  fid,           uint16_t mac_hi,          uint32_t mac_lo, \
		uint16_t cam_addr,      uint8_t drop_when_source, uint8_t  drop_when_dest, uint8_t drop_when_unmatched_src_ports, \
		uint8_t prio_src,       uint8_t has_prio_src,     uint8_t  prio_override_src, \
		uint8_t prio_dst,       uint8_t has_prio_dst,     uint8_t  prio_override_dst, \
                uint16_t port_mask_src, uint16_t port_mask_dst,   uint32_t last_access ,   uint32_t zbt_addr)
{
	uint32_t tmp;
	//write address
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_MFIFO_R0,0x00000001);			//hw_addr = 0x12

	
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_MFIFO_R1,(0x0001FFFF & zbt_addr)); 	//hw_addr = 0x13

	//write data
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_MFIFO_R0,0x00000000);			//hw_addr = 0x12
	usleep(10000);
	tmp = ((0xFFFF & mac_hi) << 16) | ((0xFF & fid)<< 4 ) | (( 0x1 & go_to_cam) << 3 ) | (( 0x1 & is_bpdu) << 2 ) | \
	      (( 0x1 & end_of_bucket) << 1 )| ( 0x1 & valid );
	      
	//w0
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_MFIFO_R1, tmp);			//hw_addr = 0x13

	
	//w1
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_MFIFO_R1, mac_lo);			//hw_addr = 0x13

	
	tmp = (( 0x1 & drop_when_dest) << 28) | ((0x1 & prio_override_dst)<< 27 ) | (( 0x7 & prio_dst) << 24 ) | (( 0x1 & has_prio_dst) << 23 ) | \
	      (( 0x1 & drop_when_unmatched_src_ports) << 22 ) | ( (0x1 & drop_when_source) << 21) | ( (0x1 & prio_override_src) << 20) | \
	      (( 0x7 & prio_src) << 17)| ( (0x1 & has_prio_src) << 16)| ( 0x1FF & cam_addr);
	
	//w2	      
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_MFIFO_R1, tmp);			//hw_addr = 0x13

	
	//w3	
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_MFIFO_R1, ((0xFFFF & port_mask_dst) << 16) | (0xFFFF & port_mask_src) );			//hw_addr = 0x13


	//w4
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_MFIFO_R1, last_access);									//hw_addr = 0x13


	//w5	
	//_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_MFIFO_R1, 0xcccccccc);
	return 1;
}
/*
Writes hcam entry to mem
*/
int rtu_hw_write_hcam_entry(	uint8_t  valid,         uint8_t end_of_bucket,    uint8_t  is_bpdu,        uint8_t go_to_cam , \
		uint8_t  fid,           uint16_t mac_hi,          uint32_t mac_lo, \
		uint16_t cam_addr,      uint8_t drop_when_source, uint8_t  drop_when_dest, uint8_t drop_when_unmatched_src_ports, \
		uint8_t prio_src,       uint8_t has_prio_src,     uint8_t  prio_override_src, \
		uint8_t prio_dst,       uint8_t has_prio_dst,     uint8_t  prio_override_dst, \
                uint16_t port_mask_src, uint16_t port_mask_dst,   uint32_t last_access ,   uint32_t mem_addr, int bank)
{
	uint32_t tmp;
	//write address
	
//	if(bank == 0)
	  mem_addr = 4*mem_addr;
//	else if(bank == 1)
//	  mem_addr = 4*mem_addr + 256; //256 = 2^8
	
	tmp = ((0xFFFF & mac_hi) << 16) | ((0xFF & fid)<< 4 ) | (( 0x1 & go_to_cam) << 3 ) | (( 0x1 & is_bpdu) << 2 ) | \
	      (( 0x1 & end_of_bucket) << 1 )| ( 0x1 & valid );

	
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_HCAM + mem_addr , tmp);

	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_HCAM + mem_addr + 4*0x1, mac_lo);			//hw_addr = 0x13
	
	
	tmp = (( 0x1 & drop_when_dest) << 28) | ((0x1 & prio_override_dst)<< 27 ) | (( 0x7 & prio_dst) << 24 ) | (( 0x1 & has_prio_dst) << 23 ) | \
	      (( 0x1 & drop_when_unmatched_src_ports) << 22 ) | ( (0x1 & drop_when_source) << 21) | ( (0x1 & prio_override_src) << 20) | \
	      (( 0x7 & prio_src) << 17)| ( (0x1 & has_prio_src) << 16)| ( 0x1FF & cam_addr);
	      
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_HCAM + mem_addr + 4*0x2, tmp);			//hw_addr = 0x13
	
	
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_HCAM + mem_addr + 4*0x3 , ((0xFFFF & port_mask_dst) << 16) | (0xFFFF & port_mask_src) );			//hw_addr = 0x13
	
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_HCAM + mem_addr + 4*0x4, last_access);									//hw_addr = 0x13

	return 1;
}
/*

writes entry to vlan table

*/
int rtu_hw_write_vlan_entry(int addr, uint32_t port_mask,uint8_t fid,uint8_t prio,int has_prio,int prio_override,int drop)
{

	//                                                 ADDR |                MASK  |     FID     |             has_prio     |             PRIO     |                   override    |  drop
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_VLAN_TAB + 4*addr , (0xFFFF & port_mask) | (fid << 16) | ((0x1 & has_prio) << 26) | ((0x7 & prio) << 27) | ((0x1 & prio_override) << 30) | ((0x1 & drop) << 31) ) ;

	return 1;
}
/*
write aging register for CAM
*/
int rtu_hw_write_agr_hcam(uint32_t data)
{
  	// write agr_hcam for testing
	
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_AGR_HCAM, data) ;
	usleep(1000);
	fprintf(stderr, "arg_hcam filled in.... \n"); 
}
/*
read aging register for cam
*/
uint32_t rtu_hw_read_arg_hcam()
{
  	// write agr_hcam for testing
	uint32_t ret = _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_AGR_HCAM);
	printf( "arg_hcam : 0x%8x \n",ret ); 
	return ret;
}

//////////////////////////////////// cleaning mems ////////////////////////////////////
/*
cleans cam mem
*/

int rtu_hw_clean_cam_entry(uint32_t mem_addr)
{
	uint32_t tmp;
	//write address
	
	mem_addr = 4*mem_addr;
	
	
	
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_HCAM + mem_addr ,        0x00000000);

	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_HCAM + mem_addr + 4*0x1, 0x00000000);			
	
	
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_HCAM + mem_addr + 4*0x2, 0x00000000);			
	
	
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_HCAM + mem_addr + 4*0x3 , 0x00000000 );			
	
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_HCAM + mem_addr + 4*0x4, 0x00000000);									//hw_addr = 0x13
	
	return 1;
}
/*
clean entire hcam
*/
int rtu_hw_clean_hcam(int active_bank)
{
	int i;
	for(i=0;i<256 ;i=i+8)
	    rtu_hw_clean_cam_entry(i);
	
	rtu_hw_set_active_hcam_bank((active_bank == 0 ? 1 : 0));
	
	for(i=0;i<256 ;i=i+8)
	    rtu_hw_clean_cam_entry(i);
	
	rtu_hw_set_active_hcam_bank(active_bank);	
	
	fprintf(stderr, "hcam zeroed.... \n"); 

	return 1;
}

/*
cleans vlan tab, it sets all the entries to drop !!!
*/
int rtu_hw_clean_vlan_tab()
{
    int i;
    //all except the first one should drap packages by default (except first because this is how Tomek did it....)
    for(i=1;i<4096 ; i++)
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_VLAN_TAB + 4*i , 0x80000000 ) ; // by default drop all -> drop is the bit 31st

    fprintf(stderr, "vlan tab zeroed.... \n");

    return 1;
}
/*
cleans htab entry
*/
int rtu_hw_clean_htab_entry( uint16_t zbt_addr)
{

	// write address
  	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_MFIFO_R0,0x00000001);			//hw_addr = 0x12
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_MFIFO_R1,(0x0000FFFF & zbt_addr)); 	//hw_addr = 0x13
	//write data
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_MFIFO_R0,0x00000000);			//hw_addr = 0x12
	//w0
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_MFIFO_R1, 0x00000000);
	//w1
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_MFIFO_R1, 0x00000000);
	//w2
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_MFIFO_R1, 0x00000000);
	//w3
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_MFIFO_R1, 0x00000000);
	//w4
	_fpga_writel(FPGA_BASE_RTU + FPGA_RTU_MFIFO_R1, 0x00000000);
		
	return 1;
}
/*
cleans entire htab memory
*/
int rtu_hw_clean_htab_memory(int active_bank)
{
	int i;
	
	for(i=0;i< 32768;i=i+8)
	   rtu_hw_clean_htab_entry(i);
    
	rtu_hw_set_active_htab_bank((active_bank == 0 ? 1 : 0));
	
	for(i=0;i< 32768;i=i+8)
	   rtu_hw_clean_htab_entry(i);
	
	rtu_hw_set_active_htab_bank(active_bank);
	
	fprintf(stderr, "htab zeroed.... \n"); 
	
	return 1;
}
/*
clean learning queue by reading all the entries out 
*/
int rtu_hw_clean_learning_queue()
{
  int i;
  for (i=0;i<100;i++)
  {
    	uint32_t dmac_lo = _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_UFIFO_R0);
	uint32_t dmac_hi = _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_UFIFO_R1);
	uint32_t smac_lo = _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_UFIFO_R2);
	uint32_t smac_hi = _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_UFIFO_R3);
	uint32_t tmp =     _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_UFIFO_R4);
  }
  return 1;
}

int rtu_hw_clear_arg_htab()
{
  	int i;
	for(i=0;i< ARAM_WORDS;i++)
	  rtu_hw_write_agr_htab(i, 0x00000000) ;
}

/*
clean all mems
*/
int rtu_hw_clean_mems(int active_htab_bank,int active_hcam_bank)
{
  //clean zbt sram
  rtu_hw_clean_htab_memory(active_htab_bank);
  //clean hcam
  rtu_hw_clean_hcam(active_hcam_bank);
  //clean learning queue
  rtu_hw_clean_learning_queue();
  //clean vlan, by default VLANs are disabled
  rtu_hw_clean_vlan_tab();
  //clean aging hcam register
  rtu_hw_write_agr_hcam(0x00000000); 
  //clean aging htab memory
  rtu_hw_clear_arg_htab();
  
  return 1;	
}




/*
printf workd form aging htab
*/
int rtu_hw_printf_agr_htab(uint32_t addr)
{
        printf("arg_htab[addr=0x%x] = 0x%x \n", addr, _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_ARAM_MAIN + 4*addr)) ;
	return 1;
}
/*
read word from arging htab
*/
uint32_t rtu_hw_read_agr_htab(uint32_t addr)
{
        return _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_ARAM_MAIN + 4*addr) ;
}


/////////////////////////////// request/resonse ////////////////////////////


/*
write request entry to hardware
*/

int rtu_hw_write_request_entry_from_req_table(rtu_request_t rq)
{
	uint32_t tmp =  ((0x1 & rq.has_prio) << 17) | ((0x1 & rq.has_vid) << 16) | \
	((0x7 & rq.prio) << 12) | (0xFFF & rq.vid) ;
	uint32_t port_addr;
	switch(rq.port_id){
	  case 0:
	    port_addr = FPGA_BASE_PORT0;
	    break;
	  case 1:
	    port_addr = FPGA_BASE_PORT1;
	    break;
	  case 2:
	    port_addr = FPGA_BASE_PORT2;
	    break;
	  case 3:
	    port_addr = FPGA_BASE_PORT3;
	    break;
	  case 4:
	    port_addr = FPGA_BASE_PORT4;
	    break;
	  case 5:
	    port_addr = FPGA_BASE_PORT5;
	    break;
	  case 6:
	    port_addr = FPGA_BASE_PORT6;
	    break;
	  case 7:
	    port_addr = FPGA_BASE_PORT7;
	    break;
	  case 8:
	    port_addr = FPGA_BASE_PORT8;
	    break;
	  case 9:
	    port_addr = FPGA_BASE_PORT9;
	    break;
	  default:
	    return -1;
	    break;
	};
	
	//destination mac
	_fpga_writel(port_addr + 0x0C,               rq.dst_low);
	usleep(10000);
	_fpga_writel(port_addr + 0x10, (0x0000FFFF & rq.dst_high));
	usleep(10000);
	//source mack
	_fpga_writel(port_addr + 0x14,               rq.src_low);
	usleep(10000);
	_fpga_writel(port_addr + 0x18, (0x0000FFFF & rq.src_high));
	usleep(10000);
	// has_prio,has_vid, prio, vid
	_fpga_writel(port_addr + 0x1c, (0x0003FFFF & tmp    ));
	usleep(10000);
	return 1;

}
/*
check count of response (out from RTU) fifo
*/
int rtu_hw_check_response_fifo_cnt_on_port(int port_id )
{

  
  	uint32_t tmp;
	switch(port_id)
	{
	  case 0:
	    tmp =  _fpga_readl(FPGA_BASE_PORT0 + 0x8);
	    break;
	  case 1:
	    tmp =  _fpga_readl(FPGA_BASE_PORT1 + 0x8);
	    break;
	  case 2:
	    tmp =  _fpga_readl(FPGA_BASE_PORT2 + 0x8);
	    break;
	  case 3:
	    tmp =  _fpga_readl(FPGA_BASE_PORT3 + 0x8);
	    break;
	  case 4:
	    tmp =  _fpga_readl(FPGA_BASE_PORT4 + 0x8);
	    break;
	  case 5:
	    tmp =  _fpga_readl(FPGA_BASE_PORT5 + 0x8);
	    break;
	  case 6:
	    tmp =  _fpga_readl(FPGA_BASE_PORT6 + 0x8);
	    break;
	  case 7:
	    tmp =  _fpga_readl(FPGA_BASE_PORT7 + 0x8);
	    break;
	  case 8:
	    tmp =  _fpga_readl(FPGA_BASE_PORT8 + 0x8);
	    break;
	  case 9:
	    tmp =  _fpga_readl(FPGA_BASE_PORT9 + 0x8);
	    break;
	  default:
	    return -1;
	    break;
	}
	
	return (0x7F & tmp );
	
}
/*
check count of response (out from RTU) fifo
*/
int rtu_hw_read_test_reg_on_port(int port_id )
{

  
  	uint32_t tmp;
	switch(port_id)
	{
	  case 0:
	    tmp =  _fpga_readl(FPGA_BASE_PORT0 + 0x0);
	    break;
	  case 1:
	    tmp =  _fpga_readl(FPGA_BASE_PORT1 + 0x0);
	    break;
	  case 2:
	    tmp =  _fpga_readl(FPGA_BASE_PORT2 + 0x0);
	    break;
	  case 3:
	    tmp =  _fpga_readl(FPGA_BASE_PORT3 + 0x0);
	    break;
	  case 4:
	    tmp =  _fpga_readl(FPGA_BASE_PORT4 + 0x0);
	    break;
	  case 5:
	    tmp =  _fpga_readl(FPGA_BASE_PORT5 + 0x0);
	    break;
	  case 6:
	    tmp =  _fpga_readl(FPGA_BASE_PORT6 + 0x0);
	    break;
	  case 7:
	    tmp =  _fpga_readl(FPGA_BASE_PORT7 + 0x0);
	    break;
	  case 8:
	    tmp =  _fpga_readl(FPGA_BASE_PORT8 + 0x0);
	    break;
	  case 9:
	    tmp =  _fpga_readl(FPGA_BASE_PORT9 + 0x0);
	    break;
	  default:
	    return -1;
	    break;
	}
	
	return (0xFFFFFFFF & tmp );
	
}
/*
check count of request (input to RTU) fifo
*/
int rtu_hw_check_request_fifo_cnt_on_port(int port_id )
{

  
  	uint32_t tmp;
	switch(port_id)
	{
	  case 0:
	    tmp =  _fpga_readl(FPGA_BASE_PORT0 + 0x20);
	    break;
	  case 1:
	    tmp =  _fpga_readl(FPGA_BASE_PORT1 + 0x20);
	    break;
	  case 2:
	    tmp =  _fpga_readl(FPGA_BASE_PORT2 + 0x20);
	    break;
	  case 3:
	    tmp =  _fpga_readl(FPGA_BASE_PORT3 + 0x20);
	    break;
	  case 4:
	    tmp =  _fpga_readl(FPGA_BASE_PORT4 + 0x20);
	    break;
	  case 5:
	    tmp =  _fpga_readl(FPGA_BASE_PORT5 + 0x20);
	    break;
	  case 6:
	    tmp =  _fpga_readl(FPGA_BASE_PORT6 + 0x20);
	    break;
	  case 7:
	    tmp =  _fpga_readl(FPGA_BASE_PORT7 + 0x20);
	    break;
	  case 8:
	    tmp =  _fpga_readl(FPGA_BASE_PORT8 + 0x20);
	    break;
	  case 9:
	    tmp =  _fpga_readl(FPGA_BASE_PORT9 + 0x20);
	    break;
	  default:
	    return -1;
	    break;
	}
	
	return (0x7F & tmp );
	
}
/*
checks if there is response on the port
*/
int rtu_hw_check_if_answer_on_port(int port_id )
{

  
  	uint32_t tmp;
	switch(port_id)
	{
	  case 0:
	    tmp =  _fpga_readl(FPGA_BASE_PORT0 + 0x8);
	    break;
	  case 1:
	    tmp =  _fpga_readl(FPGA_BASE_PORT1 + 0x8);
	    break;
	  case 2:
	    tmp =  _fpga_readl(FPGA_BASE_PORT2 + 0x8);
	    break;
	  case 3:
	    tmp =  _fpga_readl(FPGA_BASE_PORT3 + 0x8);
	    break;
	  case 4:
	    tmp =  _fpga_readl(FPGA_BASE_PORT4 + 0x8);
	    break;
	  case 5:
	    tmp =  _fpga_readl(FPGA_BASE_PORT5 + 0x8);
	    break;
	  case 6:
	    tmp =  _fpga_readl(FPGA_BASE_PORT6 + 0x8);
	    break;
	  case 7:
	    tmp =  _fpga_readl(FPGA_BASE_PORT7 + 0x8);
	    break;
	  case 8:
	    tmp =  _fpga_readl(FPGA_BASE_PORT8 + 0x8);
	    break;
	  case 9:
	    tmp =  _fpga_readl(FPGA_BASE_PORT9 + 0x8);
	    break;
	  default:
	    return -1;
	    break;
	}
	
	if (0x1 & (tmp >> 17))
	{
	  #ifdef DEBUG_READING_RESPONSES
	  printf("PORT %d empty\n",port_id);
	  #endif
	  return 0;
	}
	else
	{
	  #ifdef DEBUG_READING_RESPONSES
	  printf("PORT %d not empty, %d responses waiting to be read\n",port_id,(tmp & 0x3F));
	  #endif
	  return 1;
	}
	
}
/*
read answer to a request from H/W
need to specify port
*/
int rtu_hw_read_response_from_port(rtu_response_t *rsp, int port_id )
{

  
  	uint32_t tmp;
	switch(port_id)
	{
	  case 0:
	    tmp =  _fpga_readl(FPGA_BASE_PORT0 + 0x4);
	    break;
	  case 1:
	    tmp =  _fpga_readl(FPGA_BASE_PORT1 + 0x4);
	    break;
	  case 2:
	    tmp =  _fpga_readl(FPGA_BASE_PORT2 + 0x4);
	    break;
	  case 3:
	    tmp =  _fpga_readl(FPGA_BASE_PORT3 + 0x4);
	    break;
	  case 4:
	    tmp =  _fpga_readl(FPGA_BASE_PORT4 + 0x4);
	    break;
	  case 5:
	    tmp =  _fpga_readl(FPGA_BASE_PORT5 + 0x4);
	    break;
	  case 6:
	    tmp =  _fpga_readl(FPGA_BASE_PORT6 + 0x4);
	    break;
	  case 7:
	    tmp =  _fpga_readl(FPGA_BASE_PORT7 + 0x4);
	    break;
	  case 8:
	    tmp =  _fpga_readl(FPGA_BASE_PORT8 + 0x4);
	    break;
	  case 9:
	    tmp =  _fpga_readl(FPGA_BASE_PORT9 + 0x4);
	    break;
	  default:
	    return -1;
	    break;
	}
	
	rsp->port_id   = port_id;
	rsp->drop      = 0x1 & (tmp >> 11);
	rsp->port_mask = (uint32_t)(0x7FF & tmp);
        rsp->prio      = (uint8_t)(0x7 & tmp >> 12);
	#ifdef DEBUG_READING_RESPONSES
	printf("port_id = %d, drop = 0x%x, mask = 0x%x, prio = 0x%x\n",rsp->port_id,rsp->drop,rsp->port_mask,rsp->prio );
	#endif
	return 1;
}
/*
match using RTU (only one request at a time
*/
int rtu_hw_match(rtu_request_t rq, rtu_response_t *rsp)
{

    rtu_hw_write_request_entry_from_req_table(rq);
    usleep(10000);
    rtu_hw_read_response_from_port(rsp, rq.port_id);
    return 1;
}
int rtu_hw_read_learning_queue_cnt()
{

  return (0x7F & _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_UFIFO_CSR));
 
}

/*

reading one entry from learing queue

*/
int rtu_hw_read_learning_queue(rtu_request_t *rq)
{
	


	//destination mac
	uint32_t dmac_lo = _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_UFIFO_R0);
	uint32_t dmac_hi = _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_UFIFO_R1);
	uint32_t smac_lo = _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_UFIFO_R2);
	uint32_t smac_hi = _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_UFIFO_R3);
	uint32_t tmp =     _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_UFIFO_R4);

	//info
	rq->port_id  = 0xF & (tmp >> 16);
	rq->has_prio = 0x1 & (tmp >> 21);
	rq->prio     = 0x7 & (tmp >> 12);
	rq->has_vid  = 0x1 & (tmp >> 20);
	rq->vid      = 0xFFF & tmp; 
	rq->dst_low   = dmac_lo;
	rq->dst_high  = dmac_hi;
	rq->src_low   = smac_lo;
	rq->src_high  = smac_hi;
	

	

}

int rtu_hw_dbg_hcam( uint32_t mem_addr)
{

	uint32_t w0,w1,w2,w3,w4;

	mem_addr = 4*mem_addr;
	
	w0 = _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_HCAM + mem_addr);
	
	printf("hcam[0x%x].[0] = 0x%x\n",mem_addr,w0);

	w1 = _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_HCAM + mem_addr + 4*0x1);			
	
	printf("hcam[0x%x].[1] = 0x%x\n",mem_addr,w1);
	
	w2 = _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_HCAM + mem_addr + 4*0x2);
	
	printf("hcam[0x%x].[2] = 0x%x\n",mem_addr,w2);
	
	w3 = _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_HCAM + mem_addr + 4*0x3 );
	
	printf("hcam[0x%x].[3] = 0x%x\n",mem_addr,w3);
	
	w4 = _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_HCAM + mem_addr + 4*0x4);		
	
	printf("hcam[0x%x].[4] = 0x%x\n",mem_addr,w4);
}

int rtu_hw_read_arg_htab_changes(changed_aging_htab_word_t hw_agr_htab[])
{
   int i;
   int hw_cnt;
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
  
    return hw_cnt;
}
int rtu_hw_read_learning_queue_2(request_fifo_t *learning_queue_hw)
{
  int i;
  //request_fifo_t learning_queue_hw;
  learning_queue_hw->count = rtu_hw_read_learning_queue_cnt();
  
  for(i=0;i<=learning_queue_hw->count;i++)
      rtu_hw_read_learning_queue(&(learning_queue_hw->data[i]));
  
  
 return learning_queue_hw->count;
   

}
int rtu_hw_read_hcam_entry( uint32_t mem_addr)
{
   int active_bank = CFG.cam_bank;//(CFG.cam_bank == 0 ? 1 : 0);
 
  printf("hw_hcam[bank = %d][addr = 0x%x].w0 = 0x%x; ",active_bank , mem_addr, _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_HCAM + 4*mem_addr ));
  printf("w1 = 0x%x; ", _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_HCAM + 4*mem_addr + 4*0x1));
  printf("w2 = 0x%x; ", _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_HCAM + 4*mem_addr + 4*0x2));
  printf("w3 = 0x%x; ", _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_HCAM + 4*mem_addr + 4*0x3));
  printf("w4 = 0x%x; ", _fpga_readl(FPGA_BASE_RTU + FPGA_RTU_HCAM + 4*mem_addr + 4*0x4));


}
