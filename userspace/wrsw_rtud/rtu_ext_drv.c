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


int rtux_init(void)
{
   uint8_t mac_single_A[]    = {0x11,0x50,0xca,0xfe,0xba,0xbe};
   uint8_t mac_single_B[]    = {0x11,0x11,0x11,0x11,0x11,0x11};
   uint8_t mac_range_lower[] = {0x00,0x50,0xca,0xfe,0xba,0xbe};
   uint8_t mac_range_upper[] = {0x08,0x50,0xca,0xfe,0xba,0xbe};   
   
   uint8_t UNICAST_MAC_ETH_5[]      = {0x00,0x1b,0x21,0x8e,0xd7,0x44 };
   uint8_t UNICAST_MAC_ETH_4_RENAME[] = {0x00,0x1b,0x21,0x8e,0xd7,0x45 };   
   
   TRACE(TRACE_INFO,"RTU eXtension Initialization (19 Nov 2012, 10am) ");
   
   rtux_simple_test();
   
   rtux_add_ff_mac_single(0/*ID*/, 1/*valid*/, mac_single_A/*MAC*/);
//    rtux_add_ff_mac_single(1/*ID*/, 1/*valid*/, mac_single_B/*MAC*/);
   rtux_add_ff_mac_single(1/*ID*/, 1/*valid*/, UNICAST_MAC_ETH_5/*MAC*/);
   rtux_add_ff_mac_single(2/*ID*/, 1/*valid*/, UNICAST_MAC_ETH_4_RENAME/*MAC*/);
   rtux_add_ff_mac_range (0/*ID*/, 1/*valid*/, mac_range_lower/*MAC_lower*/, 
                                               mac_range_upper /*MAC_upper*/);  
   rtux_set_port_mirror  (1<<1/*mirror src*/,1<<7/*mirror dst*/,1/*rx*/,1/*tx*/);
   rtux_set_hp_prio_mask (0x81/*hp prio mask*/); //10000001
   rtux_set_cpu_port     (1<<8/*mask: virtual port of CPU*/);
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
  
  if(RTU_GCR_RTU_VERSION_R(val) == 2)
  {
    TRACE(TRACE_INFO,"RTUeX: G/W version : %d (correct G/W !!! ) ",RTU_GCR_RTU_VERSION_R(val));
  }
  else
  {
    TRACE(TRACE_INFO,"RTUeX: G/W version : %d (in correct G/W !!! ) ",RTU_GCR_RTU_VERSION_R(val));
    TRACE(TRACE_INFO,"RTUeX: TEST FAILED ");
//     return -1;
  }
      
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
 
   TRACE(TRACE_INFO,"RTU eXtension: set fast forward single mac (id=%d, valid=%d) of"
         "%x:%x:%x:%x:%x:%x", mac_id, valid,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

void rtux_add_ff_mac_range(int mac_id, int valid, uint8_t mac_lower[ETH_ALEN],
                                                   uint8_t mac_upper[ETH_ALEN])
{
   uint32_t mac_hi, mac_lo;
   uint32_t m_mac_id; // modified mac id
   
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


void rtux_set_port_mirror(uint32_t mirror_src_mask, uint32_t mirror_dst_mask, int rx, int tx)
{
   uint32_t mp_src_rx, mp_src_tx, mp_dst, mp_sel;

   TRACE(TRACE_INFO,"RTU eXtension: set port mirroring:" );

   mp_dst    = RTU_RX_MP_R1_MASK_W(mirror_dst_mask);
   mp_src_tx = 0;
   mp_src_rx = 0;

   mp_sel    =  0 ; // destinatioon (dst_src=0)
   rtu_wr(RX_MP_R0,mp_sel);
   rtu_wr(RX_MP_R1,mp_dst);

   if(rx) 
   {
     mp_src_rx = RTU_RX_MP_R1_MASK_W(mirror_src_mask);
     mp_sel    = 0;
     mp_sel    = RTU_RX_MP_R0_DST_SRC; // destination (dst_src=0), reception traffic (rx_tx=0)
     rtu_wr(RX_MP_R0,mp_sel);
     rtu_wr(RX_MP_R1,mp_dst);
   }
    
   if(tx) 
   {
     mp_src_rx = RTU_RX_MP_R1_MASK_W(mirror_src_mask);
     mp_sel    = 0;
     mp_sel    = RTU_RX_MP_R0_DST_SRC | RTU_RX_MP_R0_RX_TX; 
                           // destination (dst_src=0), reception traffic (rx_tx=0)
     rtu_wr(RX_MP_R0,mp_sel);
     rtu_wr(RX_MP_R1,mp_dst);
   }  

   TRACE(TRACE_INFO,"\t mirror output port(s) mask                 (dst)    = 0x%x",mp_dst);
   TRACE(TRACE_INFO,"\t ingress traffic mirror source port(s) mask (src_rx) = 0x%x",mp_src_rx);   
   TRACE(TRACE_INFO,"\t egress  traffic mirror source port(s) mask (src_tx) = 0x%x",mp_src_tx);
     
}

void rtux_set_hp_prio_mask(uint8_t hp_prio_mask)
{
   uint32_t mask;

   mask = rtu_rd(RX_CTR);
   mask = RTU_RX_CTR_PRIO_MASK_W(hp_prio_mask) | mask;
                
   rtu_wr(RX_CTR,mask);
   TRACE(TRACE_INFO,"RTU eXtension: set hp priorities (for which priorities traffic is "
   "considered HP), mask=0x%x",hp_prio_mask );
}

void rtux_set_cpu_port(uint32_t llf_mask)
{
   uint32_t mask;

   mask = RTU_RX_LLF_FF_MASK_W(llf_mask);
                
   rtu_wr(RX_LLF, mask);
   TRACE(TRACE_INFO,"RTU eXtension: set port to which link-limited traffic is forwarded"
   "(from the pool of reserved MAC adresses), mask=0x%x",mask );
}

void rtux_feature_ctrl(int mr, int mac_ptp, int mac_ll, int mac_single, int mac_range, 
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
   mask = 0xFFFFFFC0 & mask; 
   /*$display("RTU eXtension features debugging: 2: cleared mask: 0x%x",mask);*/         
   
   if(mr)         mask = RTU_RX_CTR_MR_ENA              | mask;
   if(mac_ptp)    mask = RTU_RX_CTR_FF_MAC_PTP          | mask;
   if(mac_ll)     mask = RTU_RX_CTR_FF_MAC_LL           | mask;
   if(mac_single) mask = RTU_RX_CTR_FF_MAC_SINGLE       | mask;
   if(mac_range)  mask = RTU_RX_CTR_FF_MAC_RANGE        | mask;
   if(mac_br)     mask = RTU_RX_CTR_FF_MAC_BR           | mask;
   if(at_fm)      mask =  RTU_RX_CTR_AT_FMATCH_TOO_SLOW | mask;
   
   rtu_wr(RX_CTR, mask);
   rtux_disp_ctrl();
//    TRACE(TRACE_INFO,"RTU eXtension features:");
//    if(mr        ){TRACE(TRACE_INFO,"\t Port Mirroring                           - enabled"); }
//    else          {TRACE(TRACE_INFO,"\t Port Mirroring                           - disabled");} 
//    if(mac_ptp   ){TRACE(TRACE_INFO,"\t PTP fast forward                         - enabled"); }
//    else          {TRACE(TRACE_INFO,"\t PTP fast forward                         - disabled");} 
//    if(mac_br    ){TRACE(TRACE_INFO,"\t Broadcast fast forward                   - enabled"); }
//    else          {TRACE(TRACE_INFO,"\t Broadcast fast forward                   - disabled");} 
//    if(mac_ll    ){TRACE(TRACE_INFO,"\t Link-limited traffic (BPDU) fast forward - enabled"); }
//    else          {TRACE(TRACE_INFO,"\t Link-limited traffic (BPDU) fast forward - disabled");}
//    if(mac_single){TRACE(TRACE_INFO,"\t Single configured MACs fast forward      - enabled"); }
//    else          {TRACE(TRACE_INFO,"\t Single configured MACs fast forward      - disabled");}
//    if(mac_range ){TRACE(TRACE_INFO,"\t Range of configured MACs fast forward    - enabled"); }
//    else          {TRACE(TRACE_INFO,"\t Range of configured MACs fast forward    - disabled");} 
//    if(at_fm     ){TRACE(TRACE_INFO,"\t When fast match engine too slow          - braodcast processed frame");} 
//    else          {TRACE(TRACE_INFO,"\t When fast match engine too slow          - drop processed frame"); }

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
     {TRACE(TRACE_INFO,"\t (64) When fast match engine too slow          - drop processed frame"); }

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
      rtux_set_cpu_port(1<<sub_opt);
    break;
    case  3:
      rtux_set_hp_prio_mask(1<<sub_opt);
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
    case  10:

      rtux_disp_ctrl() ;
       
    break;
    default:
       TRACE(TRACE_INFO, "RTU extension control options");
       TRACE(TRACE_INFO, "Usage: wrsw_rtu -x [<option> <value>]");
       TRACE(TRACE_INFO, "-x 0         show this info");
       TRACE(TRACE_INFO, "-x 1  mask   set ctrl options, mask in decimal format:");
       TRACE(TRACE_INFO, "             mask = d'{at_fm,mac_br,mac_range,mac_single, mac_ll, mac_ptp, mr}");
       TRACE(TRACE_INFO, "-x 2  p_id   sets port of p_id to be cpu virual port recognized in fast match");
       TRACE(TRACE_INFO, "-x 3  prio   sets prio to be recognized as HP");
       TRACE(TRACE_INFO, "-x 4  opt    MIRRORING configuration (the mirroring needs to be enabled with set ctrl)");
       TRACE(TRACE_INFO, "-x 4  1      sets traffic on port 0 (tx/rx) to be mirrored on port 7");
       TRACE(TRACE_INFO, "-x 4  2      sets traffic on port 7 (rx) to be mirrored on port 1");
       TRACE(TRACE_INFO, "-x 4  3      sets traffic on port 5 (tx/rx) to be mirrored on port 7");
       TRACE(TRACE_INFO, "-x 10        show status");
  };
  exit(1);
  
}