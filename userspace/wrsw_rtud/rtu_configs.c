/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2013, CERN.
 *
 * Version:     wrsw_rtud v2.0-dev
 *
 * Authors:     Maciej Lipinski (maciej.lipinski@cern.ch)
 *              
 *
 * Description: RTU initial configuration
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
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>


#include <switch_hw.h>
#include <hal_client.h>
#include "rtu_configs.h"
#include "rtu_tatsu_drv.h"
#include "rtu_ep_drv.h"
#include "rtu_ext_drv.h"
#include "rtu_fd.h"
#include <fpga_io.h>
#include <regs/hwdu-regs.h>
#include <regs/rtu-regs.h>

uint8_t mac_single_spirent_A[] = {0x00,0x10,0x94,0x00,0x00,0x01}; // spirent MAC of port 1
uint8_t mac_single_spirent_B[] = {0x00,0x10,0x94,0x00,0x00,0x02}; // spirent MAC of port 2
uint8_t mac_single_spirent_C[] = {0x00,0x10,0x94,0x00,0x00,0x03}; // spirent MAC of port 1
uint8_t mac_single_spirent_D[] = {0x00,0x10,0x94,0x00,0x00,0x04}; // spirent MAC of port 2  

uint8_t mac_single_PC_ETH6[]   = {0x90,0xe2,0xba,0x17,0xa7,0xAF}; // test PC
uint8_t mac_single_PC_ETH7[]   = {0x90,0xe2,0xba,0x17,0xa7,0xAC}; // test PC 90:e2:ba:17:a7:AC
uint8_t mac_single_PC_ETH8[]   = {0x90,0xe2,0xba,0x17,0xa7,0xAE}; // test PC
uint8_t mac_single_PC_ETH9[]   = {0x90,0xe2,0xba,0x17,0xa7,0xAD}; // test PC

uint8_t bcast_mac[]            = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
int     prio_map[]             = {0,1,2,3,4,5,6,7};
int     rev_prio_map[]         = {7,6,5,4,3,2,1,0};
/**
 * opt=0
 */
int config_info()
{
  
  TRACE(TRACE_INFO, "Choice of predefined startup configurations of our super features");
  TRACE(TRACE_INFO, "Usage: wrsw_rtu -s [<option> <value>]");
  TRACE(TRACE_INFO, "-s 0         show this info");
  TRACE(TRACE_INFO, "-s 1         Default: TRU transparent, no RTUex features");
  TRACE(TRACE_INFO, "-s 2         Config like for TRU switch-over demo (but no rtu_thread")
  TRACE(TRACE_INFO, "             running): port 1 active, port 2 backup for p1, ports 6 ");
  TRACE(TRACE_INFO, "             and 7 enabled");
  TRACE(TRACE_INFO, "-s 3         Snake test for >normal traffic< : separate VLAN for each ");
  TRACE(TRACE_INFO, "             pair of ports (starting with VID=1 @ port 0), tagging on ");
  TRACE(TRACE_INFO, "             ingress, untagging on egress");
  TRACE(TRACE_INFO, "-s 4  n      Snake test for >fast forward traffic< : separate VLAN for ");
  TRACE(TRACE_INFO, "             each pair of ports (starting with VID=1 @ port 0), tagging ");
  TRACE(TRACE_INFO, "             on ingress, untagging on egress");
  TRACE(TRACE_INFO, "             0: single MAC entry for Spirent");
  TRACE(TRACE_INFO, "             1: single MAC entry for Test PC");
  TRACE(TRACE_INFO, "             2: range  MAC entry for Spirent");
  TRACE(TRACE_INFO, "             3: range  MAC entry for Test PC");
  TRACE(TRACE_INFO, "             4: Broadcast");
  TRACE(TRACE_INFO, "-s 5  n      Test VLANS:");
  TRACE(TRACE_INFO, "             0: VID/FID=1 with mask 0xF and static entry for broadcast to all ports");
  TRACE(TRACE_INFO, "             1: VIDs/FIDs={1-18} with mask 0x1<<i and broadcast for all ports");
  TRACE(TRACE_INFO, "             2: VID/FID=0 with mask 0xF and broadcast for all ports");
  TRACE(TRACE_INFO, "             3: VIDs/FIDs={1-18} with mask 0x3<<i and broadcast for all ports");
  TRACE(TRACE_INFO, "             4: VIDs={0-1}, FID=0 with mask 0xF<<i and broadcast for all ports");  
  TRACE(TRACE_INFO, "-s 6  n      Snake test for >fast forward traffic< (hacked): separate VLAN for ");
  TRACE(TRACE_INFO, "             each pair of ports (starting with VID=0 @ port 0), tagging ");
  TRACE(TRACE_INFO, "             on ingress, untagging on egress");
  TRACE(TRACE_INFO, "             0: single MAC entry for Spirent");
  TRACE(TRACE_INFO, "             1: single MAC entry for Test PC");
  TRACE(TRACE_INFO, "             2: range  MAC entry for Spirent");
  TRACE(TRACE_INFO, "             3: range  MAC entry for Test PC");
  TRACE(TRACE_INFO, "             4: Broadcast");
  TRACE(TRACE_INFO, "             5: nothing special");
  TRACE(TRACE_INFO, "-s 7  n      High Prio tunneling, kind of:  ");
  TRACE(TRACE_INFO, "      0      nonHP frame dropping disabled ");    
  TRACE(TRACE_INFO, "      1      nonHP frame dropping enabled ");  
  TRACE(TRACE_INFO, "-s 8         LACP test config  ");
  TRACE(TRACE_INFO, "-s 9         Configuration for debugging VLANS (VID=0->discard; VIDs>0, different masks, TRU enabled");
  TRACE(TRACE_INFO, "-s 10        Testing Fast Match (confiured but not enabled singleMAC, etc)");
  TRACE(TRACE_INFO, "-s 11 n      Testing tagging/untagging:");
  TRACE(TRACE_INFO, "      1      Only tagging   on ingress");
  TRACE(TRACE_INFO, "      2      Only untagging on egress");  
  TRACE(TRACE_INFO, "      other  Both tagging   on ingress and untagging on egress");  
  TRACE(TRACE_INFO, "-s 12 n      VLAN 1 for two ports, tagging/untagging");  
  TRACE(TRACE_INFO, "      1      VLAN 1 for ports 0 and 7, tagging/untagging");  
  TRACE(TRACE_INFO, "      2      VLAN 1 for ports 0 and 7, tagging/untagging"); 
  TRACE(TRACE_INFO, "-s 13 n      High Prio tunneling:p0 (HP) and p1 (nonHP) sending on p2:  ");
  TRACE(TRACE_INFO, "      0      nonHP frame dropping disabled ");    
  TRACE(TRACE_INFO, "      1      nonHP frame dropping enabled ");   
  TRACE(TRACE_INFO, "-s 14 n      High Prio tunneling:p0 (HP) sending on p2:  ");
  TRACE(TRACE_INFO, "      0      nonHP frame dropping disabled ");    
  TRACE(TRACE_INFO, "      1      nonHP frame dropping enabled ");   
  TRACE(TRACE_INFO, "-s 15 n      PTP and HP at ports p0 & p1:  ");
  TRACE(TRACE_INFO, "      0      nonHP frame dropping disabled ");    
  TRACE(TRACE_INFO, "      1      nonHP frame dropping enabled ");   
  TRACE(TRACE_INFO, "-s 16 n      two first ports not tagged/untagged, and no VLAN=1, other ports snake:  ");
  TRACE(TRACE_INFO, "-s 17        start switch as tester with default setings: frame size of 250 bytes and interframe gap of 100x16ns ");


  return 0;
}

/**
 * opt=1
 */
int config_default(int sub_opt, int port_num)
{
  int i;  
//   tru_enable(); // should be transparent -> bug ????????
   tru_disable();
   
   // just add to be able to use alter..
   rtux_add_ff_mac_single(0/*ID*/, 1/*valid*/, mac_single_spirent_A/*MAC*/);
   rtux_add_ff_mac_single(1/*ID*/, 1/*valid*/, mac_single_spirent_B/*MAC*/);
   rtux_add_ff_mac_single(2/*ID*/, 1/*valid*/, mac_single_spirent_C/*MAC*/);
   rtux_add_ff_mac_single(3/*ID*/, 1/*valid*/, mac_single_spirent_D/*MAC*/);   
   
   for(i=0;i<port_num;i++)
   {
     ep_set_vlan((uint32_t)i, 0x2/*qmode*/, 0 /*fix_prio*/, 0 /*prio_val*/, 0 /*pvid*/);
     ep_class_prio_map((uint32_t)i, prio_map);
   }
   rtux_set_feature_ctrl(0 /*mr*/, 
                     0 /*mac_ptp*/, 
                     0/*mac_ll*/, 
                     0/*mac_single*/, 
                     0/*mac_range*/, 
                     0/*mac_br*/,
                     1/*broadcast when full_match full ->> dropping does not work*/);   
  return 0;
}

/**
 * opt=2
 */
int config_tru_demo_test(int sub_opt, int port_num)
{
  int i;
  
  for(i=0;i<port_num;i++)
     ep_set_vlan((uint32_t)i, 0x2/*qmode*/, 0 /*fix_prio*/, 0 /*prio_val*/, 0 /*pvid*/);  
  tru_set_port_roles(1 /*active port*/,2/*backup port*/, 0 /*fid*/); //TODO: make it config
  tru_enable(); // should be transparent
  return 0;
}

/**
 * opt=3
 */
int config_snake_standard_traffic_test(int sub_opt, int port_num)
{
  int i;
  int pvid = 1;
  tru_disable();
  // for snake test
  for(i=0;i < 18;i++)
  {
    if(i%2==0 && i!=0) pvid++;
    rtu_fd_create_vlan_entry( pvid,          //vid, 
                  (0x3 << 2*(pvid-1)),  //port_mask, 
                  pvid,      //fid, 
                  0,            //prio,
                  0,            //has_prio,
                  0,            //prio_override, 
                  0             //drop
                  );     
          
   }    
   ep_snake_config(1 /*VLANS 0-17 port, access/untag*/);   
   return 0;  
}
/**
 * opt = 4
 * 
 * sub_opt:
 * 0: single mac entry test (Spirent)
 * 1: single mac entry test (PC)
 * 2: range mac entry test (Spirent)
 * 3: range mac entry test (PC)
 */
   
int config_snake_ff_traffic_test(int sub_opt, int port_num)
{
  int i;
  int pvid = 1;       
  tru_disable();
  
  //broadcast for VLAN = 0
  
  rtu_fd_create_entry(bcast_mac, 0, 0x3, STATIC, OVERRIDE_EXISTING); 
  // for snake test
  for(i=0;i < 18;i++)
  {
     if(i%2==0 && i!=0) pvid++;
     rtu_fd_create_vlan_entry( pvid,          //vid, 
                   (0x3 << 2*(pvid-1)),  //port_mask, 
                    pvid,      //fid, 
                    0,            //prio,
                    0,            //has_prio,
                    0,            //prio_override, 
                    0             //drop
                 );     
     rtu_fd_create_entry(bcast_mac, pvid, (0x3 << 2*(pvid-1)), STATIC, OVERRIDE_EXISTING);    
  }    
  switch(sub_opt)
  {
    case 0:
      rtux_add_ff_mac_single(0/*ID*/, 1/*valid*/, mac_single_spirent_A/*MAC*/);
      rtux_add_ff_mac_single(1/*ID*/, 1/*valid*/, mac_single_spirent_B/*MAC*/);
      rtux_add_ff_mac_single(2/*ID*/, 1/*valid*/, mac_single_spirent_C/*MAC*/);
      rtux_add_ff_mac_single(3/*ID*/, 1/*valid*/, mac_single_spirent_D/*MAC*/);
      break;
    case 1:
      rtux_add_ff_mac_single(0/*ID*/, 1/*valid*/, mac_single_PC_ETH6/*MAC*/);
      rtux_add_ff_mac_single(1/*ID*/, 1/*valid*/, mac_single_PC_ETH7/*MAC*/);
      rtux_add_ff_mac_single(2/*ID*/, 1/*valid*/, mac_single_PC_ETH8/*MAC*/);
      rtux_add_ff_mac_single(3/*ID*/, 1/*valid*/, mac_single_PC_ETH9/*MAC*/);     
      break;
    case 2:
      rtux_add_ff_mac_range (0/*ID*/, 1/*valid*/, mac_single_spirent_A/*MAC_lower*/, 
                                                  mac_single_spirent_D /*MAC_upper*/);        
    break;
    case 3:
      rtux_add_ff_mac_range (0/*ID*/, 1/*valid*/, mac_single_PC_ETH7/*MAC_lower*/, 
                                                  mac_single_PC_ETH6/*MAC_upper*/);             
      break;
    case 4:
      // nothing to do for broadcast
      break;
    default:
      TRACE(TRACE_INFO,"config_ff_in_vlan_test: wrong sub_opt  %d ",sub_opt);
      return -1;
    break;
  }
      
  if(sub_opt == 0 || sub_opt == 1)
    rtux_set_feature_ctrl(0 /*mr*/, 
                      0 /*mac_ptp*/, 
                      0/*mac_ll*/, 
                      1/*mac_single*/, 
                      0/*mac_range*/, 
                      0/*mac_br*/,
                      0/*drop when full_match full*/);
  else if(sub_opt == 2 || sub_opt == 3)
    rtux_set_feature_ctrl(0 /*mr*/, 
                      0 /*mac_ptp*/, 
                      0/*mac_ll*/, 
                      0/*mac_single*/, 
                      1/*mac_range*/, 
                      0/*mac_br*/,
                      0/*drop when full_match full*/);
  else if(sub_opt == 4)                     
    rtux_set_feature_ctrl(0 /*mr*/, 
                      0 /*mac_ptp*/, 
                      0/*mac_ll*/, 
                      0/*mac_single*/, 
                      0/*mac_range*/, 
                      1/*mac_br*/,
                      0/*drop when full_match full*/);
    
  ep_snake_config(1 /*VLANS 0-17 port, access/untag*/);   
  return 0;
}

/**
 * opt=5
 */
int config_VLAN_test(int sub_opt, int port_num)
{
  int i, pvid=0;

  tru_disable();
  if(sub_opt == 0)
  {
    rtu_fd_create_vlan_entry( 1,            //vid, 
                   0xF,          //port_mask, 
                   1,            //fid, 
                   0,            //prio,
                   0,            //has_prio,
                   0,            //prio_override, 
                   0             //drop
                 );  
    rtu_fd_create_entry(bcast_mac, 1, 0xFFFFFFFF , STATIC, OVERRIDE_EXISTING);
  }
  else if(sub_opt == 1)
  {
    for(i=0;i < 18;i++)
    {
      
      rtu_fd_create_vlan_entry(pvid,           //vid, 
                    (0x1 << pvid),  //port_mask, 
                    pvid,           //fid, 
                    0,              //prio,
                    0,              //has_prio,
                    0,              //prio_override, 
                    0               //drop
                    );     
      rtu_fd_create_entry(bcast_mac, pvid, 0xFFFFFFFF , STATIC, OVERRIDE_EXISTING);
      pvid++; 
    }        
  }
  else if(sub_opt == 2)
  {
    rtu_fd_create_vlan_entry( 0,            //vid, 
                   0xF,          //port_mask, 
                   0,            //fid, 
                   0,            //prio,
                   0,            //has_prio,
                   0,            //prio_override, 
                   0             //drop
                 );  
  }
  else if(sub_opt == 3)
  {
    pvid = 1;
    for(i=0;i < 18;i++)
    {
      rtu_fd_create_vlan_entry( pvid,          //vid, 
                    (0x3 << (pvid-1)),  //port_mask, 
                     pvid,      //fid, 
                     0,            //prio,
                     0,            //has_prio,
                     0,            //prio_override, 
                     0             //drop
                    );     
       rtu_fd_create_entry(bcast_mac, pvid, 0xFFFFFFFF , STATIC, OVERRIDE_EXISTING);
       pvid++; 
    } 
  }
  else if(sub_opt == 4)
  {
    /* this case does not work for both: Fast and Full match(es)
     * 1. for full match for VID=1 it drops
     *
     */
    rtu_fd_create_vlan_entry( 0,            //vid, 
                   0x7,          //port_mask, 
                   0,            //fid, 
                   0,            //prio,
                   0,            //has_prio,
                   0,            //prio_override, 
                   0             //drop
                 ); 
    rtu_fd_create_vlan_entry( 1,            //vid, 
                   0xF8,          //port_mask, 
                   0,            //fid, 
                   0,            //prio,
                   0,            //has_prio,
                   0,            //prio_override, 
                   0             //drop
                 ); 
    rtu_fd_create_entry(bcast_mac, 0, 0xFFFFFFFF , STATIC, OVERRIDE_EXISTING);    
  }  
  else
  { 
    TRACE(TRACE_INFO,"config_VLAN_test: wrong sub_opt  %d ",sub_opt);
    return -1;   
  }
   for(i=0;i<port_num;i++)
     ep_set_vlan((uint32_t)i, 0x2/*qmode*/, 0 /*fix_prio*/, 0 /*prio_val*/, 0 /*pvid*/);
  return 0;
}
/**
 * opt=6
 */
int config_snake_ff_hacked_traffic_test(int sub_opt, int port_num)
{
  int i;
  int pvid = 1;
  tru_disable();
  // for snake test
  for(i=0;i < 18;i++)
  {
    if(i%2==0 && i!=0) pvid++;
    rtu_fd_create_vlan_entry( pvid,          //vid, 
                  (0x3 << 2*(pvid-1)),  //port_mask, 
                  pvid,      //fid, 
                  0,            //prio,
                  0,            //has_prio,
                  0,            //prio_override, 
                  0             //drop
                  );     
    // static entries for snake forwarding
//     rtu_fd_create_entry(bcast_mac, pvid, (0x1 << 2*(pvid))  , mac_single_PC_ETH9, ADD_TO_EXISTING);        
//     rtu_fd_create_entry(bcast_mac, pvid, (0x1 << 2*(pvid-1)), mac_single_PC_ETH8, ADD_TO_EXISTING);
  }    
  ep_snake_config(1 /*VLANS 0-17 port, access/untag*/); 
  
  rtu_fd_create_vlan_entry( 0,          //vid, 
                 0x3,  //port_mask, 
                 0,      //fid, 
                  0,            //prio,
                  0,            //has_prio,
                  0,            //prio_override, 
                  0             //drop
                  );    
 
      
  switch(sub_opt)
  {
    case 0:
//       rtux_add_ff_mac_single(0/*ID*/, 1/*valid*/, mac_single_spirent_A/*MAC*/);
//       rtux_add_ff_mac_single(1/*ID*/, 1/*valid*/, mac_single_spirent_B/*MAC*/);
      rtux_add_ff_mac_single(0/*ID*/, 1/*valid*/, mac_single_spirent_C/*MAC*/);
      rtux_add_ff_mac_single(1/*ID*/, 1/*valid*/, mac_single_spirent_D/*MAC*/);
      break;
    case 1:
      rtux_add_ff_mac_single(0/*ID*/, 1/*valid*/, mac_single_PC_ETH6/*MAC*/);
      rtux_add_ff_mac_single(1/*ID*/, 1/*valid*/, mac_single_PC_ETH7/*MAC*/);
      rtux_add_ff_mac_single(2/*ID*/, 1/*valid*/, mac_single_PC_ETH8/*MAC*/);
      rtux_add_ff_mac_single(3/*ID*/, 1/*valid*/, mac_single_PC_ETH9/*MAC*/);     
      break;
    case 2:
      rtux_add_ff_mac_range (0/*ID*/, 1/*valid*/, mac_single_spirent_A/*MAC_lower*/, 
                                                  mac_single_spirent_D /*MAC_upper*/);        
    break;
    case 3:
      rtux_add_ff_mac_range (0/*ID*/, 1/*valid*/, mac_single_PC_ETH7/*MAC_lower*/, 
                                                  mac_single_PC_ETH6/*MAC_upper*/);             
      break;
    case 4:
      // nothing to do for broadcast
    case 5:
      // nothing to do for "normal" snake
      break;
    default:
      TRACE(TRACE_INFO,"config_ff_in_vlan_test: wrong sub_opt  %d ",sub_opt);
      return -1;
    break;
  }
      
  if(sub_opt == 0 || sub_opt == 1)
    rtux_set_feature_ctrl(0 /*mr*/, 
                      0 /*mac_ptp*/, 
                      0/*mac_ll*/, 
                      1/*mac_single*/, 
                      0/*mac_range*/, 
                      0/*mac_br*/,
                      0/*drop when full_match full*/);
  else if(sub_opt == 2 || sub_opt == 3)
    rtux_set_feature_ctrl(0 /*mr*/, 
                      0 /*mac_ptp*/, 
                      0/*mac_ll*/, 
                      0/*mac_single*/, 
                      1/*mac_range*/, 
                      0/*mac_br*/,
                      0/*drop when full_match full*/);
  else if(sub_opt == 4)                     
    rtux_set_feature_ctrl(0 /*mr*/, 
                      0 /*mac_ptp*/, 
                      0/*mac_ll*/, 
                      0/*mac_single*/, 
                      0/*mac_range*/, 
                      1/*mac_br*/,
                      0/*drop when full_match full*/);

  
  return 0;
}

/**
 * opt=7
 */
int config_hp_test(int sub_opt, int port_num)
{
  int i;
  int pvid = 1;
  tru_disable();
  // for snake test
  
  rtu_fd_create_vlan_entry( 0,          //vid, 
                 0x3,  //port_mask, 
                 0,      //fid, 
                 0,            //prio,
                 0,            //has_prio,
                 0,            //prio_override, 
                 0             //drop
                );    

  rtu_fd_create_vlan_entry( 1,          //vid, 
                 0xF,  //port_mask, 
                 1,      //fid, 
                 0,            //prio,
                 0,            //has_prio,
                 0,            //prio_override, 
                 0             //drop
                );      
  rtux_add_ff_mac_single(0/*ID*/, 1/*valid*/, mac_single_spirent_A/*MAC*/);
  rtux_add_ff_mac_single(1/*ID*/, 1/*valid*/, mac_single_spirent_B/*MAC*/);  

  ep_set_vlan(0 /*port*/, 0/*access port*/, 1 /*fix_prio*/, 7 /*prio_val*/, 1 /*pvid*/);
  ep_set_vlan(1 /*port*/, 0/*access port*/, 1 /*fix_prio*/, 7 /*prio_val*/, 1 /*pvid*/);  
//   ep_set_vlan(0 /*port*/, 0/*access port*/, 1 /*fix_prio*/, 0 /*prio_val*/, 1 /*pvid*/);
//   ep_set_vlan(1 /*port*/, 0/*access port*/, 1 /*fix_prio*/, 0 /*prio_val*/, 1 /*pvid*/);

  ep_set_vlan(2 /*port*/, 0/*access port*/, 0 /*fix_prio*/, 0 /*prio_val*/, 1 /*pvid*/);
  ep_set_vlan(3 /*port*/, 0/*access port*/, 0 /*fix_prio*/, 0 /*prio_val*/, 1 /*pvid*/);  
//   ep_set_vlan(2 /*port*/, 0/*access port*/, 1 /*fix_prio*/, 7 /*prio_val*/, 1 /*pvid*/);
//   ep_set_vlan(3 /*port*/, 0/*access port*/, 1 /*fix_prio*/, 7 /*prio_val*/, 1 /*pvid*/);

  ep_vcr1_wr( 0 /*port*/, 1/*is_vlan*/, 0 /*address*/, 0xFFFF /*data */ ); 
  ep_vcr1_wr( 1 /*port*/, 1/*is_vlan*/, 0 /*address*/, 0xFFFF /*data */ );      
  ep_vcr1_wr( 2 /*port*/, 1/*is_vlan*/, 0 /*address*/, 0xFFFF /*data */ ); 
  ep_vcr1_wr( 3 /*port*/, 1/*is_vlan*/, 0 /*address*/, 0xFFFF /*data */ ); 
  
  rtux_set_feature_ctrl(0 /*mr*/, 
                    0 /*mac_ptp*/, 
                    0/*mac_ll*/, 
                    1/*mac_single*/, 
                    0/*mac_range*/, 
                    0/*mac_br*/,
                    0/*drop when full_match full*/);  
  
   rtux_set_hp_prio_mask(1<<0); // 7 prio_val
//   rtux_set_hp_prio_mask(1<<7); // 0 prio_val
  
  if(sub_opt == 1)
    tatsu_drop_nonHP_enable();

  
  return 0;
}

/**
 * opt=8
 */
int config_lacp_test(int sub_opt, int port_num)
{
  int i;
  // set propper distribution functions for different types of traffic (hp, broadcast, unicast)
  tru_lacp_config(0 /* df_hp_id */,2 /* df_br_id */,1 /* df_un_id */);
    
  // set my PC's ports MACs as Fast-Forwrad
  rtux_add_ff_mac_single(0/*ID*/, 1/*valid*/, mac_single_PC_ETH6/*MAC*/);
  rtux_add_ff_mac_single(1/*ID*/, 1/*valid*/, mac_single_PC_ETH7/*MAC*/);
  rtux_add_ff_mac_single(2/*ID*/, 1/*valid*/, mac_single_PC_ETH8/*MAC*/);
  rtux_add_ff_mac_single(3/*ID*/, 1/*valid*/, mac_single_PC_ETH9/*MAC*/);     
      
  // enable Fast-Forward for single MACs (above)
  rtux_set_feature_ctrl(0 /*mr*/, 
                    0 /*mac_ptp*/, 
                    0/*mac_ll*/, 
                    1/*mac_single*/, 
                    0/*mac_range*/, 
                    0/*mac_br*/,
                    0/*drop when full_match full*/);  
  
  // set all fast forward traffic to be recognized as HP (bug, all or nothing)
  rtux_set_hp_prio_mask(0xFF);    
  
  //set true pattern sources
  //replacement = 4: packet filter
  //addition    = 5: received port mask
  tru_pattern_config(4/*replacement*/,5/*addition*/);
  
  // set VLAN: ports to take part in the game (as on simulation)
  rtu_fd_create_vlan_entry( 0,            //vid, 
                 0xF0F1,       //port_mask, 
                 0,            //fid, 
                 0,            //prio,
                 0,            //has_prio,
                 0,            //prio_override, 
                 0             //drop
                );   
  
  // ----------------------------------------------------------------------------------------
  // basic config:
  // excluding link aggregation, only the standard non-LACP ports
  tru_write_tab_entry(  1          /* valid         */,
                        0          /* entry_addr    */,    
                        0          /* subentry_addr */,
                        0x00000000 /* pattern_mask  */, 
                        0x00000000 /* pattern_match */,   
                        0x000      /* pattern_mode  */,
                        0x00000F0F /* ports_mask    */, 
                        0x00000F0F /* ports_egress  */,      
                        0x00000F0F /* ports_ingress */,
                        1          /* validate */,
                        1          /* print */ );                                    
  
   // ----------------------------------------------------------------------------------------
   // a bunch of link aggregation ports (ports 4 to 7 and 12&15):
   // 1) received FEC msg of class 0      
   tru_write_tab_entry(  1          /* valid         */,
                         0          /* entry_addr    */,    
                         1          /* subentry_addr */,
                         0x0000000F /* pattern_mask  */, 
                         0x00000001 /* pattern_match */,   
                         0x000      /* pattern_mode  */,
                         0x000090F0 /* ports_mask    */, 
                         0x00008010 /* ports_egress  */,      
                         0x00000000 /* ports_ingress */,
                         1          /* validate */,
                         1          /* print */ );                                    
   // 2) received FEC msg of class 1
   tru_write_tab_entry(  1          /* valid         */,
                         0          /* entry_addr    */,    
                         2          /* subentry_addr */,
                         0x0000000F /* pattern_mask  */, 
                         0x00000002 /* pattern_match */,   
                         0x000      /* pattern_mode  */,
                         0x000090F0 /* ports_mask    */, 
                         0x00008020 /* ports_egress  */,      
                         0x00000000 /* ports_ingress */,
                         1          /* validate */,
                         1          /* print */ );                                    
   // 3) received FEC msg of class 2
   tru_write_tab_entry(  1          /* valid         */,
                         0          /* entry_addr    */,    
                         3          /* subentry_addr */,
                         0x0000000F /* pattern_mask  */, 
                         0x00000004 /* pattern_match */,   
                         0x000      /* pattern_mode  */,
                         0x000090F0 /* ports_mask    */, 
                         0x00001040 /* ports_egress  */,      
                         0x00000000 /* ports_ingress */,
                         1          /* validate */,
                         1          /* print */ );                                    
   // 4) received FEC msg of class 3
   tru_write_tab_entry(  1          /* valid         */,
                         0          /* entry_addr    */,    
                         4          /* subentry_addr */,
                         0x0000000F /* pattern_mask  */, 
                         0x00000008 /* pattern_match */,   
                         0x000      /* pattern_mode  */,
                         0x000090F0 /* ports_mask    */, 
                         0x00001080 /* ports_egress  */,      
                         0x00000000 /* ports_ingress */,
                         1          /* validate */,
                         1          /* print */ );        
   
   // ----------------------------------------------------------------------------------------
   // collector: receiving frames on the aggregation ports:
   // forwarding to "normal" (others)
   tru_write_tab_entry(  1          /* valid         */,
                         0          /* entry_addr    */,    
                         5          /* subentry_addr */,
                         0x000090F0 /* pattern_mask  */, 
                         0x000090F0 /* pattern_match */,   
                         0x002      /* pattern_mode  */,
                         0x000090F0 /* ports_mask    */, 
                         0x00000000 /* ports_egress  */,      
                         0x000090F0 /* ports_ingress */,
                         1          /* validate */,
                         1          /* print */ );         
   // ----------------------------------------------------------------------------------------
   
   //program pfilter
   for(i=0;i< port_num;i++)
     ep_pfilter_lacp_test_code(i);
   
   //enable TRU (quite useful to do...)
   tru_enable(); 
   
   return 0;
}

/**
 * opt=9
 */
int config_VLAN_dbg(int sub_opt, int port_num)
{
  int i;  
  tru_disable();
  for(i=0;i<port_num;i++)
  {
    ep_set_vlan((uint32_t)i, 0x2/*qmode*/, 0 /*fix_prio*/, 0 /*prio_val*/, 0 /*pvid*/);
    ep_class_prio_map((uint32_t)i, prio_map);
  }
  
  rtu_fd_create_vlan_entry(0               /* vid           */, 
                0               /* port_mask     */, 
                0               /* fid           */, 
                0               /* prio          */,
                0               /* has_prio      */,
                0               /* prio_override */, 
                1                /* drop          */);  
  
  tru_write_tab_entry(1          /* valid         */,
                      0          /* entry_addr    */,    
                      0          /* subentry_addr */,
                      0x00000000 /* pattern_mask  */, 
                      0x00000000 /* pattern_match */,   
                      0x000      /* pattern_mode  */,
                      0x0000FFFF /* ports_mask    */, 
                      0x0000FFFF /* ports_egress  */,      
                      0x0000FFFF /* ports_ingress */,
                      1          /* validate      */,
                      1          /* print         */ );      
    
  for(i=1;i<10;i++)
  {
    rtu_fd_create_vlan_entry(i                /* vid           */, 
                  1<<i             /* port_mask     */, 
                  i                /* fid           */, 
                  0                /* prio          */,
                  0                /* has_prio      */,
                  0                /* prio_override */, 
                  0                /* drop          */);      
    
    tru_write_tab_entry(1          /* valid         */,
                        i          /* entry_addr    */,    
                        0          /* subentry_addr */,
                        0x00000000 /* pattern_mask  */, 
                        0x00000000 /* pattern_match */,   
                        0x000      /* pattern_mode  */,
                        0x0000FFFF /* ports_mask    */, 
                        0x0000FFFF /* ports_egress  */,      
                        0x0000FFFF /* ports_ingress */,
                        1          /* validate */,
                        1          /* print */ );     
  }
  tru_enable(); 
  
  return 0;
}

/**
 * opt=10
 */
int config_FF_test(int sub_opt, int port_num)
{
  int i;  
//   tru_enable(); // should be transparent -> bug ????????
   tru_disable();
   for(i=0;i<port_num;i++)
   {
     ep_set_vlan((uint32_t)i, 0x2/*qmode*/, 0 /*fix_prio*/, 0 /*prio_val*/, 0 /*pvid*/);
     ep_class_prio_map((uint32_t)i, prio_map);
   }
  rtux_add_ff_mac_single(0/*ID*/, 1/*valid*/, mac_single_PC_ETH6/*MAC*/);
  rtux_add_ff_mac_single(1/*ID*/, 1/*valid*/, mac_single_PC_ETH7/*MAC*/);

  rtux_add_ff_mac_single(2/*ID*/, 1/*valid*/, mac_single_spirent_A/*MAC*/);
  rtux_add_ff_mac_single(3/*ID*/, 1/*valid*/, mac_single_spirent_B/*MAC*/);
   
  rtux_add_ff_mac_range (0/*ID*/, 1/*valid*/, mac_single_PC_ETH9 /*MAC_lower*/, 
                                              mac_single_PC_ETH6 /*MAC_upper*/);   
  
  return 0;
}
/**
 * opt=11
 */
int config_tag_untag_test(int sub_opt, int port_num)
{
  int i;  
//   tru_enable(); // should be transparent -> bug ????????
   tru_disable();
   for(i=0;i<port_num;i++)
   {
     ep_class_prio_map((uint32_t)i, prio_map);
     if(sub_opt == 1) // only tagging on ingress
     {
       ep_set_vlan((uint32_t)i, 0x3/*qmode*/, 0 /*fix_prio*/, 0 /*prio_val*/, 0 /*pvid*/);
       ep_vcr1_wr((uint32_t)i /*port*/, 1/*is_vlan*/, 0 /*address*/, 0x0000 /*data */ );       
     }
     else if(sub_opt == 2) // only untagging on egress
     {
       ep_set_vlan((uint32_t)i, 0x2/*qmode*/, 0 /*fix_prio*/, 0 /*prio_val*/, 0 /*pvid*/);      
       ep_vcr1_wr((uint32_t)i /*port*/, 1/*is_vlan*/, 0 /*address*/, 0xFFFF /*data */ );
     }
     else
     {
       ep_set_vlan((uint32_t)i, 0x3/*qmode*/, 0 /*fix_prio*/, 0 /*prio_val*/, 0 /*pvid*/);
       ep_vcr1_wr((uint32_t)i /*port*/, 1/*is_vlan*/, 0 /*address*/, 0xFFFF /*data */ );       
     }
   }
  
  return 0;
}
/**
 * opt=12
 */
int config_two_ports_vlan(int sub_opt, int port_num)
{
  int i;
  int pvid = 1;
  tru_disable();
  
  switch(sub_opt)
  {
    case 1:  
      rtu_fd_create_vlan_entry(  1,          //vid, 
                   0x81,  //port_mask, 
                      1,      //fid, 
                      0,            //prio,
                      0,            //has_prio,
                      0,            //prio_override, 
                      0             //drop
                      );     
      break;
    case 2: 
      rtu_fd_create_vlan_entry(  1,          //vid, 
                   0x20001,  //port_mask, 
                      1,      //fid, 
                      0,            //prio,
                      0,            //has_prio,
                      0,            //prio_override, 
                      0             //drop
                      );     
      break;
    
    default:
    case 0: 
     TRACE(TRACE_INFO,"config_startup: opt = 12 sub_opt = %d NOT SUPPORTED", sub_opt);
     break;
  }   
   ep_strange_config(sub_opt/*option-not implemented*/);// config ports 0 and 7 in VLAN=1 and as access
      rtux_add_ff_mac_single(0/*ID*/, 1/*valid*/, mac_single_spirent_A/*MAC*/);
      rtux_add_ff_mac_single(1/*ID*/, 1/*valid*/, mac_single_spirent_B/*MAC*/);
      rtux_add_ff_mac_single(2/*ID*/, 1/*valid*/, mac_single_spirent_C/*MAC*/);
      rtux_add_ff_mac_single(3/*ID*/, 1/*valid*/, mac_single_spirent_D/*MAC*/);   
   return 0;  
}

/**
 * opt=13
 */
int config_hp_test_1(int sub_opt, int port_num)
{
  int i;
  int pvid = 1;
  tru_disable();
  // for snake test
  
  rtu_fd_create_vlan_entry( 0,          //vid, 
                 0xFF,  //port_mask, 
                 0,      //fid, 
                 0,            //prio,
                 0,            //has_prio,
                 0,            //prio_override, 
                 0             //drop
                );    

  rtu_fd_create_vlan_entry( 1,          //vid, 
                 0x5,  //port_mask, 
                 1,      //fid, 
                 0,            //prio,
                 0,            //has_prio,
                 0,            //prio_override, 
                 0             //drop
                );      
  rtu_fd_create_vlan_entry( 2,          //vid, 
                 0x6,  //port_mask, 
                 2,      //fid, 
                 0,            //prio,
                 0,            //has_prio,
                 0,            //prio_override, 
                 0             //drop
                );      

  
  
  rtux_add_ff_mac_single(0/*ID*/, 1/*valid*/, mac_single_spirent_A/*MAC*/);
  rtux_add_ff_mac_single(1/*ID*/, 1/*valid*/, mac_single_spirent_B/*MAC*/);  
  rtux_add_ff_mac_single(2/*ID*/, 1/*valid*/, mac_single_spirent_C/*MAC*/);
  rtux_add_ff_mac_single(3/*ID*/, 1/*valid*/, mac_single_spirent_D/*MAC*/);  

  
  ep_set_vlan(0 /*port*/, 0/*access port*/, 1 /*fix_prio*/, 7 /*prio_val*/, 1 /*pvid*/);
  ep_set_vlan(1 /*port*/, 0/*access port*/, 1 /*fix_prio*/, 0 /*prio_val*/, 2 /*pvid*/);  

  ep_vcr1_wr( 0 /*port*/, 1/*is_vlan*/, 0 /*address*/, 0xFFFF /*data */ ); 
  ep_vcr1_wr( 1 /*port*/, 1/*is_vlan*/, 0 /*address*/, 0xFFFF /*data */ );      
  ep_vcr1_wr( 2 /*port*/, 1/*is_vlan*/, 0 /*address*/, 0xFFFF /*data */ ); 
  ep_vcr1_wr( 3 /*port*/, 1/*is_vlan*/, 0 /*address*/, 0xFFFF /*data */ ); 
  
  rtux_set_feature_ctrl(0 /*mr*/, 
                    0 /*mac_ptp*/, 
                    0/*mac_ll*/, 
                    1/*mac_single*/, 
                    0/*mac_range*/, 
                    0/*mac_br*/,
                    0/*drop when full_match full*/);  
  
   //rtux_set_hp_prio_mask(1<<0); // 7 prio_val
   rtux_set_hp_prio_mask(1<<7); // 0 prio_val
  
  if(sub_opt == 1)
    tatsu_drop_nonHP_enable();
  else 
   tatsu_drop_nonHP_disable();

  
  return 0;
}

/**
 * opt=14
 */
int config_hp_test_2(int sub_opt, int port_num)
{
  int i;
  int pvid = 1;
  tru_disable();
  // for snake test
  
  rtu_fd_create_vlan_entry( 0,          //vid, 
                 0xFF,  //port_mask, 
                 0,      //fid, 
                 0,            //prio,
                 0,            //has_prio,
                 0,            //prio_override, 
                 0             //drop
                );    

  rtu_fd_create_vlan_entry( 1,          //vid, 
                 0x5,  //port_mask, 
                 1,      //fid, 
                 0,            //prio,
                 0,            //has_prio,
                 0,            //prio_override, 
                 0             //drop
                );      
  
  
  rtux_add_ff_mac_single(0/*ID*/, 1/*valid*/, mac_single_spirent_C/*MAC*/);
  ep_set_vlan(0 /*port*/, 0/*access port*/, 1 /*fix_prio*/, 7 /*prio_val*/, 1 /*pvid*/); 
  ep_vcr1_wr( 0 /*port*/, 1/*is_vlan*/, 0 /*address*/, 0xFFFF /*data */ ); 
  ep_vcr1_wr( 1 /*port*/, 1/*is_vlan*/, 0 /*address*/, 0xFFFF /*data */ );      
  ep_vcr1_wr( 2 /*port*/, 1/*is_vlan*/, 0 /*address*/, 0xFFFF /*data */ ); 
  ep_vcr1_wr( 3 /*port*/, 1/*is_vlan*/, 0 /*address*/, 0xFFFF /*data */ ); 
  
  
  rtux_set_feature_ctrl(0 /*mr*/, 
                    0 /*mac_ptp*/, 
                    0/*mac_ll*/, 
                    1/*mac_single*/, 
                    0/*mac_range*/, 
                    0/*mac_br*/,
                    0/*drop when full_match full*/);  
  
   //rtux_set_hp_prio_mask(1<<0); // 7 prio_val
   rtux_set_hp_prio_mask(1<<7); // 0 prio_val
  
  if(sub_opt == 1)
    tatsu_drop_nonHP_enable();
  else 
   tatsu_drop_nonHP_disable();

  
  return 0;
}


/**
 * opt=15
 */
int config_hp_and_ptp(int sub_opt, int port_num)
{
  int i;
  int pvid = 1;
  tru_disable();
  // for snake test
  
  rtu_fd_create_vlan_entry( 0,          //vid, 
                 0xFF,  //port_mask, 
                 0,      //fid, 
                 0,            //prio,
                 0,            //has_prio,
                 0,            //prio_override, 
                 0             //drop
                );    

  rtu_fd_create_vlan_entry( 1,          //vid, 
                 0x3,  //port_mask, 
                 1,      //fid, 
                 0,            //prio,
                 0,            //has_prio,
                 0,            //prio_override, 
                 0             //drop
                );      
  
  
  rtux_add_ff_mac_single(0/*ID*/, 1/*valid*/, mac_single_spirent_C/*MAC*/);
  ep_set_vlan(0 /*port*/, 0/*access port*/, 1 /*fix_prio*/, 7 /*prio_val*/, 1 /*pvid*/); 
  ep_set_vlan(1 /*port*/, 0/*access port*/, 1 /*fix_prio*/, 7 /*prio_val*/, 1 /*pvid*/); 
  ep_vcr1_wr( 0 /*port*/, 1/*is_vlan*/, 0 /*address*/, 0xFFFF /*data */ ); 
  ep_vcr1_wr( 1 /*port*/, 1/*is_vlan*/, 0 /*address*/, 0xFFFF /*data */ );      
  ep_vcr1_wr( 2 /*port*/, 1/*is_vlan*/, 0 /*address*/, 0xFFFF /*data */ ); 
  ep_vcr1_wr( 3 /*port*/, 1/*is_vlan*/, 0 /*address*/, 0xFFFF /*data */ ); 
  
  
  rtux_set_feature_ctrl(0 /*mr*/, 
                    1 /*mac_ptp*/, 
                    0/*mac_ll*/, 
                    1/*mac_single*/, 
                    0/*mac_range*/, 
                    0/*mac_br*/,
                    0/*drop when full_match full*/);  
  
   //rtux_set_hp_prio_mask(1<<0); // 7 prio_val
   rtux_set_hp_prio_mask(1<<7); // 0 prio_val
  
  if(sub_opt == 1)
    tatsu_drop_nonHP_enable();
  else 
   tatsu_drop_nonHP_disable();

  
  return 0;
}


/**
 * opt=16
 */
int config_snake_with_PTP_hacked_test(int sub_opt, int port_num)
{
  int i;
  int pvid = 0; 
  uint8_t ptp_mcast_mac[]     = {0x01, 0x1b, 0x19, 0x00, 0x00, 0x00};
  tru_disable();
  // for snake test

  
  for(i=0;i < 18;i++)
  {
    if(i%2==0 && i!=0) pvid++;
    rtu_fd_create_vlan_entry( pvid,          //vid, 
                  ((0x3 << 2*pvid) | (0x1 << 18)),  //port_mask, 
                  pvid,      //fid, 
                  0,            //prio,
                  0,            //has_prio,
                  0,            //prio_override, 
                  0             //drop
                  );     

   }
  //add PTP forwarding to CPU on first vlan
   rtu_fd_create_entry(ptp_mcast_mac, 1, (1 << port_num), STATIC, OVERRIDE_EXISTING);
   rtu_fd_create_entry(ptp_mcast_mac, 2, (1 << port_num), STATIC, OVERRIDE_EXISTING);
   rtu_fd_create_entry(ptp_mcast_mac, 3, (1 << port_num), STATIC, OVERRIDE_EXISTING);
   rtu_fd_create_entry(ptp_mcast_mac, 4, (1 << port_num), STATIC, OVERRIDE_EXISTING);
   rtu_fd_create_entry(ptp_mcast_mac, 5, (1 << port_num), STATIC, OVERRIDE_EXISTING);
   rtu_fd_create_entry(ptp_mcast_mac, 6, (1 << port_num), STATIC, OVERRIDE_EXISTING);
   rtu_fd_create_entry(ptp_mcast_mac, 7, (1 << port_num), STATIC, OVERRIDE_EXISTING);
   rtu_fd_create_entry(ptp_mcast_mac, 8, (1 << port_num), STATIC, OVERRIDE_EXISTING);
   
   rtux_add_ff_mac_single(0/*ID*/, 1/*valid*/, mac_single_spirent_A/*MAC*/);
   rtux_add_ff_mac_single(1/*ID*/, 1/*valid*/, mac_single_spirent_B/*MAC*/);
   rtux_add_ff_mac_single(2/*ID*/, 1/*valid*/, mac_single_spirent_C/*MAC*/);
   rtux_add_ff_mac_single(3/*ID*/, 1/*valid*/, mac_single_spirent_D/*MAC*/);     
   ep_snake_config(5 /* ports 2-17: VLANS + access/untag*/);   
   rtux_set_feature_ctrl(0 /*mr*/, 
                     0 /*mac_ptp*/, 
                     0/*mac_ll*/, 
                     1/*mac_single*/, 
                     0/*mac_range*/, 
                     0/*mac_br*/,
                     1/*broadcast full_match full*/);   
   return 0;  
}

#define rtu_rd(reg) \
	 _fpga_readl(FPGA_BASE_RTU + offsetof(struct RTU_WB, reg))

#define rtu_wr(reg, val) \
	 _fpga_writel(FPGA_BASE_RTU + offsetof(struct RTU_WB, reg), val)

int config_startup(int opt, int sub_opt, int port_num)
{
  
  TRACE(TRACE_INFO,"config_startup: opt = %ds sub_opt = %d ",opt, sub_opt);
  switch(opt)
  {
    case 1:
      config_default(sub_opt,port_num);
      break;
    case 2:
      config_tru_demo_test(sub_opt,port_num);
      break;
    case 3:
      config_snake_standard_traffic_test(sub_opt,port_num);
      break;
    case 4:
      config_snake_ff_traffic_test(sub_opt,port_num);
      break;
    case 5:
      config_VLAN_test(sub_opt, port_num);
      break;
    case 6:
      config_snake_ff_hacked_traffic_test(sub_opt,port_num);
      break;
    case 7:
      config_hp_test(sub_opt,port_num);
      break;
    case 8: 
      config_lacp_test(sub_opt,8);
      break;
    case 9:
      config_VLAN_dbg(sub_opt,8);
      break;
    case 10:
      config_FF_test(sub_opt,8);
      break;      
    case 11:
      config_tag_untag_test(sub_opt,8);
      break;      
    case 12:
      config_two_ports_vlan(sub_opt,0);
      break;  
    case 13:
      config_hp_test_1(sub_opt,port_num);
      break;  
    case 14:
      config_hp_test_2(sub_opt,port_num);
      break;       
    case 15:
      config_hp_and_ptp(sub_opt, port_num);
      break;    
    case 16:
      port_num= RTU_PSR_N_PORTS_R(rtu_rd(PSR));
      config_snake_with_PTP_hacked_test(sub_opt,port_num);
      break;
    case 17:
      config_default(sub_opt,port_num);
      ep_inj_gen_ctr_config_N_ports(port_num, 100 /*ifg*/, 250/*size*/, 0);
      break;
    //////////////////////////////////////////////////////////////////
    case 0:
    default:
      config_info();
      exit(1);
      break;    
  }

  
}
