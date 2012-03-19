/*
-------------------------------------------------------------------------------
-- Title      : Routing Table Unit Test Data
-- Project    : White Rabbit Switch
-------------------------------------------------------------------------------
-- File       : rtu_test_data.c
-- Authors    : Maciej Lipinski (maciej.lipinski@cern.ch)
-- Company    : CERN BE-CO-HT
-- Created    : 2010-06-30
-- Last update: 2010-06-30
-- Description: Here we :
--                 - fill in memories with data (both soft and h/w)
--                 - fill in request table, this is used to test RTU, 
--                   answers to these requests (from S/W and H/W are compared


*/

#include <stdio.h>
#include "rtu_sim.h"
#include "rtu_hw.h"
#include "rtu_test_main.h"
/*
fill in global settings
*/
int rtu_fill_in_global_settings(uint8_t hcam_bank,uint8_t htab_bank,uint16_t hash_poly)
{
	uint8_t global_enable;
	

	printf("Settings(global)   : ");
	
	if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 3) 
	//if(0)
	{
	      global_enable = 0x1; 
	      printf("RTU enabled; ");
	}
	else 
	{
	      global_enable = 0x0;
	      printf("RTU disabled; ");
	}
	
	printf("CAM active bank = %d, ZBT active bank = %d; ",hcam_bank, htab_bank);
	printf("HASH POLY: 0x%x\n",hash_poly);
		// global settings
	rtu_setting( global_enable /*global_enable*/, \
	             hcam_bank     /*hcam_bank*/    , \
	             htab_bank     /*htab_bank*/    , \
	             hash_poly      );
		     
}

/*
write settings to RTU and ports

*/

int rtu_fill_in_port_settings(int port)
{
	int i;
	
	int learn_en;
	int pass_all;
	int pass_bpdu;
	int fixed_prio_en;
	int fixed_prio_value;
	int b_unrec ;
	
	uint32_t setting;

	printf("Settings(Port %d): ",port);
	

	if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 2) 
	
	{
	      pass_all = FPGA_RTU_PCR_PASS_ALL; 
	      printf("port enabled, ");
	}
	else 
	{
	      pass_all = 0x0;
	      printf("port disabled, ");
	}	
	
	if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 2)  // tested
	{
	      learn_en = FPGA_RTU_PCR_LEARN_EN; 
	      printf("learning enabled, ");
	}
	else 
	{
	      learn_en = 0x0;
	      printf("learning disabled, ");
	}

	if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) 
	{
	      pass_bpdu = FPGA_RTU_PCR_PASS_BPDU; 
	      printf("pass_bpdu enabled, ");
	}
	else 
	{
	      pass_bpdu = 0x0;
	      printf("pass_bpdu disabled, ");
	}

	if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) 
	{
	      fixed_prio_en = FPGA_RTU_PCR_FIX_PRIO; 
	      printf("fixed port priority: ");
	}
	else 
	{
	      fixed_prio_en = 0x0;
	      printf("no port priority, ");
	}
	
	if(fixed_prio_en)
	{
	  fixed_prio_value = (int) ((float)8 * (rand() / (RAND_MAX + 1.0)));
	  printf("0x%x; ", fixed_prio_value);
	}
	else
	{
	  fixed_prio_value = 0;
	  printf("; ");
	}
	
	if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 3) 
	{
	      b_unrec = FPGA_RTU_PCR_B_UNREC; 
	      printf("unrec broadcasted\n");
	}
	else 
	{
	      b_unrec = 0x0;
	      printf("unrec packages dropped\n");
	}
	
        setting = ((0x7 & fixed_prio_value) << 4) | learn_en | pass_all | pass_bpdu | fixed_prio_en | b_unrec;

        rtu_port_setting(port, setting);
	
  
}
int rtu_fill_settings(uint8_t hcam_bank,uint8_t htab_bank, uint16_t hash_poly)
{
    int i;
#ifdef RANDOM_SETTINGS

    
    //random global settings
    rtu_fill_in_global_settings(hcam_bank,htab_bank,hash_poly);
    
    // random settings for each port
    for(i=0;i<PORT_NUMBER;i++)
      rtu_fill_in_port_settings(i);
    
#else

    // global settings
    rtu_setting( 1 /*global_enable*/, \
	         0 /*hcam_bank*/    , \
	         0 /*htab_bank*/    , \
		 USE_POLYNOMIAL);
    printf("Settings(global)   : RTU enabled, CAM active bank = 0, ZBT active bank = 0\n");
    //settings for each port: enable ports and learning on ports
    for(i=0;i<PORT_NUMBER;i++)
	rtu_port_setting(i, FPGA_RTU_PCR_LEARN_EN | FPGA_RTU_PCR_PASS_ALL);
    printf("Settings(all ports): port enabled, learning enabled, pass_bpdu disabled, fixed port priority disabled\n");
#endif

}


/*
write htab (zbt sram) with data.
*/

int rtu_fill_in_htab()
{
  
  
	int i;
	int 	 valid;
	int 	 end_of_bucket;
	int 	 is_bpdu;
	int 	 go_to_cam;
	uint8_t  fid;
	uint16_t mac_hi;
	uint32_t mac_lo;
	uint8_t  cam_addr; 
	int 	 drop_when_source;
	int 	 drop_when_dest;
	int 	 drop_when_unmatched_src_ports ;
	uint8_t  prio_src;
	uint8_t  has_prio_src;
	uint8_t  prio_override_src;
	uint8_t  prio_dst;
	int      has_prio_dst;
	int      prio_override_dst;
	uint16_t port_mask_src;
	uint16_t port_mask_dst;
	uint32_t last_access;
	int	 bank;
	int 	 bucket;
	
	int      cam_entries_number =0;
	
	 // fill in HTAB																					   | for hash calculation    |
		    //       |valid|end_of|is_bpdu|go_tc| fid|      MAC        |cam_adr|drop_ws|drop_wd|drop_us|prio_src|has_ps|prio_ors|prio_dst|has_pd|prio_ord|mask_s| mask_d|last_access|bank | bucket| FID  | MAC addr         | 
	   
#ifdef RANDOM_DATA		    

#ifndef AGING_PROBLEM
#ifndef DST_HAS_PRIO_PROBLEM
	//for(i=0;i<RTU_ENTRIES/4;i++)
	for(i=0;i<RTU_ENTRIES/20;i++)
	{
	 
	  if(i%100==0)
	    fprintf(stderr, "."); 
	  
	  
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5)
	  {
	    valid = 0x1;
	  }
	  else
	  {
	    //if not valid, no use to go further
	    valid = 0x0;
	    continue;
	  }
	  
	  end_of_bucket = 0; //alwasy 0 in htab
	  
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) is_bpdu = 0x1; else is_bpdu = 0x0;
	  
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) go_to_cam = 0x1; else go_to_cam = 0x0;
	  
	  
	  fid  =  (uint8_t) (256.0 * (rand() / (RAND_MAX + 1.0))); // fid_width = 8, 2^8 = 256
	  
	  mac_hi = (uint16_t) (65536.0 * (rand() / (RAND_MAX + 1.0))); // 2^16 = 65536
	  mac_lo = (uint32_t) (4294967296.0 * (rand() / (RAND_MAX + 1.0))); // 2^32 = 4294967296
	  cam_addr = (int) ((float)(CAM_ENTRIES/8) * (rand() / (RAND_MAX + 1.0))); 
	  cam_addr = cam_addr * 8;
	  
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) drop_when_source = 0x1; else drop_when_source = 0x0;
	  
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) drop_when_dest = 0x1; else drop_when_dest = 0x0;
	  
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) drop_when_dest = 0x1; else drop_when_dest = 0x0; 
	   
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) drop_when_unmatched_src_ports = 0x1; else drop_when_unmatched_src_ports = 0x0; 
	  

	  prio_src = (int) (8.0 * (rand() / (RAND_MAX + 1.0))); 
	  
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) has_prio_src = 0x1; else has_prio_src = 0x0; 
	  
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) prio_override_src = 0x1; else prio_override_src = 0x0; 
	  

	  prio_dst= (int) (8.0 * (rand() / (RAND_MAX + 1.0))); 
	  
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) has_prio_dst = 0x1; else has_prio_dst = 0x0; 
	  
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) prio_override_dst = 0x1; else prio_override_dst = 0x0; 

	  
	  port_mask_src = (uint16_t) (2048.0 * (rand() / (RAND_MAX + 1.0))); //2^11
	  port_mask_dst = (uint16_t) (2048.0 * (rand() / (RAND_MAX + 1.0))); //2^11
	  last_access   = (uint32_t) (4294967296.0 * (rand() / (RAND_MAX + 1.0))); // 2^32 = 4294967296

	  bucket = 0;	  
	  
	  
	  go_to_cam=0x0;
	  cam_addr =0x0;
	  cam_entries_number += \
	  rtu_write_htab_entry(	valid, end_of_bucket,is_bpdu, go_to_cam , fid, mac_hi, mac_lo,cam_addr, drop_when_source, drop_when_dest, \
				drop_when_unmatched_src_ports, prio_src, has_prio_src, prio_override_src, prio_dst, has_prio_dst, prio_override_dst, \
				port_mask_src, port_mask_dst, last_access , bucket);
	
	
	}
#ifdef CAM_DROP_PROBLEM_2
	//w0
	rtu_write_htab_entry( 0x1 , 0x0  ,   0x1 , 0x0, 0xe9,0xee8a,0x54665408, 0x18  ,  0x0  ,  0x1  , 0x1   ,   0x5  ,  0x1 ,   0x1  ,   0x5   , 0x1 ,   0x1  ,0x07c2, 0x003b, 0x54d66314,  0  , 0    ); 
	//w1
	rtu_write_htab_entry( 0x1 , 0x0  ,   0x1 , 0x0, 0xa4,0x0400,0x57a1993a, 0x18  ,  0x1  ,  0x1  , 0x0   ,   0x3  ,  0x1 ,   0x1  ,   0x2   , 0x1 ,   0x0  ,0x039a, 0x0177, 0x4a797330,  0  , 0    ); 
	//w2
	rtu_write_htab_entry( 0x1 , 0x0  ,   0x1 , 0x0, 0x0e,0xb457,0x4d36620c, 0x08  ,  0x1 ,  0x0  , 0x1   ,   0x4 ,  0x0 ,   0x1  ,   0x4   , 0x1 ,   0x0  ,0x040c, 0x074c, 0xaa0a7b52,  0  , 0    ); 
	//w3
		    //       |valid|end_of|is_bpdu|go_tc| fid|      MAC        |cam_adr|drop_ws|drop_wd|drop_us|prio_src|has_ps|prio_ors|prio_dst|has_pd|prio_ord|mask_s| mask_d|last_access|bank | bucket| FID  | MAC addr  
	rtu_write_htab_entry( 0x1 , 0x0  ,   0x0 , 0x1, 0xf9,0x1bed,0xe1f4a7b2, 0x7cd  , 0x0  ,  0x1  , 0x1   ,   0x6  ,  0x0 ,   0x0  ,   0x7   , 0x0 ,   0x1  ,0x047d, 0x007f, 0x183d8430,  0  , 3    );   
#endif

	 printf("\nentire htab filled in with random data [cam entries: %d ].... \n",cam_entries_number);
#endif  /*DST_HAS_PRIO_PROBLEM*/
#endif  /*AGING_PROBLEM*/
#endif  /*RANDOM_DATA*/
#ifdef PREDEFINED_DATA	
	 // fill in HTAB																					   | for hash calculation    |
		    //       |valid|end_of|is_bpdu|go_tc| fid|      MAC        |cam_adr|drop_ws|drop_wd|drop_us|prio_src|has_ps|prio_ors|prio_dst|has_pd|prio_ord|mask_s| mask_d|last_access|bank | bucket| FID  | MAC addr         | 
	 rtu_write_htab_entry( 0x1 , 0x0  ,   0x0 , 0x0, 0xFF,0xFF00,0x0000FFFF, 0x00  ,  0x0  ,  0x0  , 0x0   ,   0x2  ,  0x1 ,   0x0  ,   0x2   , 0x1 ,   0x0  ,0x000F, 0x000F, 0x01234567,  0  , 0    );   
	 rtu_write_htab_entry( 0x1 , 0x0  ,   0x0 , 0x0, 0xFF,0x1234,0x56789000, 0x00  ,  0x0  ,  0x0  , 0x0   ,   0x2  ,  0x1 ,   0x1  ,   0x4   , 0x1 ,   0x1  ,0x0001, 0x0001, 0x01234567,  0  , 0    );   

#ifdef CAM_DROP_PROBLEM
/*
|=============== zbt_addr = 0x10c0   hash = 0x86=====================================|
|valid = 0x1              | end_of_bucket = 0x0          | is_bpdu = 0x1              |
|fid              = 0x  e9|mac           = 0xee8a54665408| cam_addr= 0x18             |
|drop_when_source = 0x0   |drop_when_dest= 0x1           |dp_w_unmatc_s_p = 0x1       | 
|prio_src         = 0x5   |has_prio_src  = 0x1           |prio_oride_src  = 0x1       |
|prio_dst         = 0x5   |has_prio_dst  = 0x1           |prio_oride_dst  = 0x1       |
|port_mask_src    = 0x 7c2|port_mask_dst = 0x  3b        |last_access     = 0x54d66314|
|bank             = 0x0   |bucket        = 0x0           | go_to_cam = 0x0            |
|^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^|
*/
		    //       |valid|end_of|is_bpdu|go_tc| fid|      MAC        |cam_adr|drop_ws|drop_wd|drop_us|prio_src|has_ps|prio_ors|prio_dst|has_pd|prio_ord|mask_s| mask_d|last_access|bank | bucket| FID  | MAC addr         | 
	 rtu_write_htab_entry( 0x1 , 0x0  ,   0x1 , 0x0, 0xe9,0xee8a,0x54665408, 0x18  ,  0x0  ,  0x1  , 0x1   ,   0x5  ,  0x1 ,   0x1  ,   0x5   , 0x1 ,   0x1  ,0x07c2, 0x003b, 0x54d66314,  0  , 0    ); 
/*
|=============== zbt_addr = 0x10c8   hash = 0x86=====================================|
|valid = 0x1              | end_of_bucket = 0x0          | is_bpdu = 0x1              |
|fid              = 0x  a4|mac           = 0x 40057a1993a| cam_addr= 0x18             |
|drop_when_source = 0x1   |drop_when_dest= 0x1           |dp_w_unmatc_s_p = 0x0       | 
|prio_src         = 0x3   |has_prio_src  = 0x1           |prio_oride_src  = 0x1       |
|prio_dst         = 0x2   |has_prio_dst  = 0x1           |prio_oride_dst  = 0x0       |
|port_mask_src    = 0x 39a|port_mask_dst = 0x 177        |last_access     = 0x4a797330|
|bank             = 0x0   |bucket        = 0x1           | go_to_cam = 0x0            |
|^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^|
*/
		    //       |valid|end_of|is_bpdu|go_tc| fid|      MAC        |cam_adr|drop_ws|drop_wd|drop_us|prio_src|has_ps|prio_ors|prio_dst|has_pd|prio_ord|mask_s| mask_d|last_access|bank | bucket| FID  | MAC addr         | 
	 rtu_write_htab_entry( 0x1 , 0x0  ,   0x1 , 0x0, 0xa4,0x0400,0x57a1993a, 0x18  ,  0x1  ,  0x1  , 0x0   ,   0x3  ,  0x1 ,   0x1  ,   0x2   , 0x1 ,   0x0  ,0x039a, 0x0177, 0x4a797330,  0  , 0    ); 
/*
|=============== zbt_addr = 0x10d0   hash = 0x86=====================================|
|valid = 0x1              | end_of_bucket = 0x0          | is_bpdu = 0x1              |
|fid              = 0x   e|mac           = 0xb4574d36620c| cam_addr= 0x 8             |
|drop_when_source = 0x1   |drop_when_dest= 0x0           |dp_w_unmatc_s_p = 0x1       | 
|prio_src         = 0x4   |has_prio_src  = 0x0           |prio_oride_src  = 0x1       |
|prio_dst         = 0x4   |has_prio_dst  = 0x1           |prio_oride_dst  = 0x0       |
|port_mask_src    = 0x 40c|port_mask_dst = 0x 74c        |last_access     = 0xaa0a7b52|
|bank             = 0x0   |bucket        = 0x2           | go_to_cam = 0x0            |
|^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^|
*/
		    //       |valid|end_of|is_bpdu|go_tc| fid|      MAC        |cam_adr|drop_ws|drop_wd|drop_us|prio_src|has_ps|prio_ors|prio_dst|has_pd|prio_ord|mask_s| mask_d|last_access|bank | bucket| FID  | MAC addr         | 
	 rtu_write_htab_entry( 0x1 , 0x0  ,   0x1 , 0x0, 0x0e,0xb457,0x4d36620c, 0x08  ,  0x1 ,  0x0  , 0x1   ,   0x4 ,  0x0 ,   0x1  ,   0x4   , 0x1 ,   0x0  ,0x040c, 0x074c, 0xaa0a7b52,  0  , 0    ); 
/*
|=============== zbt_addr = 0x10d8   hash = 0x86=====================================|
|valid = 0x1              | end_of_bucket = 0x0          | is_bpdu = 0x0              |
|fid              = 0x  f9|mac           = 0x1bede1f4a7b2| cam_addr= 0x38             |
|drop_when_source = 0x0   |drop_when_dest= 0x1           |dp_w_unmatc_s_p = 0x1       | 
|prio_src         = 0x6   |has_prio_src  = 0x0           |prio_oride_src  = 0x0       |
|prio_dst         = 0x7   |has_prio_dst  = 0x0           |prio_oride_dst  = 0x1       |
|port_mask_src    = 0x 47d|port_mask_dst = 0x  7f        |last_access     = 0x183d8430|
|bank             = 0x0   |bucket        = 0x3           | go_to_cam = 0x0            |
|^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^|
*/
		    //       |valid|end_of|is_bpdu|go_tc| fid|      MAC        |cam_adr|drop_ws|drop_wd|drop_us|prio_src|has_ps|prio_ors|prio_dst|has_pd|prio_ord|mask_s| mask_d|last_access|bank | bucket| FID  | MAC addr         | 
	 rtu_write_htab_entry( 0x1 , 0x0  ,   0x0 , 0x1, 0xf9,0x1bed,0xe1f4a7b2, 0x7cd  ,  0x0  ,  0x1  , 0x1   ,   0x6  ,  0x0 ,   0x0  ,   0x7   , 0x0 ,   0x1  ,0x047d, 0x007f, 0x183d8430,  0  , 0    );   

/*
|=============== cam_addr = 0x414   hash = 0x86=====================================|
|valid = 0x1              | end_of_bucket = 0x0          | is_bpdu = 0x0              |
|fid              = 0x  72|mac           = 0xcaf22e2e52b8| cam_addr= 0x28             |
|drop_when_source = 0x1   |drop_when_dest= 0x1           |dp_w_unmatc_s_p = 0x0       | 
|prio_src         = 0x5   |has_prio_src  = 0x0           |prio_oride_src  = 0x1       |
|prio_dst         = 0x1   |has_prio_dst  = 0x1           |prio_oride_dst  = 0x1       |
|port_mask_src    = 0x 530|port_mask_dst = 0x  ef        |last_access     = 0x475cefb2|
|bank             = 0x0   |bucket        = 0x3           | go_to_cam = 0x0            |
|^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^|
*/	 
	 //                   |valid|end_of|is_bpdu|go_tc| fid|      MAC        |cam_adr|drop_ws|drop_wd|drop_us|prio_src|has_ps|prio_ors|prio_dst|has_pd|prio_ord|mask_s| mask_d|last_access|wr_addr|
	 //rtu_write_hcam_entry ( 0x1 , 0x0  ,   0x1 , 0x0, 0x66,0xdec2,0x4f0e2870, 0x00  ,  0x0  ,  0x0  , 0x1   ,   0x7  ,  0x0 ,   0x0  ,   0x3   , 0x1 ,   0x0  ,0x053a, 0x041e, 0x2567e490, 0x7cd);
	  rtu_write_htab_entry(0x1 , 0x0  ,   0x1 , 0x0, 0x72,0xcaf2,0x2e2e52b8, 0x28  ,  0x1  ,  0x1  , 0x0   ,   0x5  ,  0x0 ,   0x1  ,   0x1   , 0x1 ,   0x1  ,0x0530, 0x00ef, 0x475cefb2,  0  , 0    );   	
	  rtu_write_htab_entry(0x1 , 0x1  ,   0x1 , 0x0, 0x72,0xcdf2,0x2e2e52b8, 0x28  ,  0x1  ,  0x1  , 0x0   ,   0x5  ,  0x0 ,   0x1  ,   0x1   , 0x1 ,   0x1  ,0x0530, 0x00ef, 0x475cefb2,  0  , 0    );   	
	 //some
	 //rtu_write_hcam_entry ( 0x1 , 0x1  ,   0x1 , 0x0, 0xA0,0xFF00,0x0000FFFF, 0x00  ,  0x0  ,  0x0  , 0x0   ,   0x2  ,  0x1 ,   0x0  ,   0x2   , 0x1 ,   0x1  ,0x000F, 0x000F, 0x01234567, 0x7ed);
	 
	 return;
	 
#endif
	 
#ifdef AGING_PROBLEM	 
 	 //problem with aging: 036A  ->> this one
	 rtu_write_htab_entry( 0x1 , 0x0  ,   0x0 , 0x0, 0xf2,0xf976,0x88619328, 0x48  ,  0x0  ,  0x1  , 0x0   ,   0x4  ,  0x1 ,   0x0  ,   0x1   , 0x0 ,   0x0  ,0x0464, 0x06f1, 0xf5cd1e96,  0  , 0    );   
	 
	 //problem with aging: 0x42C
	 rtu_write_htab_entry( 0x1 , 0x0  ,   0x0 , 0x0, 0xE1,0xd842,0xa6b6b8b2, 0x50  ,  0x1  ,  0x1  , 0x1   ,   0x4  ,  0x1 ,   0x1  ,   0x0   , 0x0 ,   0x1  ,0x0386, 0x00af, 0xa00509a6,  0  , 0    );   
	 
	 //problem with aging: 0x792
	 rtu_write_htab_entry( 0x1 , 0x0  ,   0x1 , 0x0, 0xF0,0x492a,0xd5a53aa2, 0x30  ,  0x0  ,  0x0  , 0x1   ,   0x3  ,  0x0 ,   0x0  ,   0x4   , 0x0 ,   0x0  ,0x0098, 0x002c, 0xe5320514,  0  , 0    );   
	   
	return;
#endif
#ifdef DST_HAS_PRIO_PROBLEM
		    //       |valid|end_of|is_bpdu|go_tc| fid|      MAC        |cam_adr|drop_ws|drop_wd|drop_us|prio_src|has_ps|prio_ors|prio_dst|has_pd|prio_ord|mask_s| mask_d|last_access|bank | bucket| FID  | MAC addr         | 
	 rtu_write_htab_entry( 0x1 , 0x0  ,   0x0 , 0x0, 0x71,0x1e92,0xe3201888, 0x68  ,  0x0  ,  0x0  , 0x0   ,   0x61  ,  0x1 ,   0x1  ,   0x6   , 0x0 ,   0x1  ,0x06e4, 0x01dc, 0xdcdea302,  0  , 0    );   


#endif 
	 
   rtu_write_htab_entry_cheat( 0x1 , 0x0  ,   0x0 , 0x0, 0xFF,0x0101,0x01010101, 0x00  ,  0x0  ,  0x0  , 0x0   ,   0x0  ,  0x0 ,   0x0  ,   0x0   , 0x0 ,   0x0  ,0x0000, 0x0000, 0x01234567,  0  , 0     , 0xA0,0x2424,0x24242424);    //0xEAE0);
   rtu_write_htab_entry_cheat( 0x1 , 0x0  ,   0x0 , 0x0, 0xFF,0x0101,0x01010121, 0x00  ,  0x0  ,  0x0  , 0x0   ,   0x0  ,  0x0 ,   0x0  ,   0x0   , 0x0 ,   0x0  ,0x0000, 0x0000, 0x01234567,  0  , 1     , 0xA0,0x2424,0x24242424);    //0xEAE8);
   rtu_write_htab_entry_cheat( 0x1 , 0x0  ,   0x0 , 0x0, 0xFF,0x0101,0x343d0101, 0x00  ,  0x0  ,  0x0  , 0x0   ,   0x0  ,  0x0 ,   0x0  ,   0x0   , 0x0 ,   0x0  ,0x0000, 0x0000, 0x01234567,  0  , 2     , 0xA0,0x2424,0x24242424);    //0xEAF0);
   rtu_write_htab_entry_cheat( 0x1 , 0x0  ,   0x0 , 0x1, 0xFF,0x01bc,0xa1010101, 0x00  ,  0x0  ,  0x0  , 0x0   ,   0x0  ,  0x0 ,   0x0  ,   0x0   , 0x0 ,   0x0  ,0x0000, 0x0000, 0x01234567,  0  , 3     , 0xA0,0x2424,0x24242424);    //0xEAF8);
  
   rtu_write_htab_entry_cheat( 0x1 , 0x0  ,   0x0 , 0x0, 0xFF,0x0101,0x01010101, 0x00  ,  0x0  ,  0x0  , 0x0   ,   0x0  ,  0x0 ,   0x0  ,   0x0   , 0x0 ,   0x0  ,0x0000, 0x0000, 0x01234567,  0  , 0     ,0xFF,0x01bc,0xa1010101);    //0x10A0);
   rtu_write_htab_entry_cheat( 0x1 , 0x0  ,   0x0 , 0x0, 0xFF,0x0101,0x01010121, 0x00  ,  0x0  ,  0x0  , 0x0   ,   0x0  ,  0x0 ,   0x0  ,   0x0   , 0x0 ,   0x0  ,0x0000, 0x0000, 0x01234567,  0  , 1     ,0xFF,0x01bc,0xa1010101);    //0x10A8);
   rtu_write_htab_entry_cheat( 0x1 , 0x0  ,   0x0 , 0x0, 0xFF,0x0101,0x343d0101, 0x00  ,  0x0  ,  0x0  , 0x0   ,   0x0  ,  0x0 ,   0x0  ,   0x0   , 0x0 ,   0x0  ,0x0000, 0x0000, 0x01234567,  0  , 2     ,0xFF,0x01bc,0xa1010101);    //0x10B0);
   rtu_write_htab_entry_cheat( 0x1 , 0x0  ,   0x0 , 0x1, 0xFF,0x01bc,0xa1010101, 0x03  ,  0x0  ,  0x0  , 0x0   ,   0x0  ,  0x0 ,   0x0  ,   0x0   , 0x0 ,   0x0  ,0x0000, 0x0000, 0x01234567,  0  , 3     ,0xFF,0x01bc,0xa1010101);    //0x10B8);
#endif
   fprintf(stderr, "\nsome htab entries filled in with predefined data.... \n");
   
//	 rtu_write_htab_entry( 0x1 , 0x0  ,   0x0 , 0x0, 0xFF,0xFF00,0x0000FFFF, 0x00  ,  0x0  ,  0x0  , 0x0   ,   0x2  ,  0x1 ,   0x0  ,   0x2   , 0x1 ,   0x0  ,0x000F, 0x000F, 0x01234567, 0);    //0xeae0);
//	 rtu_write_htab_entry( 0x1 , 0x0  ,   0x0 , 0x0, 0xFF,0xFF00,0x0000FFFF, 0x00  ,  0x0  ,  0x0  , 0x0   ,   0x2  ,  0x1 ,   0x0  ,   0x2   , 0x1 ,   0x0  ,0x000F, 0x000F, 0x01234567, 0);    //0x9a20);

	 
}

/*
fill in cam mem 
*/


int rtu_fill_in_hcam()
{
  
  	uint16_t i;
	int 	 valid;
	int 	 end_of_bucket;
	int 	 is_bpdu;
	int 	 go_to_cam;
	uint8_t  fid;
	uint16_t mac_hi;
	uint32_t mac_lo;
	uint8_t  cam_addr; 
	int 	 drop_when_source;
	int 	 drop_when_dest;
	int 	 drop_when_unmatched_src_ports ;
	uint8_t  prio_src;
	uint8_t  has_prio_src;
	uint8_t  prio_override_src;
	uint8_t  prio_dst;
	int      has_prio_dst;
	int      prio_override_dst;
	uint16_t port_mask_src;
	uint16_t port_mask_dst;
	uint32_t last_access;

	
	 // fill in HTAB																					   | for hash calculation    |
		    //       |valid|end_of|is_bpdu|go_tc| fid|      MAC        |cam_adr|drop_ws|drop_wd|drop_us|prio_src|has_ps|prio_ors|prio_dst|has_pd|prio_ord|mask_s| mask_d|last_access|bank | bucket| FID  | MAC addr         | 
#ifdef RANDOM_DATA		    
	for(i=0;i<CAM_ENTRIES;i=i+0x8)
	{
	 
	  fprintf(stderr, "."); 
	  valid = 0x1;

	  
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) end_of_bucket = 0x1; else end_of_bucket = 0x0;
	  
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) is_bpdu = 0x1; else is_bpdu = 0x0;
	  
	  go_to_cam=0x0;
	  
	  fid  =  (uint8_t) (256.0 * (rand() / (RAND_MAX + 1.0))); // fid_width = 8, 2^8 = 256
	  
	  mac_hi = (uint16_t) (65536.0 * (rand() / (RAND_MAX + 1.0))); // 2^16 = 65536
	  mac_lo = (uint32_t) (4294967296.0 * (rand() / (RAND_MAX + 1.0))); // 2^32 = 4294967296
	  
	  cam_addr = 0x0;
	  
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) drop_when_source = 0x1; else drop_when_source = 0x0;
	  
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) drop_when_dest = 0x1; else drop_when_dest = 0x0;
	  
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) drop_when_dest = 0x1; else drop_when_dest = 0x0; 
	   
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) drop_when_unmatched_src_ports = 0x1; else drop_when_unmatched_src_ports = 0x0; 
	  

	  prio_src = (int) (8.0 * (rand() / (RAND_MAX + 1.0))); 
	  
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) has_prio_src = 0x1; else has_prio_src = 0x0; 
	  
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) prio_override_src = 0x1; else prio_override_src = 0x0; 
	  

	  prio_dst= (int) (8.0 * (rand() / (RAND_MAX + 1.0))); 
	  
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) has_prio_dst = 0x1; else has_prio_dst = 0x0; 
	  
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) prio_override_dst = 0x1; else prio_override_dst = 0x0; 

	  
	  port_mask_src = (uint16_t) (2048.0 * (rand() / (RAND_MAX + 1.0))); //2^11
	  port_mask_dst = (uint16_t) (2048.0 * (rand() / (RAND_MAX + 1.0))); //2^11
	  last_access   = (uint32_t) (4294967296.0 * (rand() / (RAND_MAX + 1.0))); // 2^32 = 4294967296

	  
	  rtu_write_hcam_entry(	valid, end_of_bucket,is_bpdu, go_to_cam , fid, mac_hi, mac_lo,cam_addr, drop_when_source, drop_when_dest, \
				drop_when_unmatched_src_ports, prio_src, has_prio_src, prio_override_src, prio_dst, has_prio_dst, prio_override_dst, \
				port_mask_src, port_mask_dst, last_access,(uint16_t)i );
	
	
	}

	printf("\nentire hcam filled in with random data.... \n");
#endif
#ifdef PREDEFINED_DATA  
  	 // fill in HCAM
	 //                   |valid|end_of|is_bpdu|go_tc| fid|      MAC        |cam_adr|drop_ws|drop_wd|drop_us|prio_src|has_ps|prio_ors|prio_dst|has_pd|prio_ord|mask_s| mask_d|last_access|wr_addr|
	 rtu_write_hcam_entry ( 0x1 , 0x0  ,   0x0 , 0x0, 0xA0,0xFF00,0x0000FFFF, 0x00  ,  0x0  ,  0x0  , 0x0   ,   0x2  ,  0x1 ,   0x0  ,   0x2   , 0x1 ,   0x1  ,0x000F, 0x000F, 0x01234567, 0x0000);
	 rtu_write_hcam_entry ( 0x1 , 0x0  ,   0x0 , 0x0, 0xA0,0xF500,0x0000FFFF, 0x00  ,  0x0  ,  0x0  , 0x0   ,   0x2  ,  0x1 ,   0x0  ,   0x4   , 0x1 ,   0x1  ,0x000F, 0x000F, 0x01234567, 0x0008);
 	 rtu_write_hcam_entry ( 0x1 , 0x1  ,   0x0 , 0x0, 0xA0,0x2424,0x24242424, 0x00  ,  0x0  ,  0x0  , 0x0   ,   0x2  ,  0x1 ,   0x0  ,   0x0   , 0x2 ,   0x1  ,0x000F, 0x000F, 0x01234567, 0x0010);
	 //
	 rtu_write_hcam_entry ( 0x1 , 0x0  ,   0x0 , 0x0, 0xF9,0xFF00,0x0000FFFF, 0x00  ,  0x0  ,  0x0  , 0x0   ,   0x0  ,  0x0 ,   0x0  ,   0x0   , 0x0 ,   0x0  ,0x0000, 0x0000, 0x01234567, 0x0018);
	 rtu_write_hcam_entry ( 0x1 , 0x0  ,   0x0 , 0x0, 0xF9,0xF500,0x0000FFFF, 0x00  ,  0x0  ,  0x0  , 0x0   ,   0x0  ,  0x0 ,   0x0  ,   0x0   , 0x0 ,   0x0  ,0x0000, 0x0000, 0x01234567, 0x0020);
	 rtu_write_hcam_entry ( 0x1 , 0x0  ,   0x0 , 0x0, 0xF9,0xF500,0x0000FFFF, 0x00  ,  0x0  ,  0x0  , 0x0   ,   0x0  ,  0x0 ,   0x0  ,   0x0   , 0x0 ,   0x0  ,0x0000, 0x0000, 0x01234567, 0x0028);
	 rtu_write_hcam_entry ( 0x1 , 0x1  ,   0x0 , 0x0, 0xF9,0xF500,0x0000FFFF, 0x00  ,  0x0  ,  0x0  , 0x0   ,   0x0  ,  0x0 ,   0x0  ,   0x0   , 0x0 ,   0x0  ,0x0000, 0x0000, 0x01234567, 0x0030);
	 
	
	printf("\nsome hcam entries filled in with predefined data.... \n");
#endif
}
/*
fill in vlan tab
*/
int rtu_fill_in_vlan_tab()
{
	int      i;
        int      addr;
	uint32_t port_mask;
	uint8_t  fid;
	uint8_t  prio;
	int      has_prio;
	int      prio_override;
	int      drop;
	

#ifdef RANDOM_DATA	
	for(i=0;i<MAX_VLANS;i++)
	{
	  
	  if(i%100==0) fprintf(stderr, "."); 
	  port_mask = (uint16_t) (2048.0 * (rand() / (RAND_MAX + 1.0))); //2^11
	  fid       = (uint8_t) (256.0 * (rand() / (RAND_MAX + 1.0))); // fid_width = 8, 2^8 = 256
	  prio      = (int) (8.0 * (rand() / (RAND_MAX + 1.0))); 
	  
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) has_prio = 0x1; else has_prio = 0x0; 
	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) prio_override = 0x1; else prio_override = 0x0; 
  	  if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 9) drop = 0x1; else drop = 0x0; 
	
	
	  rtu_write_vlan_entry(i, port_mask,fid,has_prio,prio,prio_override,drop);
	  
	}
	printf("\nentire vlan_tab filled in with random data.... \n");
#endif
#ifdef PREDEFINED_DATA	
  	// fill in VLAN TABLE
	//                   ADDR | MASK  | FID  |has_prio| PRIO| override |  drop
	

#ifdef CAM_DROP_PROBLEM
/*

|===============  addr = 0x112   ==========================|
|port_mask = 0x 23e | fid           = 0x  e9 | prio = 0x0  |
|has_prio  = 0x1    | prio_override = 0x0   | drop = 0x0   |
|^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^|
*/
	//                   ADDR | MASK  | FID  |has_prio| PRIO| override |  drop
	rtu_write_vlan_entry(274, 0x23e , 0xe9 ,   0x1  , 0x0 ,    0x0   ,  0x0  ) ;
/*

|===============  addr = 0x1dc   ==========================|
|port_mask = 0x 7ed | fid           = 0x  a4 | prio = 0x2  |
|has_prio  = 0x1    | prio_override = 0x0   | drop = 0x0   |
|^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^|
*/
	//                   ADDR | MASK  | FID  |has_prio| PRIO| override |  drop
	rtu_write_vlan_entry(0x1dc, 0x7ed , 0xa4 ,   0x1  , 0x2 ,    0x0   ,  0x0  ) ;
/*

|===============  addr = 0x1de   ==========================|
|port_mask = 0x  92 | fid           = 0x   e | prio = 0x5  |
|has_prio  = 0x1    | prio_override = 0x0   | drop = 0x0   |
|^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^|
*/
	//                   ADDR | MASK  | FID  |has_prio| PRIO| override |  drop
	rtu_write_vlan_entry(0x1de, 0x092 , 0x0e ,   0x1  , 0x5 ,    0x0   ,  0x0  ) ;
/*

|===============  addr = 0x2a2   ==========================|
|port_mask = 0x 78d | fid           = 0x  f9 | prio = 0x4  |
|has_prio  = 0x0    | prio_override = 0x1   | drop = 0x0   |
|^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^|
*/
	//                   ADDR | MASK  | FID  |has_prio| PRIO| override |  drop
	rtu_write_vlan_entry(0x2a2, 0x78d , 0xf9 ,   0x0  , 0x4 ,    0x1   ,  0x0  ) ;
/*

|===============  addr = 0x907   ==========================|
|port_mask = 0x 198 | fid           = 0x  66 | prio = 0x6  |
|has_prio  = 0x1    | prio_override = 0x1   | drop = 0x1   |
|^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^|
*/
	//                   ADDR | MASK  | FID  |has_prio| PRIO| override |  drop
	rtu_write_vlan_entry(0x907, 0x198 , 0x66 ,   0x1  , 0x6 ,    0x1   ,  0x1  ) ;
/*

|===============  addr = 0x155   ==========================|
|port_mask = 0x 155 | fid           = 0x  72 | prio = 0x5  |
|has_prio  = 0x0    | prio_override = 0x0   | drop = 0x0   |
|^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^|
*/
	//                   ADDR | MASK  | FID  |has_prio| PRIO| override |  drop
	rtu_write_vlan_entry(341, 0x155 , 0x72 ,   0x0  , 0x5 ,    0x0   ,  0x0  ) ;


	
	return;
#endif

#ifdef AGING_PROBLEM	
	rtu_write_vlan_entry(20, 0x48E , 0xF0 ,   0x1  , 0x0 ,    0x1   ,  0x0  ) ;//0x792
	rtu_write_vlan_entry(99, 0x0e3 , 0xe1 ,   0x0  , 0x5 ,    0x1   ,  0x0  ) ;//0x42C
	rtu_write_vlan_entry(288, 0x43F , 0xF2 ,   0x0  , 0x6 ,    0x1   ,  0x0  ) ;//0x36A


#endif
#ifdef DST_HAS_PRIO_PROBLEM
	rtu_write_vlan_entry(95, 0x244 , 0x71 ,   0x1  , 0x0 ,    0x1   ,  0x0  ) ;//0x6BA
	printf("dupa\n");
	return;
#endif
	rtu_write_vlan_entry(  0  , 0x7FF , 0x00 ,   0x0  , 0x0 ,    0x0   ,  0x0  ) ;
	rtu_write_vlan_entry(  1  , 0x555 , 0x01 ,   0x0  , 0x0 ,    0x1   ,  0x0  ) ;
	rtu_write_vlan_entry(  2  , 0x7E0 , 0xF0 ,   0x1  , 0x7 ,    0x0   ,  0x0  ) ;
	rtu_write_vlan_entry(  3  , 0x666 , 0xF4 ,   0x0  , 0x0 ,    0x0   ,  0x1  ) ;
	rtu_write_vlan_entry(  4  , 0x71c , 0xF9 ,   0x1  , 0x2 ,    0x0   ,  0x0  ) ;
	rtu_write_vlan_entry(  5  , 0x078 , 0xA0 ,   0x1  , 0x1 ,    0x0   ,  0x0  ) ;
	rtu_write_vlan_entry(  6  , 0x61C , 0xA4 ,   0x0  , 0x3 ,    0x1   ,  0x0  ) ;
	rtu_write_vlan_entry(  7  , 0x020 , 0xBB ,   0x1  , 0x5 ,    0x0   ,  0x1  ) ;
	rtu_write_vlan_entry(  8  , 0x00F , 0xFF ,   0x1  , 0x5 ,    0x0   ,  0x0  ) ;
	

	printf("\nsome vlan_tab entries filled in with predefined data.... \n");
#endif	
}




int rtu_fill_in_req_table(rtu_request_t rq_tab[], int req_number)
{
	int start = 0;
	uint32_t hash_addr;
	int i,j;
	int      port   ;
	uint16_t smac_hi;
	uint32_t smac_lo;
	uint16_t dmac_hi;
	uint32_t dmac_lo;
	uint16_t vid    ;
	uint8_t  prio   ;
	uint8_t has_vid ;
	uint8_t has_prio;
	char    *comment;
	uint16_t cam_addr;
	mac_table_entry_t *ent_1,*ent_2, *ent_cam;
#ifdef PREDEFINED_DATA	

//					     rtu_request_t ,port ,smac_hi,  smac_lo ,dmac_hi,   dmac_lo , vid  , prio ,has_vid ,has_prio,comment )

  #ifdef CAM_DROP_PROBLEM
	  rtu_write_request_entry_to_req_table(&rq_tab[0]    , 0   ,0xFF00 ,0x0000FFFF,0x1234 , 0x56789000,0x00F , 0x3  ,   0x1  ,  0x1   ,"predefined request");
	  rtu_write_request_entry_to_req_table(&rq_tab[1]    , 0   ,0x2424 ,0x24242424,0x0101 , 0x01010101,0x00F , 0x7  ,   0x1  ,  0x1   ,"predefined request");
	  rtu_write_request_entry_to_req_table(&rq_tab[2]    , 0   ,0x8144 , 0x3db674ee,0xcaf2 , 0x2e2e52b8,0x155 , 0x2  ,   0x1  ,  0x0   ,"problem with cam drop");
	  rtu_write_request_entry_to_req_table(&rq_tab[3]    , 0   ,0xcaf2 , 0x2e2e52b8,0x8144 ,0x3db674ee,0x155 , 0x2  ,   0x1  ,  0x0   ,"problem with cam drop");
	  
	  return;
  #endif


  #ifdef DST_HAS_PRIO_PROBLEM
	  rtu_write_request_entry_to_req_table(&rq_tab[0]    , 1   ,0x1234 , 0x56789000,0x1e92 ,0xe3201888,0x05f , 0x2  ,   0x1  ,  0x0   ,"predefined request - 0x6ba - dst_has_prio");
	  return;
  #else
	  rtu_write_request_entry_to_req_table(&rq_tab[0]    , 0   ,0xFF00 ,0x0000FFFF,0x1234 , 0x56789000,0x00F , 0x3  ,   0x1  ,  0x1   ,"predefined request");
  #endif
	
	rtu_write_request_entry_to_req_table(&rq_tab[1]    , 0   ,0x2424 ,0x24242424,0x0101 , 0x01010101,0x00F , 0x7  ,   0x1  ,  0x1   ,"predefined request");

  #ifdef AGING_PROBLEM
	  // problem with aging mem 1:hash = 0x792
	  rtu_write_request_entry_to_req_table(&rq_tab[2]    , 0   ,0x492a ,0xd5a53aa2,0x1234 , 0x56789000,0x014 , 0x6  ,   0x1  ,  0x0   ," problem with aging mem 1:hash = 0x792");
	  // problem with aging mem 1:hash = 0x42c
	  rtu_write_request_entry_to_req_table(&rq_tab[3]    , 0   ,0xd842 ,0xa6b6b8b2,0x0101 , 0x01010101,0x063 , 0x7  ,   0x1  ,  0x0   ," problem with aging mem 1:hash = 0x42c ");
	  // problem with aging mem 1:hash = 0x36a
	  rtu_write_request_entry_to_req_table(&rq_tab[4]    , 1   ,0xf976 ,0x88619328,0x1234 , 0x56789000,0x120 , 0x1  ,   0x1  ,  0x0   ," problem with aging mem 1:hash = 0x36a");
	  
  #else
	  rtu_write_request_entry_to_req_table(&rq_tab[2]    , 0   ,0xFF00 ,0x0000FFFF,0x1234 , 0x56789000,0x008 , 0x7  ,   0x1  ,  0x1   ,"normal & drop_when_src, update");
	  rtu_write_request_entry_to_req_table(&rq_tab[3]    , 0   ,0x2424 ,0x24242424,0x0101 , 0x01010101,0x005 , 0x7  ,   0x1  ,  0x1   ,"finding entry in CAM, learning ");
	  rtu_write_request_entry_to_req_table(&rq_tab[4]    , 1   ,0xFF00 ,0x0000FFFF,0x1234 , 0x56789000,0x00F , 0x5  ,   0x1  ,  0x1   ,"predefined request");
  #endif
	rtu_write_request_entry_to_req_table(&rq_tab[5]    , 1   ,0x2424 ,0x24242424,0x0101 , 0x01010101,0x00F , 0x7  ,   0x1  ,  0x1   ,"predefined request");
	rtu_write_request_entry_to_req_table(&rq_tab[6]    , 1   ,0xFF00 ,0x0000FFFF,0x1234 , 0x56789000,0x006 , 0x7  ,   0x1  ,  0x1   ,"predefined request");
	rtu_write_request_entry_to_req_table(&rq_tab[7]    , 1   ,0x2424 ,0x24242424,0x0101 , 0x01010101,0x003 , 0x7  ,   0x1  ,  0x1   ,"predefined request");
	rtu_write_request_entry_to_req_table(&rq_tab[8]    , 1   ,0x2424 ,0x24242424,0x0101 , 0x01010101,0x005 , 0x3  ,   0x1  ,  0x0   ,"predefined request");
	rtu_write_request_entry_to_req_table(&rq_tab[9]    , 1   ,0x2424 ,0x24242424,0x0101 , 0x01010101,0x005 , 0x3  ,   0x0  ,  0x1   ,"predefined request");
  #ifdef AGING_PROBLEM
	  return ;
  #endif
	printf("\npredefined requests written.... \n");
	start = 10;
#endif	
#ifdef RANDOM_DATA
	for(i=start;i<req_number;i++)
	{
	    if(i%10==0) fprintf(stderr, "."); 
	    
// 	    ===========================================================
// 	    port     = (int) (2.0 * (rand() / (RAND_MAX + 1.0))); //2^11;
// 	    smac_hi  = (uint16_t) (65536.0 * (rand() / (RAND_MAX + 1.0))); // 2^16 = 65536;
// 	    smac_lo  = (uint32_t) (4294967296.0 * (rand() / (RAND_MAX + 1.0))); // 2^32 = 4294967296;
// 	    dmac_hi  = (uint16_t) (65536.0 * (rand() / (RAND_MAX + 1.0))); // 2^16 = 65536;
// 	    dmac_lo  = (uint32_t) (4294967296.0 * (rand() / (RAND_MAX + 1.0))); // 2^32 = 4294967296;
// 	    vid      = (uint16_t) (4096.0 * (rand() / (RAND_MAX + 1.0))); // 2^12 = 4096;;
// 	    prio     = (int) (8.0 * (rand() / (RAND_MAX + 1.0)));
// 	
// 	    if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) has_vid = 0x1; else has_vid = 0x0; 
// 	    if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) has_prio = 0x1; else has_prio = 0x0; 


//=================================================================
	    
	    //port     = (int) (2.0 * (rand() / (RAND_MAX + 1.0))); //2^11;
	    port     = (int) ((float)PORT_NUMBER * (rand() / (RAND_MAX + 1.0))); //2^11;
	    
	    
	    //choose source address
	    int random = (int) ((float)hash_address_writen_cnt * (rand() / (RAND_MAX + 1.0)));
	    hash_addr =  hash_addresses[random] >> 5;
	   
	    
	    #ifdef ML_DBG
	    printf("max: %d, random : %d, hash_addr: %d\n",hash_address_writen_cnt,random,hash_addr);
	    #endif
	    
	    random = (int) ((float)RTU_BUCKETS * (rand() / (RAND_MAX + 1.0)));
	    ent_1 = &rtu_tab[CFG.rtu_bank].buckets[hash_addr][random];
	    
	    //choose destination address
	    random = (int) ((float)hash_address_writen_cnt * (rand() / (RAND_MAX + 1.0)));
	    hash_addr =  hash_addresses[random]  >> 5;
	    
	    #ifdef ML_DBG
	    printf("max: %d, random : %d, hash_addr: %d\n",hash_address_writen_cnt,random,hash_addr);
	    #endif
	    
	    random = (int) ((float)RTU_BUCKETS * (rand() / (RAND_MAX + 1.0)));
	    ent_2 = &rtu_tab[CFG.rtu_bank].buckets[hash_addr][random];
	    
	    //take randam cam addr (from unactive bank)
	    cam_addr = (uint16_t) ((float)((int)(cam_address[CFG.cam_bank]/8) - 1) * (rand() / (RAND_MAX + 1.0)));
	    
	    //printf("hcam_addr = 0x%x \n",cam_addr);
	    
	    ent_cam = &rtu_cam[CFG.rtu_bank][cam_addr];
	    
	    smac_hi  = (uint16_t)((ent_1->mac[0] << 8)  |  ent_1->mac[1]);
	    smac_lo  = (uint32_t)((ent_1->mac[2] << 24) | (ent_1->mac[3] << 16) | (ent_1->mac[4] << 8) | ent_1->mac[5]);

	    
	    dmac_hi  = (uint16_t)((ent_2->mac[0] << 8)  |  ent_2->mac[1]);
	    dmac_lo  = (uint32_t)((ent_2->mac[2] << 24) | (ent_2->mac[3] << 16) | (ent_2->mac[4] << 8) | ent_2->mac[5]);

	    
	    
	    
	    //choose whether to choose VID/FID for source or destination
	    if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) 
	    {
#ifdef ENFORCE_DATA_ENTRIES	      
		//choose whether we want find the thing in cam or htab 20/80, if there is no 
		// cam entries, don't choose from CAM
		if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 4 || cam_address[CFG.cam_bank] == 0) 
#else
		// just to make the data repeatable - so there is the same 
		// number of rand() calles
		int dummy = 1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0)));
		if(1)
#endif		  
		{
		    //take vid of source MAC
		    for(j=0;j<MAX_VLANS;j++)
		    {
		      if(vlan_tab[j].fid == ent_1->fid)
		      {
			vid = (uint16_t)j;  
			break;
		      }
		    }
		}
		else
		{
		   
		    smac_hi  = (uint16_t)((ent_cam->mac[0] << 8)  |  ent_cam->mac[1]);
		    smac_lo  = (uint32_t)((ent_cam->mac[2] << 24) | (ent_cam->mac[3] << 16) | (ent_cam->mac[4] << 8) | ent_cam->mac[5]);
		    
		    //take vid of source MAC
		    for(j=0;j<MAX_VLANS;j++)
		    {
		      if(vlan_tab[j].fid == ent_cam->fid)
		      {
			vid = (uint16_t)j;  
			#ifdef ML_DBG
			printf("CAM request[addr=0x%x]: 0x%4x%8x , fid: 0x%x , vid=0x%x\n",cam_addr,smac_hi,smac_lo ,ent_cam->fid, vid);
			#endif
			break;
		      }
		    }
		
		}
	    
	    }
	    else 
	    {
		//choose whether we want find the thing in cam or htab 20/80, if there is no 
		// cam entries, don't choose from CAM
#ifdef ENFORCE_DATA_ENTRIES
		if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) < 7 || cam_address[CFG.cam_bank] == 0)
#else
		// just to make the data repeatable - so there is the same 
		// number of rand() calles
		int dummy = 1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0)));
		if(1)
#endif
		{
		  //take vid of destination MAC
		    for(j=0;j<MAX_VLANS;j++)
		    {
		      if(vlan_tab[j].fid == ent_2->fid)
		      {
			vid = (uint16_t)j;  
			break;
		      }
		    }
		}
		else
		{
		
		
		    dmac_hi  = (uint16_t)((ent_cam->mac[0] << 8)  |  ent_cam->mac[1]);
		    dmac_lo  = (uint32_t)((ent_cam->mac[2] << 24) | (ent_cam->mac[3] << 16) | (ent_cam->mac[4] << 8) | ent_cam->mac[5]);
		    
		    //take vid of source MAC
		    for(j=0;j<MAX_VLANS;j++)
		    {
		      if(vlan_tab[j].fid == ent_cam->fid)
		      {
			vid = (uint16_t)j;  
			#ifdef ML_DBG
			printf("CAM request[addr=0x%x]: 0x%4x%8x , fid: 0x%x , vid=0x%x\n",cam_addr,dmac_hi,dmac_lo ,ent_cam->fid, vid);
			#endif
			break;
		      }
		    }
		    
		}
	    
	    }
	    
	      
	    
	    
	    prio = (int) (8.0 * (rand() / (RAND_MAX + 1.0)));
	
	    if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 3) has_vid = 0x1; else has_vid = 0x0; 
	    if(1 + (int) (10.0 * (rand() / (RAND_MAX + 1.0))) > 5) has_prio = 0x1; else has_prio = 0x0;	    	    
	    
	    rtu_write_request_entry_to_req_table(&rq_tab[i],port,smac_hi, smac_lo, dmac_hi,dmac_lo,vid,prio ,has_vid, has_prio,"request generated with random data" );

	  
	}
  
	
	printf("\nrequests with random data written.... \n");
#endif
}