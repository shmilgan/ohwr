/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_rtud v1.1
 *
 * Authors:     Maciej Lipinski (maciej.lipinski@cern.ch)
 *
 * Description: RTU driver module in user space to control extended RTU features
 *
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

#include <fpga_io.h>
#include <regs/rtu-regs.h>

#include "rtu_ext_drv.h"
#include "wr_rtu.h"
#include "rtu_tatsu_drv.h"

int rtux_init(void)
{
//    uint8_t mac_single_A[]    = {0x11,0x50,0xca,0xfe,0xba,0xbe};
//    uint8_t mac_single_B[]    = {0x11,0x11,0x11,0x11,0x11,0x11};
   uint8_t mac_single_A[]    = {0x00,0x10,0x94,0x00,0x00,0x01}; // spirent MAC of port 1
   uint8_t mac_single_B[]    = {0x00,0x10,0x94,0x00,0x00,0x02}; // spirent MAC of port 2
   uint8_t mac_single_C[]    = {0xb8,0xac,0x6f,0x5a,0x1a,0xd2}; // my laptop (new)
   uint8_t mac_single_D[]    = {0x00,0x15,0xb7,0x2f,0x81,0x69}; // my laptop (old)   
   uint8_t mac_range_lower[] = {0x00,0x50,0xca,0xfe,0xba,0xbe};
   uint8_t mac_range_upper[] = {0x08,0x50,0xca,0xfe,0xba,0xbe};   
   
   uint8_t UNICAST_MAC_ETH_5[]      = {0x00,0x1b,0x21,0x8e,0xd7,0x44 };
   uint8_t UNICAST_MAC_ETH_4_RENAME[] = {0x00,0x1b,0x21,0x8e,0xd7,0x45 };   
   
   TRACE(TRACE_INFO,"RTU eXtension Initialization (19 Nov 2012, 10am) ");
   
   rtux_simple_test();
   
//    rtux_add_ff_mac_single(0/*ID*/, 1/*valid*/, mac_single_A/*MAC*/);
//    rtux_add_ff_mac_single(1/*ID*/, 1/*valid*/, mac_single_B/*MAC*/);
//    rtux_add_ff_mac_single(2/*ID*/, 1/*valid*/, mac_single_C/*MAC*/);
//    rtux_add_ff_mac_single(3/*ID*/, 1/*valid*/, mac_single_D/*MAC*/);
   
//    rtux_add_ff_mac_range (0/*ID*/, 1/*valid*/, mac_range_lower/*MAC_lower*/, 
//                                                mac_range_upper /*MAC_upper*/);  
//    rtux_set_port_mirror  (1<<1/*mirror src*/,1<<7/*mirror dst*/,1/*rx*/,1/*tx*/);
   rtux_set_hp_prio_mask (0x00/*hp prio mask*/); // no HP

   rtux_read_cpu_port    ();
   rtux_feature_ctrl     (0 /*mr*/, 
                          0 /*mac_ptp*/, 
                          0/*mac_ll*/, 
                          0/*mac_single*/, 
                          0/*mac_range*/, 
                          0/*mac_br*/,
                          0/*drop when full_match full*/);

   return 0;
}

#define rtu_rd(reg) \
	 _fpga_readl(FPGA_BASE_RTU + offsetof(struct RTU_WB, reg))

#define rtu_wr(reg, val) \
	 _fpga_writel(FPGA_BASE_RTU + offsetof(struct RTU_WB, reg), val)




// --------------------------------------------
// RTU eXtension
// --------------------------------------------

int rtux_simple_test() 
{
  uint32_t val;
  
  
  TRACE(TRACE_INFO,">>>>>>>>>>> RTU eXtension simple test (19 Nov 2012, 10am)<<<<<<<<<<< ");
  val = rtu_rd(GCR);
  TRACE(TRACE_INFO,"RTUeX: G/W version : %d ",RTU_GCR_RTU_VERSION_R(val));
//   if(RTU_GCR_RTU_VERSION_R(val) == 2)
//   {
//     TRACE(TRACE_INFO,"RTUeX: G/W version : %d (correct G/W !!!) ",RTU_GCR_RTU_VERSION_R(val));
//   }
//   else
//   {
//     TRACE(TRACE_INFO,"RTUeX: G/W version : %d (in correct G/W !!! ) ",RTU_GCR_RTU_VERSION_R(val));
//     TRACE(TRACE_INFO,"RTUeX: TEST FAILED ");
// //     return -1;
//   }
      
  val = 8;
  rtu_wr(RX_CTR, RTU_RX_CTR_PRIO_MASK_W(val));
  TRACE(TRACE_INFO,"writing to the RX_CTR register value = %d ",val);
  val = rtu_rd(RX_CTR);
  TRACE(TRACE_INFO,"reading from the RX_CTR register value = %d ",RTU_RX_CTR_PRIO_MASK_R(val));
  
  TRACE(TRACE_INFO,">>>>>>>>>>>                                             <<<<<<<<<<< ");
  
//   TRACE(TRACE_INFO,"reading from the RTU_RX_FF_MAC_R1 max number values: ");
//   val = rtu_rd(RX_FF_MAC_R1);
//   TRACE(TRACE_INFO,"\trange max number:%d (should be 1) ",RTU_RX_FF_MAC_R1_HI_ID_R(val));
//   TRACE(TRACE_INFO,"\trange max number:%d (should be 4) ",RTU_RX_FF_MAC_R1_ID_R(val));
/*  
  TRACE(TRACE_INFO,"mirror traffic from port 2 into port 3 ");
  val = 1 << 2;
  rtu_wr(RX_MP_R0, RTU_RX_MP_R0_SRC_MASK_W(val));  
  val = 1 << 3;
  rtu_wr(RX_MP_R1, RTU_RX_MP_R1_DST_MASK_W(val));  */

  return 0;
}

static uint32_t get_mac_lo(uint8_t mac[ETH_ALEN])
{
    return
        ((0xFF & mac[0])                        << 24)  |
        ((0xFF & mac[1])                        << 16)  |
        ((0xFF & mac[2])                        <<  8)  |
        ((0xFF & mac[3])                             )  ;

}

static uint32_t get_mac_hi(uint8_t mac[ETH_ALEN])
{
    return

        ((0xFF & mac[4])                        <<  8)  |
        ((0xFF & mac[5])                             )  ;
}


void rtux_add_ff_mac_single(int mac_id, int valid, uint8_t mac[ETH_ALEN])
{
   uint32_t mac_hi, mac_lo;

   mac_lo = RTU_RX_FF_MAC_R0_LO_W   (get_mac_lo(mac)) ;
   mac_hi = RTU_RX_FF_MAC_R1_ID_R   (get_mac_hi(mac)) |
            RTU_RX_FF_MAC_R1_HI_ID_W(mac_id)      |
            //RTU_RX_FF_MAC_R1_TYPE                     // type = 0
            RTU_RX_FF_MAC_R1_VALID;
                
   rtu_wr(RX_FF_MAC_R0, mac_lo);
   rtu_wr(RX_FF_MAC_R1, mac_hi);
 
   TRACE(TRACE_INFO,"RTU eXtension: set fast forward single mac (id=%d, valid=%d) of "
         "%x:%x:%x:%x:%x:%x", mac_id, valid,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}
/**
 * \brief Configures a range of MACs to be forwarded using fast-match (broadcast within VLAN)
 * @param mac_id    ID of the entry
 * @param valid     indicates whether it is valid (if not valid, the entry is removed
 * @param mac_lower MAC with address that starts the range
 * @param mac_upper MAC with address that finishes the range
 */
void rtux_add_ff_mac_range(int mac_id, int valid, uint8_t mac_lower[ETH_ALEN],
                                                  uint8_t mac_upper[ETH_ALEN])
{
   uint32_t mac_hi=0, mac_lo=0;
   uint32_t m_mac_id=0; // modified mac id
   // writting lower boundary of the mac range
   m_mac_id = (~(1 << 7) ) & mac_id; // lower range (highest bit is low)

   mac_lo = RTU_RX_FF_MAC_R0_LO_W   (get_mac_lo(mac_lower)) ;
   mac_hi = RTU_RX_FF_MAC_R1_ID_R   (get_mac_hi(mac_lower)) |
            RTU_RX_FF_MAC_R1_HI_ID_W(mac_id)                |
            RTU_RX_FF_MAC_R1_TYPE                           | // type = 1
            RTU_RX_FF_MAC_R1_VALID;
                
   rtu_wr(RX_FF_MAC_R0, mac_lo);
   rtu_wr(RX_FF_MAC_R1, mac_hi);
 
   // writting upper boundary of the mac range
   m_mac_id = (1 << 7) | mac_id; // upper range high (highest bit is low)

   mac_lo = RTU_RX_FF_MAC_R0_LO_W   (get_mac_lo(mac_upper)) ;
   mac_hi = RTU_RX_FF_MAC_R1_ID_R   (get_mac_hi(mac_upper)) |
            RTU_RX_FF_MAC_R1_HI_ID_W(mac_id)                |
            RTU_RX_FF_MAC_R1_TYPE                           | // type = 1
            RTU_RX_FF_MAC_R1_VALID;
                
   rtu_wr(RX_FF_MAC_R0, mac_lo);
   rtu_wr(RX_FF_MAC_R1, mac_hi);
                
   
   TRACE(TRACE_INFO,"RTU eXtension: set fast forward mac range: (id=%d, valid=%d):", mac_id, 
                                                                                     valid);
   TRACE(TRACE_INFO,"\t lower_mac = %x:%x:%x:%x:%x:%x",mac_lower[0],mac_lower[1],mac_lower[2],
                                                       mac_lower[3],mac_lower[4],mac_lower[5]);
   TRACE(TRACE_INFO,"\t upper_mac = %x:%x:%x:%x:%x:%x",mac_upper[0],mac_upper[1],mac_upper[2],
                                                       mac_upper[3],mac_upper[4],mac_upper[5]);
}

/**
 * \brief Configure mirroring of ports (currently bugy in GW- don't use)
 * @param mirror_src_mask indicates from which ports traffic should be mirrored
 * @param mirror_dst_mask indicates to which ports mirrored traffic should be sent
 * @param rx              indicates that traffic received to src port should be mirrored
 * @param tx              indicates that traffic transmitted to src port should be mirrored
 */
void rtux_set_port_mirror(uint32_t mirror_src_mask, uint32_t mirror_dst_mask, int rx, int tx)
{
   uint32_t mp_src_rx=0, mp_src_tx=0, mp_dst=0, mp_sel=0;
   mp_dst    = RTU_RX_MP_R1_MASK_W(mirror_dst_mask);
   mp_src_tx = 0;
   mp_src_rx = 0;

   mp_sel    =  0 ; // destinatioon (dst_src=0)
   rtu_wr(RX_MP_R0,mp_sel);
   rtu_wr(RX_MP_R1,mp_dst);
   if(rx)
     mp_src_rx = RTU_RX_MP_R1_MASK_W(mirror_src_mask);
   else
     mp_src_rx = 0;
   mp_sel    = 0;
   mp_sel    = RTU_RX_MP_R0_DST_SRC; // source (dst_src=1), reception traffic (rx_tx=0)
   rtu_wr(RX_MP_R0,mp_sel);
   rtu_wr(RX_MP_R1,mp_src_rx);
    
   if(tx)
     mp_src_tx = RTU_RX_MP_R1_MASK_W(mirror_src_mask);
   else
     mp_src_tx = 0;
   mp_sel    = 0;
   mp_sel    = RTU_RX_MP_R0_DST_SRC | RTU_RX_MP_R0_RX_TX;
                           //  source (dst_src=1), transmission traffic (rx_tx=1)
   rtu_wr(RX_MP_R0,mp_sel);
   rtu_wr(RX_MP_R1,mp_src_tx);

   TRACE(TRACE_INFO,"\t mirror output port(s) mask                 (dst)    = 0x%x",mp_dst);
   TRACE(TRACE_INFO,"\t ingress traffic mirror source port(s) mask (src_rx) = 0x%x",mp_src_rx);
   TRACE(TRACE_INFO,"\t egress  traffic mirror source port(s) mask (src_tx) = 0x%x",mp_src_tx);
}
/**
 * \brief Read the mask which which priorities are considered High Priority (this only
 *        concerns the traffic which is fast-forwarded)
 * @return mask with priorities (eg. 0x9 => priority 7 and 0 are considered HP)

 */
uint8_t rtux_get_hp_prio_mask()
{
   uint32_t val=0;

   val  = rtu_rd(RX_CTR);
   TRACE(TRACE_INFO,"RTU eXtension: read hp priorities (for which priorities traffic is "
   "considered HP),  mask[rd]=0x%x",RTU_RX_CTR_PRIO_MASK_R(val) );
   return (uint8_t)RTU_RX_CTR_PRIO_MASK_R(val);
}
/**
 * \brief Set the mask which which priorities are considered High Priority (this only
 *        concerns the traffic which is fast-forwarded)
 * @param hp_prio_mask mask with priorities (eg. 0x9 => priority 7 and 0 are considered HP)
 */
void rtux_set_hp_prio_mask(uint8_t hp_prio_mask)
{
   uint32_t mask, val=0;

   mask = rtu_rd(RX_CTR);
   mask = RTU_RX_CTR_PRIO_MASK_W(hp_prio_mask) | (mask & ~RTU_RX_CTR_PRIO_MASK_MASK);
   rtu_wr(RX_CTR,mask);
   val  = rtu_rd(RX_CTR);
   TRACE(TRACE_INFO,"RTU eXtension: set hp priorities (for which priorities traffic is "
   "considered HP), mask[wr]=0x%x => mask[rd]=0x%x",mask, RTU_RX_CTR_PRIO_MASK_R(val));
}
/**
 * \brief Read number of virtual port on which CPU is connected to SWcore
 * @return port number
 */
int rtux_get_cpu_port()
{
   uint32_t mask;
   int i =0;

   mask = rtu_rd(CPU_PORT);
   
   TRACE(TRACE_INFO,"RTU eXtension: reading mask indicating which (virtual) port is connected"
                    "to CPU mask=0x%x", RTU_CPU_PORT_MASK_R(mask));
   for(i=0;i<= MAX_PORT+1;i++)
   {
     if(mask & 0x1) return i;
     else           mask = mask >> 1;
   }
   return -1;
}
/**
 * \brief Setting (enabling/disabling) fast forwarding (few cycles, using special
 *        fast-match engine) and features
 * @param mr         enable mirroring of ports (needs to be configured properly)
 * @param mac_ptp    enable fast forwarding of PTP (Ethernet mapping, MAC=01:1b:19:00:00:00, wild card)
 * @param mac_ll     enable fast forwarding of Reserved MACs (wild card)
 * @param mac_single enable fast forwarding of MACs configured previously (boardcast within VLAN)
 * @param mac_range  enable fast forwarding of range of MACs configured previously (boardcast within VLAN)
 * @param mac_br     enable fast forwarding of range Broadcast traffic
 * @param at_fm      configure behavior of when the full match is to slow (0=drop, 1=broadcast within VLAN)
 */
void rtux_set_feature_ctrl(int mr, int mac_ptp, int mac_ll, int mac_single, int mac_range,
                       int mac_br, int at_fm)
{
   uint32_t mask;
   mask = rtu_rd(RX_CTR);
//    $display("RTU eXtension features debugging: 1: read mask: 0x%x",mask);
//    mask = !(`RTU_RX_CTR_MR_ENA        |
//             `RTU_RX_CTR_FF_MAC_PTP    |
//             `RTU_RX_CTR_FF_MAC_LL     |
//             `RTU_RX_CTR_FF_MAC_SINGLE |
//             `RTU_RX_CTR_FF_MAC_RANGE  |
//             `RTU_RX_CTR_FF_MAC_BR     |
//             32'h00000000) &
//              mask; 
   mask = 0xFFFFFF80 & mask; 
   /*$display("RTU eXtension features debugging: 2: cleared mask: 0x%x",mask);*/         
   
   if(mr)         mask = RTU_RX_CTR_MR_ENA              | mask;
   if(mac_ptp)    mask = RTU_RX_CTR_FF_MAC_PTP          | mask;
   if(mac_ll)     mask = RTU_RX_CTR_FF_MAC_LL           | mask;
   if(mac_single) mask = RTU_RX_CTR_FF_MAC_SINGLE       | mask;
   if(mac_range)  mask = RTU_RX_CTR_FF_MAC_RANGE        | mask;
   if(mac_br)     mask = RTU_RX_CTR_FF_MAC_BR           | mask;
   if(at_fm)      mask = RTU_RX_CTR_AT_FMATCH_TOO_SLOW  | mask;
   
   rtu_wr(RX_CTR, mask);
   rtux_disp_ctrl();
}
/**
 * \brief Sets forwarding of High Priority and/or unrecognized traffic to CPU (by default
 * 	  such traffic is not forwarded)
 * @param hp    flag indicateing High Prio traffic
 * @param unrec flag indicating unrecognized trafic)
 */
void rtux_set_fw_to_CPU(int hp, int unrec)
{
   uint32_t mask;
   mask = rtu_rd(RX_CTR);
   mask = 0xFFF0FFFF & mask;
   
   if(hp)    mask = RTU_RX_CTR_HP_FW_CPU_ENA   | mask;
   if(unrec) mask = RTU_RX_CTR_UREC_FW_CPU_ENA | mask;
   
   rtu_wr(RX_CTR, mask);
}

void rtux_fw_to_CPU(int arg)
{
   uint32_t mask;
   int hp    = 0x1 & (arg >> 0);
   int unrec = 0x1 & (arg >> 1);
   mask = rtu_rd(RX_CTR);
   mask = 0xFFF0FFFF & mask; 
   
   /*$display("RTU eXtension features debugging: 2: cleared mask: 0x%x",mask);*/         
   
   if(hp)    mask = RTU_RX_CTR_HP_FW_CPU_ENA   | mask;
   if(unrec) mask = RTU_RX_CTR_UREC_FW_CPU_ENA | mask;
   
   rtu_wr(RX_CTR, mask);
   rtux_disp_fw_to_CPU();

}

void rtux_disp_fw_to_CPU()
{
   uint32_t mask;
   mask = rtu_rd(RX_CTR);
   TRACE(TRACE_INFO,"RTU eXtension features (read):");
   if(RTU_RX_CTR_HP_FW_CPU_ENA & mask)
     {TRACE(TRACE_INFO,"\t (1 ) HP forwarding to CPU                    - enabled"); }
   else          
     {TRACE(TRACE_INFO,"\t (1 ) HP forwarding to CPU                    - disabled"); } 

   if(RTU_RX_CTR_UREC_FW_CPU_ENA & mask)
     {TRACE(TRACE_INFO,"\t (2 ) Unrec broadcast forwarding to CPU       - enabled"); }
   else          
     {TRACE(TRACE_INFO,"\t (2 ) Unrec broadcast forwarding to CPU       - disabled"); } 

}

int rtux_dbg_force_match(int arg)
{
   uint32_t mask;
   mask = rtu_rd(RX_CTR);
   mask = 0xF3FFFFFF & mask; 
   
   if(arg == 0) 
   {
     mask;
     TRACE(TRACE_INFO,"RTU eXtension DBG features: disable forcing Match Mechanism [mask=0x%x]", mask);
   }
   else if(arg == 1)
   {
     mask = RTU_RX_CTR_FORCE_FAST_MATCH_ENA | mask;
     TRACE(TRACE_INFO,"RTU eXtension DBG features: force Fast Match Mechanism [mask=0x%x]", mask);
   }
   else if(arg == 2)
   {
     mask = RTU_RX_CTR_FORCE_FULL_MATCH_ENA | mask;
     TRACE(TRACE_INFO,"RTU eXtension DBG features: force Full Match Mechanism [mask=0x%x]", mask);
   }
   else
   {
     TRACE(TRACE_INFO,"RTU eXtension DBG features: FAILED: wrong input value [value=%d, allowed: 0,1,2]", arg);
     return 0;
   }
   rtu_wr(RX_CTR, mask);
   
}

void rtux_disp_ctrl(void)
{
   uint32_t mask;
   mask = rtu_rd(RX_CTR);
   TRACE(TRACE_INFO,"RTU eXtension features (read):");
   if(RTU_RX_CTR_MR_ENA             & mask)
     {TRACE(TRACE_INFO,"\t (1 ) Port Mirroring                           - enabled"); }
   else
     {TRACE(TRACE_INFO,"\t (1 ) Port Mirroring                           - disabled");}
   if(RTU_RX_CTR_FF_MAC_PTP         & mask)
     {TRACE(TRACE_INFO,"\t (2 ) PTP fast forward                         - enabled"); }
   else
     {TRACE(TRACE_INFO,"\t (2 ) PTP fast forward                         - disabled");}
   if(RTU_RX_CTR_FF_MAC_LL          & mask)
     {TRACE(TRACE_INFO,"\t (4 ) Link-limited traffic (BPDU) fast forward - enabled"); }
   else
     {TRACE(TRACE_INFO,"\t (4 ) Link-limited traffic (BPDU) fast forward - disabled");}
   if(RTU_RX_CTR_FF_MAC_SINGLE       & mask)
     {TRACE(TRACE_INFO,"\t (8 ) Single configured MACs fast forward      - enabled"); }
   else
     {TRACE(TRACE_INFO,"\t (8 ) Single configured MACs fast forward      - disabled");}
   if(RTU_RX_CTR_FF_MAC_RANGE       & mask)
     {TRACE(TRACE_INFO,"\t (16) Range of configured MACs fast forward    - enabled"); }
   else
     {TRACE(TRACE_INFO,"\t (16) Range of configured MACs fast forward    - disabled");}
   if(RTU_RX_CTR_FF_MAC_BR          & mask)
     {TRACE(TRACE_INFO,"\t (32) Broadcast fast forward                   - enabled"); }
   else
     {TRACE(TRACE_INFO,"\t (32) Broadcast fast forward                   - disabled");}
   if(RTU_RX_CTR_AT_FMATCH_TOO_SLOW & mask)
     {TRACE(TRACE_INFO,"\t (64) When fast match engine too slow          - braodcast processed frame");}
   else
     {TRACE(TRACE_INFO,"\t (64) When fast match engine too slow          - drop processed frame");}
   if(RTU_RX_CTR_FORCE_FAST_MATCH_ENA & mask)
     {TRACE(TRACE_INFO,"\t DBG  Force Fast Mach Mechanism                - enabled");}
   else
     {TRACE(TRACE_INFO,"\t DBG  Force Fast Mach Mechanism                - disabled");}
   if(RTU_RX_CTR_FORCE_FULL_MATCH_ENA & mask)
     {TRACE(TRACE_INFO,"\t DBG  Force Full Mach Mechanism                - enabled");}
   else
     {TRACE(TRACE_INFO,"\t DBG  Force Full Mach Mechanism                - disabled");}
}

void rtux_set_life(char *optarg)
{
  
  int i;
  uint32_t n1,n2, n3,n4;
  int opt     = strtol(optarg, &optarg, 0);
  int sub_opt = strtol(optarg+1, &optarg, 0);
  
  int err = shw_fpga_mmap_init();
  if(err)
  {
     TRACE(TRACE_INFO, "Problem with fpga mapping");
     exit(1);
  }
  
  TRACE(TRACE_INFO, "opt: %ld, sub_opt  %ld",opt, sub_opt);
  
  switch(opt){
    case 1:
       rtux_feature_ctrl((sub_opt>>0)&0x1, //mr
                         (sub_opt>>1)&0x1, //mac_pto
                         (sub_opt>>2)&0x1, //mac_ll
                         (sub_opt>>3)&0x1, //mac_single
                         (sub_opt>>4)&0x1, //mac_range
                         (sub_opt>>5)&0x1, //mac_br
                         (sub_opt>>6)&0x1  //at_fm
                         );
    break;
    case  2:
//       rtux_set_cpu_port(1<<sub_opt);
      rtux_read_cpu_port();
    break;
    case  3:
       rtux_set_hp_prio_mask((0xFF & sub_opt));
    break;
    case  4:
       if(sub_opt == 1)
       {
          rtux_set_port_mirror(1<<0 /*mirror_src_mask*/, 1<<7 /*mirror_dst_mask*/, 1/*rx*/, 1/*tx*/);
       }
       else if(sub_opt == 2)
       {
          rtux_set_port_mirror(1<<7/*mirror_src_mask*/, 1<<1/*mirror_dst_mask*/, 1/*rx*/, 0/*tx*/);
       }
       else if(sub_opt == 3)
       {
          rtux_set_port_mirror(1<<7/*mirror_src_mask*/, 1<<5/*mirror_dst_mask*/, 1/*rx*/, 1/*tx*/);
       }
       else
       {
	  TRACE(TRACE_INFO, "MIRRORING: unrecognzied option");
       }
       break;
    case  5:
      rtux_fw_to_CPU(sub_opt);
    break;       
    case 6:
       if(sub_opt == 1 ) tatsu_drop_nonHP_enable();
       else if(sub_opt == 0 ) tatsu_drop_nonHP_disable();
      break;
    case 7:
      rtux_dbg_force_match(sub_opt);
      break;      
    case  10:

      rtux_disp_ctrl() ;
      rtux_disp_fw_to_CPU(); 
      rtux_read_cpu_port();
      rtux_read_hp_prio_mask();
      tatsu_read_status();
      
    break;
    default:
       TRACE(TRACE_INFO, "RTU extension control options");
       TRACE(TRACE_INFO, "Usage: wrsw_rtu -x [<option> <value>]");
       TRACE(TRACE_INFO, "-x 0         show this info");
       TRACE(TRACE_INFO, "-x 1  mask   set ctrl options, mask in decimal format:");
       TRACE(TRACE_INFO, "             mask = d'{at_fm,mac_br,mac_range,mac_single, mac_ll, mac_ptp, mr}");
       TRACE(TRACE_INFO, "-x 2         read port mask cpu virual port recognized in fast match");
       TRACE(TRACE_INFO, "-x 3  prio   sets prio to be recognized as HP (provide mask)");
       TRACE(TRACE_INFO, "-x 4  opt    MIRRORING configuration (the mirroring needs to be enabled with set ctrl)");
       TRACE(TRACE_INFO, "-x 4  1      sets traffic tx-ed/rx-ed on port 1 to be mirrored on port 7");
       TRACE(TRACE_INFO, "-x 4  2      sets traffic rx-ed       on port 1 to be mirrored on port 7");
       TRACE(TRACE_INFO, "-x 4  3      sets traffic tx-ed       on port 1 to be mirrored on port 7");
       TRACE(TRACE_INFO, "-x 5  n      set forwarding to CPU config: mask = {unrec_fw_to_CPU, hp_fw_to_CPU}");
       TRACE(TRACE_INFO, "-x 6  0/1    TATSU: drop_nonHP frames disable/enable");
       TRACE(TRACE_INFO, "-x 7  opt    DBG: enable/disable forcing particular MATCH mechanism in RTU:");
       TRACE(TRACE_INFO, "-x 7  0           disable this stuff");
       TRACE(TRACE_INFO, "-x 7  1           force Fast Match only ");
       TRACE(TRACE_INFO, "-x 7  2           force Full Match only ");
       TRACE(TRACE_INFO, "-x 10        show status");
  };
  exit(1);
  
}

