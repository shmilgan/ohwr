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
#include <regs/tru-regs.h>

#include "rtu_tru_drv.h"
#include "wr_rtu.h"

#define m_dbg 1

#define tru_rd(reg) \
	 _fpga_readl(FPGA_BASE_TRU + offsetof(struct TRU_WB, reg))

#define tru_wr(reg, val) \
	 _fpga_writel(FPGA_BASE_TRU + offsetof(struct TRU_WB, reg), val)

extern int shw_fpga_mmap_init();
	 
#define TRU_TAB_SUBENTRY_NUM 8 /* as in H/W */  
int tru_init(int tru_enabled )
{
   uint32_t val;
   int i;
   /**/
   tru_disable();
   
   tru_simple_test();
   tru_pattern_config(1/*replacement*/,2/*addition*/);
   tru_lacp_config(0 /* df_hp_id */,2 /* df_br_id */,1 /* df_un_id */);
//    tru_rt_reconf_config(4 /*tx_frame_id*/, 4/*rx_frame_id*/, 1 /*mode*/);
//    tru_rt_reconf_enable();
//    tru_transition_config(0          /*mode */,     
//                          4          /*rx_id*/,
//                          1          /*prio mode*/, 
//                          0          /*prio*/, 
//                          20         /*time_diff*/, 
//                          3          /*port_a_id*/, 
//                          4          /*port_b_id*/);
  
   //clear tru tab subentries for FID=0
   for(i=0;i<TRU_TAB_SUBENTRY_NUM;i++)
     tru_write_tab_entry(0,0,i,0,0,0,0,0,0);
     
   /// port 1 active
   /// port 2 backup

  tru_set_port_roles(1 /*active port*/,2/*backup port*/);
//   tru_set_port_roles(2 /*active port*/,1/*backup port*/);
  if(tru_enabled)
    tru_enable();

  return 0;
}


// --------------------------------------------
// RTU eXtension
// --------------------------------------------

int tru_simple_test() 
{
  hexp_port_list_t ports;
  uint32_t val;
  
  halexp_query_ports(&ports);
  tru_show_status(ports.num_ports) ;
 
  return 0;
}

void tru_show_status(int num_ports) 
{
  uint32_t bank, ports_up, ports_stb_up;
  int i, port_pass_all;
  uint32_t val;
  
  val = tru_rd(GCR);
  if(TRU_GCR_G_ENA & val)
     {TRACE(TRACE_INFO,"TRU: enabled");}
  else
     {TRACE(TRACE_INFO,"TRU: disabled");}    
  
  tru_read_status(&bank, &ports_up, &ports_stb_up,0);
  TRACE(TRACE_INFO, "TRU port state display [port num=%d]",num_ports);
  for(i=0;i< num_ports;i++)
  {
    port_pass_all = rtu_port_setting(i);
    if(((ports_up>>i)&0x1) && ((ports_stb_up>>i)&0x1)) 
    {
      if(port_pass_all)
        {TRACE(TRACE_INFO, "Port %2d -  up  | enabled  | (stabily up)",i);}
      else
        {TRACE(TRACE_INFO, "Port %2d -  up  | disabled | (stabily up)",i);}
    }
    else if(!((ports_up>>i)&0x1) && ((ports_stb_up>>i)&0x1)) 
    {
      if(port_pass_all)
        {TRACE(TRACE_INFO, "Port %2d - down | enabled  | (stabily up)",i);}
      else
        {TRACE(TRACE_INFO, "Port %2d - down | disabled | (stabily up)",i);}
    }
    else if(!((ports_up>>i)&0x1) && !((ports_stb_up>>i)&0x1)) 
    {
      if(port_pass_all)
        {TRACE(TRACE_INFO, "Port %2d - down | enabled  | (not stabily up)",i);}
      else
        {TRACE(TRACE_INFO, "Port %2d - down | disabled | (not stabily up)",i);}
    }
    else 
      {TRACE(TRACE_INFO, "Port %2d - not expected state",i);}
  }
  tru_transition_status();
}

int tru_port_state_up(int port_id)
{
   uint32_t bank, ports_up, ports_stb_up;  
   tru_read_status(&bank, &ports_up, &ports_stb_up,0);
   return (0x1 & (ports_up >> port_id));
}

uint32_t tru_port_stable_up_mask(void)
{
   uint32_t bank, ports_up, ports_stb_up;  
   tru_read_status(&bank, &ports_up, &ports_stb_up,0);
   return ports_stb_up;
}

void tru_write_tab_entry(int valid,      int fid,          int subfid, 
                         int patrn_mask, int patrn_match,  int patrn_mode,
                         int ports_mask, int ports_egress, int ports_ingress)
{
   
   uint32_t ttr0;   
   tru_wr(TTR1,TRU_TTR1_PORTS_INGRESS_W(ports_ingress));
   tru_wr(TTR2,TRU_TTR2_PORTS_EGRESS_W (ports_egress));
   tru_wr(TTR3,TRU_TTR3_PORTS_MASK_W   (ports_mask));
   tru_wr(TTR4,TRU_TTR4_PATRN_MATCH_W  (patrn_match));
   tru_wr(TTR5,TRU_TTR5_PATRN_MASK_W   (patrn_mask));
  
   // write
   if(valid)
   {
     ttr0 = TRU_TTR0_FID_W       (fid)       | 
            TRU_TTR0_SUB_FID_W   (subfid)    |
            TRU_TTR0_PATRN_MODE_W(patrn_mode)|
            TRU_TTR0_MASK_VALID              |
            TRU_TTR0_UPDATE                  ; 
   }
   else
     ttr0 = TRU_TTR0_UPDATE                  ; 
   
   tru_wr(TTR0, ttr0);
   
   if(m_dbg & valid) 
   {
      TRACE(TRACE_INFO,"TRU: TAB entry write [fid = %2d, subfid = %2d, pattern mode = %2d]:",
                                               fid, subfid, patrn_mode);
      if(patrn_mode==0) 
         {TRACE(TRACE_INFO,"\t Pattern Mode   : replace masked bits of the port mask");}
      if(patrn_mode==1) 
         {TRACE(TRACE_INFO,"\t Pattern Mode   : add     masked bits of the port mask");}
      if(patrn_mode==2) 
         {TRACE(TRACE_INFO,"\t Pattern Mode   : add     port status based masked bits");}
      if(patrn_mode > 2)
         {TRACE(TRACE_INFO,"\t Pattern Mode   : error, unrecognized mode");}
      TRACE(TRACE_INFO,"\t Ingress config : port  = 0x%x , mask = 0x%x",ports_ingress,
          ports_mask);
      TRACE(TRACE_INFO,"\t Egress  config : port  = 0x%x , mask = 0x%x",ports_egress, 
          ports_mask);
      TRACE(TRACE_INFO,"\t Pattern config : match = 0x%x , mask = 0x%x",patrn_match, 
          patrn_mask);
   }
   if(m_dbg & !valid) 
      {TRACE(TRACE_INFO,"TRU: TAB entry clear [fid = %2d, subfid = %2d]:",fid, subfid);}
//    tru_swap_bank();
}

void tru_transition_config(int mode,      int rx_id,   int prio_mode,    int prio, int time_diff,
                           int port_a_id, int port_b_id)
{
   uint32_t val=0;
   if(prio_mode)
     val = TRU_TCGR_TRANS_PRIO_MODE              |
           TRU_TCGR_TRANS_MODE_W     (mode)      |
           TRU_TCGR_TRANS_RX_ID_W    (rx_id)     |
           TRU_TCGR_TRANS_PRIO_W     (prio)      |
           TRU_TCGR_TRANS_TIME_DIFF_W(time_diff) ;
   else
     val = TRU_TCGR_TRANS_MODE_W     (mode)      |
           TRU_TCGR_TRANS_RX_ID_W    (rx_id)     |
           TRU_TCGR_TRANS_PRIO_W     (prio)      |
           TRU_TCGR_TRANS_TIME_DIFF_W(time_diff) ;
     
   
   tru_wr(TCGR,val);

   val = 0;
   val = TRU_TCPR_TRANS_PORT_A_ID_W(port_a_id) |
         TRU_TCPR_TRANS_PORT_B_ID_W(port_b_id) |
         TRU_TCPR_TRANS_PORT_A_VALID           |
         TRU_TCPR_TRANS_PORT_B_VALID           ;
   tru_wr(TCPR,val);     

   if(m_dbg) 
   {
      TRACE(TRACE_INFO,"TRU: transition configuration [mode id = %2d]:",mode);
      if(mode == 0) 
         {TRACE(TRACE_INFO,"\tMode   : marker triggered");}
      if(mode == 1) 
         {TRACE(TRACE_INFO,"\tMode   : LACP distributor");}
      if(mode == 1)  
         {TRACE(TRACE_INFO,"\tMode   : LACP collector");}
      TRACE(TRACE_INFO,"\tPorts  : A_ID = %2d (before tran), B_ID = %2d (after trans)",
         port_a_id, port_b_id);
      TRACE(TRACE_INFO,"\tParams : Rx Frame ID =  %2d, PrioMode = %1d, Priority = %2d, Time diff = %3d", 
         rx_id, prio_mode, prio, time_diff);
   }    
}
void tru_transition_enable()
{
   uint32_t val;
   val = tru_rd(TCGR);
   val = TRU_TCGR_TRANS_ENA | val;
   tru_wr(TCGR, val);    
   if(m_dbg) 
      { TRACE(TRACE_INFO,"TRU: enable transition");}
}

void tru_transition_disable()
{
   uint32_t val;
   val = tru_rd(TCGR);
   val = (~TRU_TCGR_TRANS_ENA) & val;
   tru_wr(TCGR, val);
   if(m_dbg) 
      {TRACE(TRACE_INFO,"TRU: disable transition");}
}

void tru_transition_clear()
{
   uint32_t val;
   val = tru_rd(TCGR);
   val = TRU_TCGR_TRANS_CLEAR | val;      
   if(m_dbg) 
      { TRACE(TRACE_INFO,"TRU: clear transition");}
}

void tru_transition_status()
{
   uint32_t val1, val2;
   val1 = tru_rd(TCGR);
   val2 = tru_rd(TSR);
   if(val1 & TRU_TCGR_TRANS_ENA)
   { 
     TRACE(TRACE_INFO,"TRU->transition status: trans_finished=%1d, trans_active=%1d",
                      ((val2 & TRU_TSR_TRANS_STAT_FINISHED) != 0) ? 1 : 0 ,
                       (val2 & TRU_TSR_TRANS_STAT_ACTIVE));
   }
   else
   {
     TRACE(TRACE_INFO,"TRU->transition status: transition disabled");
   }
}


void tru_pattern_config(uint32_t replacement,  uint32_t addition)
{
   uint32_t val;
   val = TRU_MCR_PATTERN_MODE_REP_W(replacement) |
         TRU_MCR_PATTERN_MODE_ADD_W(addition)    ;
   tru_wr(MCR, val);  
   
   if(m_dbg) 
   {
      TRACE(TRACE_INFO,"TRU: Real Time transition source of patterns config: ");
      TRACE(TRACE_INFO,"\tReplacement pattern ID = %d: ",replacement);
      TRACE(TRACE_INFO,"\tAddition    pattern ID = %d: ",addition);
      TRACE(TRACE_INFO,"\tChoice info: ");
      TRACE(TRACE_INFO,"\t\t0: non: zeros ");
      TRACE(TRACE_INFO,"\t\t1: ports status (bit HIGH when port down");
      TRACE(TRACE_INFO,"\t\t2: received special frames - filtered by endpoints according to" 
                              "configuration (pfliter in endpoint + RTR_RX class ID in " 
                              "Real Time Reconfiguration Control Register)");
      TRACE(TRACE_INFO,"\t\t3: according to aggregation ID (the source of the ID depends on "
                              "the traffic kind: HP/Broadcast/Uniast, set in Link Aggregation "
                              "Control Register)");
      TRACE(TRACE_INFO,"\t\t4: received port");
   }
}

void tru_enable()
{
  uint32_t val;
  
  val = tru_rd(GCR);
  val = TRU_GCR_G_ENA | val;
  tru_wr(GCR,  val);    
//   TRACE(TRACE_INFO,"TRU: enabled");
  if(m_dbg) 
     {TRACE(TRACE_INFO,"TRU: enabled");}
}
void tru_disable()
{
  uint32_t val;
  val = tru_rd(GCR);
  val = (~TRU_GCR_G_ENA) & val;
  tru_wr(GCR,  val);    
  if(m_dbg) 
     {TRACE(TRACE_INFO,"TRU: disabled");}
}


void tru_swap_bank()
{
   uint32_t val;
   val = tru_rd(GCR);
   val = TRU_GCR_TRU_BANK | val;
   tru_wr(GCR, val);    
   if(m_dbg) 
      {TRACE(TRACE_INFO,"TRU: swap TABLE banks");}
}

void tru_rx_frame_reset(uint32_t reset_rx)
{
   uint32_t val;
   val = tru_rd(GCR);
   val = TRU_GCR_RX_FRAME_RESET_W(reset_rx) | (val & (~TRU_GCR_RX_FRAME_RESET_MASK));
   tru_wr(GCR, val);
   if(m_dbg) 
      {TRACE(TRACE_INFO,"TRU: reset rx frame register (foget received frames)");}
}

void tru_rt_reconf_config(int tx_frame_id, int rx_frame_id, int mode)
{
   uint32_t val;
   val = tru_rd(RTRCR);
   val = (val & 0x00000001)                |
         TRU_RTRCR_RTR_MODE_W(mode       ) |
         TRU_RTRCR_RTR_RX_W  (rx_frame_id) |
         TRU_RTRCR_RTR_TX_W  (tx_frame_id) ;
   tru_wr(RTRCR, val);
   if(m_dbg) 
   {
      TRACE(TRACE_INFO,"TRU: Real Time re-configuration Mode [%2d]:",mode);
      TRACE(TRACE_INFO,"\tFrames: rx_id = %2d, tx_id = %2d", rx_frame_id, tx_frame_id);
      if(mode == 0) 
         {TRACE(TRACE_INFO,"\tMode  : default (do nothing)");}
      if(mode == 1) 
         {TRACE(TRACE_INFO,"\tMode  : eRSTP (send HW-generated frames on port down, etc...)");}
      if(mode > 1) 
         {TRACE(TRACE_INFO,"\tMode  : undefined");}
   }
}

void tru_rt_reconf_enable()
{
   uint32_t val;
   val = tru_rd(RTRCR);
   val = TRU_RTRCR_RTR_ENA | val;
   tru_wr(RTRCR, val);
   if(m_dbg) 
      {TRACE(TRACE_INFO,"TRU: Real Time re-configuration enable");}
}

void tru_rt_reconf_disable()
{
   uint32_t val;
   val = tru_rd(RTRCR);
   val = (~TRU_RTRCR_RTR_ENA) | val;
   tru_wr(RTRCR, val);
   if(m_dbg) 
      {TRACE(TRACE_INFO,"TRU: Real Time re-configuration disable");}
}

void tru_rt_reconf_reset()
{
   uint64_t val;
   val = tru_rd(RTRCR);
   val = TRU_RTRCR_RTR_RESET | val;    
   tru_wr(RTRCR, val);
   if(m_dbg) 
      {TRACE(TRACE_INFO,"TRU: Real Time re-configuration reset (memory)");}
}
   
void tru_read_status(uint32_t *bank, uint32_t *ports_up, uint32_t *ports_stb_up, int display)
{
   uint32_t val;
      
   val = tru_rd(GSR0); 
   
   *bank         = TRU_GSR0_STAT_BANK & val;
   *ports_stb_up = TRU_GSR0_STAT_STB_UP_R(val);
      
   val = tru_rd(GSR1);
   *ports_up     = TRU_GSR1_STAT_UP_R(val);
   
   if(m_dbg & display) 
   {
      TRACE(TRACE_INFO,"TRU: status read:");
      TRACE(TRACE_INFO,"\tactive TABLE bank           : %2d",  *bank);
      TRACE(TRACE_INFO,"\tports status (1:up, 0:down) : 0x%x", *ports_up);
      TRACE(TRACE_INFO,"\tports stable (1:up, 0:down) : 0x%x", *ports_stb_up);
   }
}

void tru_lacp_config(uint32_t df_hp_id, uint32_t df_br_id, uint32_t df_un_id)
{
   uint64_t val;
   val = TRU_LACR_AGG_DF_HP_ID_W(df_hp_id) |
         TRU_LACR_AGG_DF_BR_ID_W(df_br_id) |
         TRU_LACR_AGG_DF_UN_ID_W(df_un_id) ;
   tru_wr(LACR, val);    
   if(m_dbg)
   {
     TRACE(TRACE_INFO,"TRU: Link Aggregation config:");
     TRACE(TRACE_INFO,"\tDistribution Function for High Priority traffic: id = %2d",df_hp_id);
     TRACE(TRACE_INFO,"\tDistribution Function for Broadcast     traffic: id = %2d",df_br_id);
     TRACE(TRACE_INFO,"\tDistribution Function for Unicast       traffic: id = %2d",df_un_id);
   }
}

void tru_ep_debug_read_pfilter(uint32_t port)
{
   uint64_t val;
   tru_wr(DPS, TRU_DPS_PID_W(port));
   val = tru_rd(PFDR);
//    if(m_dbg)
//    {
      TRACE(TRACE_INFO, "TRU-DBG [pFILTER-port_%2d]: filtered classes = 0x%2x [cnt_filtered=%4d, cnt_all=%4d, raw=0x%4x]",port,       
              (uint32_t)TRU_PFDR_CLASS_R(val), 
              0xFF & (uint32_t)TRU_PFDR_CNT_R(val), 
              0xFF & ((uint32_t)TRU_PFDR_CNT_R(val)>>8), 
              (uint32_t)val);
              
//    }
}
void tru_ep_debug_clear_pfilter(uint32_t port)
{
   tru_wr(DPS, TRU_DPS_PID_W(port));
   tru_wr(PFDR, TRU_PFDR_CLR);
   if(m_dbg)
   {
      TRACE(TRACE_INFO, "TRU-DBG [pFILTER-port_%2d]: filtered packet classes & cnt cleared",port);
   }
}

void tru_ep_debug_inject_packet(uint32_t port, uint32_t user_val, uint32_t pck_sel)
{
   uint64_t val;
   tru_wr(DPS, TRU_DPS_PID_W(port));
   val =             TRU_PIDR_INJECT |
          TRU_PIDR_PSEL_W(pck_sel)   |
          TRU_PIDR_UVAL_W(user_val);
   tru_wr(PIDR, val);    
   if(m_dbg)
   {
      TRACE(TRACE_INFO, "TRU-DBG [pINJECT-port_%2d]: inject packet: pck_sel=%2d, user_val=0x%x",
                        port,pck_sel, user_val);
   }   
}

void tru_ep_debug_read_pinject(uint32_t port)
{
   uint64_t val;
   int ready;
   tru_wr(DPS, TRU_DPS_PID_W(port));
   val = tru_rd(PIDR);
   if(val & TRU_PIDR_IREADY) ready = 1; else ready = 0;
//    if(m_dbg)
//    {
      TRACE(TRACE_INFO, "TRU-DBG [pINJECT-port_%2d]: inject   ready   =   %2d [                               raw=0x%4x] ", port,ready,val);
//    }               
}
void tru_ep_debug_status(uint32_t ports_num)
{
  uint32_t i;
  for(i=0;i<ports_num;i++)
  {
    tru_ep_debug_read_pfilter(i);
    tru_ep_debug_read_pinject(i);
  }
}

void tru_set_port_roles(int active_port, int backup_port)
{
  if(active_port == 1 && backup_port == 2)
  {
   
//       tru_write_tab_entry(  1          /* valid     */,
//                             0          /* entry_addr   */,    
//                             0          /* subentry_addr*/,
//                             0x00000000 /*pattern_mask*/, 
//                             0x00000000 /* pattern_match*/,   
//                             0x000      /* pattern_mode */,
//                             0x000000FF /*ports_mask  */, 
//                             0x00000083 /* ports_egress */,      
//                             0x00000083 /* ports_ingress   */); 
// 
//       tru_write_tab_entry(  1          /* valid     */,
//                             0          /* entry_addr   */,    
//                             1          /* subentry_addr*/,
//                             0x00000006 /*pattern_mask*/, 
//                             0x00000002 /* pattern_match*/,   
//                             0x000      /* pattern_mode */,
//                             0x00000006 /*ports_mask  */, 
//                             0x00000004 /* ports_egress */,     
//                             0x00000004 /* ports_ingress   */);        
    
      tru_write_tab_entry(  1          /* valid     */,
                            0          /* entry_addr   */,    
                            0          /* subentry_addr*/,
                            0x00000000 /*pattern_mask*/, 
                            0x00000000 /* pattern_match*/,   
                            0x000      /* pattern_mode */,
                            0x000000FF /*ports_mask  */, 
                            0x00000087 /* ports_egress */,      
                            0x00000083 /* ports_ingress   */); 

      tru_write_tab_entry(  1          /* valid     */,
                            0          /* entry_addr   */,    
                            1          /* subentry_addr*/,
                            0x00000006 /*pattern_mask*/, 
                            0x00000002 /* pattern_match*/,   
                            0x000      /* pattern_mode */,
                            0x00000006 /*ports_mask  */, 
                            0x00000006 /* ports_egress */,     
                            0x00000004 /* ports_ingress   */);    
      tru_swap_bank();
      TRACE(TRACE_INFO, "PORT ROLES: active port %d, backup port %d", active_port,backup_port);
     }   
   else if(active_port == 2 && backup_port == 1)
   {
//       tru_write_tab_entry(  1          /* valid     */,
//                             0          /* entry_addr   */,    
//                             0          /* subentry_addr*/,
//                             0x00000000 /*pattern_mask*/, 
//                             0x00000000 /* pattern_match*/,   
//                             0x000      /* pattern_mode */,
//                             0x000000FF /*ports_mask  */, 
//                             0x00000085 /* ports_egress */,      
//                             0x00000085 /* ports_ingress   */); 
//       tru_write_tab_entry(  1          /* valid     */,
//                             0          /* entry_addr   */,    
//                             1          /* subentry_addr*/,
//                             0x00000006 /*pattern_mask*/, 
//                             0x00000004 /* pattern_match*/,   
//                             0x000      /* pattern_mode */,
//                             0x00000006 /*ports_mask  */, 
//                             0x00000002 /* ports_egress */,     
//                             0x00000002 /* ports_ingress   */);    
      
      tru_write_tab_entry(  1          /* valid     */,
                            0          /* entry_addr   */,    
                            0          /* subentry_addr*/,
                            0x00000000 /*pattern_mask*/, 
                            0x00000000 /* pattern_match*/,   
                            0x000      /* pattern_mode */,
                            0x000000FF /*ports_mask  */, 
                            0x00000087 /* ports_egress */,      
                            0x00000085 /* ports_ingress   */); 
      tru_write_tab_entry(  1          /* valid     */,
                            0          /* entry_addr   */,    
                            1          /* subentry_addr*/,
                            0x00000006 /*pattern_mask*/, 
                            0x00000004 /* pattern_match*/,   
                            0x000      /* pattern_mode */,
                            0x00000006 /*ports_mask  */, 
                            0x00000006 /* ports_egress */,     
                            0x00000002 /* ports_ingress   */);        
      tru_swap_bank();
      TRACE(TRACE_INFO, "PORT ROLES: active port %d, backup port %d", active_port,backup_port);
   }
   else
   {
      TRACE(TRACE_INFO, "PORT ROLES: setting not supported [active port %d, backup port %d]", 
                         active_port,backup_port);
   }  
   
}

void tru_set_life(char *optarg)
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
       if(sub_opt == 1)
	 tru_enable();
       else
         tru_disable(); 
    break;
    case  2:
       tru_rt_reconf_reset();
    break;
    case  3:
       if(sub_opt == 1)
       {
         tru_rt_reconf_config(1 /*tx_frame_id*/, 1/*rx_frame_id*/, 1 /*mode*/);
         tru_rt_reconf_enable();
       }
       else
         tru_rt_reconf_disable();
    break;
    case  4:
       switch(sub_opt){
          case 1:
             tru_write_tab_entry(  1          /* valid     */,
                                   0          /* entry_addr   */,    
                                   0          /* subentry_addr*/,
                                   0x00000000 /*pattern_mask*/, 
                                   0x00000000 /* pattern_match*/,   
                                   0x000      /* pattern_mode */,
                                   0x000000FF /*ports_mask  */, 
                                   0x000000F3 /* ports_egress */,     //32'b111000000010110001
                                   0x000000F3 /* ports_ingress   */); //32'b111000000010110001
              break;
          case 2:
             tru_write_tab_entry(  1          /* valid     */,
                                   0          /* entry_addr   */,    
                                   0          /* subentry_addr*/,
                                   0x00000000 /*pattern_mask*/, 
                                   0x00000000 /* pattern_match*/,   
                                   0x000      /* pattern_mode */,
                                   0x0003FFFF /*ports_mask  */, 
                                   0x0003FFFF /* ports_egress */,     //32'b111000000010110001
                                   0x0003FFFF /* ports_ingress   */); //32'b111000000010110001
             break;
          case 3:
             tru_write_tab_entry(  1          /* valid     */,
                                   0          /* entry_addr   */,    
                                   0          /* subentry_addr*/,
                                   0x00000003 /*pattern_mask*/, 
                                   0x00000001 /* pattern_match*/,   
                                   0x000      /* pattern_mode */,
                                   0x00000003 /*ports_mask  */, 
                                   0x00000002 /* ports_egress */,     //32'b111000000010110001
                                   0x00000002 /* ports_ingress   */); //32'b111000000010110001	
             break;
          case 4: 
             tru_write_tab_entry(  1          /* valid     */,
                                   0          /* entry_addr   */,    
                                   0          /* subentry_addr*/,
                                   0x00000000 /*pattern_mask*/, 
                                   0x00000000 /* pattern_match*/,   
                                   0x000      /* pattern_mode */,
                                   0x000000FF /*ports_mask  */, 
                                   0x00000083 /* ports_egress */,      
                                   0x00000083 /* ports_ingress   */); 	     
             break;
          case 5: 
             tru_write_tab_entry(  1          /* valid     */,
                                   0          /* entry_addr   */,    
                                   0          /* subentry_addr*/,
                                   0x00000000 /*pattern_mask*/, 
                                   0x00000000 /* pattern_match*/,   
                                   0x000      /* pattern_mode */,
                                   0x000000FF /*ports_mask  */, 
                                   0x00000085 /* ports_egress */,      
                                   0x00000085 /* ports_ingress   */); 	     
             break;
          case 6: 
             tru_write_tab_entry(  1          /* valid     */,
                                   0          /* entry_addr   */,    
                                   1          /* subentry_addr*/,
                                   0x00000006 /*pattern_mask*/, 
                                   0x00000002 /* pattern_match*/,   
                                   0x000      /* pattern_mode */,
                                   0x00000006 /*ports_mask  */, 
                                   0x00000004 /* ports_egress */,      
                                   0x00000004 /* ports_ingress   */); 	     
             break;	     
          case 7: 
             tru_write_tab_entry(  1          /* valid     */,
                                   0          /* entry_addr   */,    
                                   1          /* subentry_addr*/,
                                   0x00000006 /*pattern_mask*/, 
                                   0x00000004 /* pattern_match*/,   
                                   0x000      /* pattern_mode */,
                                   0x00000006 /*ports_mask  */, 
                                   0x00000002 /* ports_egress */,      
                                   0x00000002 /* ports_ingress   */); 	     
             break;	     
          case 8: 
             // basic config, excluding link aggregation, only the standard non-LACP ports
             tru_write_tab_entry(  1          /* valid         */,
                                   0          /* entry_addr    */,    
                                   0          /* subentry_addr */,
                                   0x00000000 /* pattern_mask  */, 
                                   0x00000000 /* pattern_match */,   
                                   0x000      /* pattern_mode  */,
                                   0x00000F0F /* ports_mask    */, 
                                   0x00000F0F /* ports_egress  */,      
                                   0x00000F0F /* ports_ingress */); 
             // a bunch of link aggregation ports (ports 4 to 7 and 12&15)
             // received FEC msg of class 0      
             tru_write_tab_entry(  1          /* valid         */,
                                   0          /* entry_addr    */,    
                                   1          /* subentry_addr */,
                                   0x0000000F /* pattern_mask  */, 
                                   0x00000001 /* pattern_match */,   
                                   0x000      /* pattern_mode  */,
                                   0x000090F0 /* ports_mask    */, 
                                   0x00008010 /* ports_egress  */,      
                                   0x00000000 /* ports_ingress */); 
             // received FEC msg of class 1
             tru_write_tab_entry(  1          /* valid         */,
                                   0          /* entry_addr    */,    
                                   2          /* subentry_addr */,
                                   0x0000000F /* pattern_mask  */, 
                                   0x00000002 /* pattern_match */,   
                                   0x000      /* pattern_mode  */,
                                   0x000090F0 /* ports_mask    */, 
                                   0x00008020 /* ports_egress  */,      
                                   0x00000000 /* ports_ingress */);
             // received FEC msg of class 2
             tru_write_tab_entry(  1          /* valid         */,
                                   0          /* entry_addr    */,    
                                   3          /* subentry_addr */,
                                   0x0000000F /* pattern_mask  */, 
                                   0x00000004 /* pattern_match */,   
                                   0x000      /* pattern_mode  */,
                                   0x000090F0 /* ports_mask    */, 
                                   0x00001040 /* ports_egress  */,      
                                   0x00000000 /* ports_ingress */);
             // received FEC msg of class 3
             tru_write_tab_entry(  1          /* valid         */,
                                   0          /* entry_addr    */,    
                                   4          /* subentry_addr */,
                                   0x0000000F /* pattern_mask  */, 
                                   0x00000008 /* pattern_match */,   
                                   0x000      /* pattern_mode  */,
                                   0x000090F0 /* ports_mask    */, 
                                   0x00001080 /* ports_egress  */,      
                                   0x00000000 /* ports_ingress */);
             // collector: receiving frames on the aggregation ports, 
             // forwarding to "normal" (others)
             tru_write_tab_entry(  1          /* valid         */,
                                   0          /* entry_addr    */,    
                                   5          /* subentry_addr */,
                                   0x000090F0 /* pattern_mask  */, 
                                   0x000090F0 /* pattern_match */,   
                                   0x002      /* pattern_mode  */,
                                   0x000090F0 /* ports_mask    */, 
                                   0x00000000 /* ports_egress  */,      
                                   0x000090F0 /* ports_ingress */);

             break;	     
         case 10:
             tru_swap_bank();
             break;
          default:
             TRACE(TRACE_INFO, "bad option");
       };
       break;
    case  5:
       //writing new entries (the banks of TRU TAB wil be swapped on transition
       tru_write_tab_entry(  1          /* valid     */,
                             0          /* entry_addr   */,    
                             0          /* subentry_addr*/,
                             0x00000000 /*pattern_mask*/, 
                             0x00000000 /* pattern_match*/,   
                             0x000      /* pattern_mode */,
                             0x000000FF /*ports_mask  */, 
                             0x00000087 /* ports_egress: 1000_0111  */,     
                             0x00000085 /* ports_ingress 1000_0101  */);         
       tru_write_tab_entry(  1          /* valid     */,
                             0          /* entry_addr   */,    
                             1          /* subentry_addr*/,
                             0x00000000 /*pattern_mask*/, 
                             0x00000004 /* pattern_match*/,   
                             0x000      /* pattern_mode */,
                             0x00000006 /*ports_mask  */, 
                             0x00000087 /* ports_egress  1000_0111 */,      
                             0x00000082 /* ports_ingress 1000_0010  */);
             //program transition
       tru_transition_config(0          /*mode */,     
                             1          /*rx_id*/,
                             1          /*prio mode*/,                               
                             0          /*prio*/, 
                             20         /*time_diff*/, 
                             1          /*port_a_id*/, 
                             2          /*port_b_id*/);
       tru_transition_enable();      
       break;  
    case  6:
       ep_pfilter_status_N_ports((uint32_t)8);
       break;
    case  7:
       ep_pfilter_reload_code(sub_opt);
       break;
    case  8:
       pfilter_disable((uint32_t)sub_opt);
       break;
    case  9:
       tru_ep_debug_status((uint32_t)8);
       break;   
    case  10:
       tru_ep_debug_clear_pfilter((uint32_t)sub_opt);
       break;       
    case  11:
       tru_ep_debug_read_pfilter((uint32_t)sub_opt);
       break;       
    case  12:
       tru_ep_debug_inject_packet((uint32_t)sub_opt, 0xBABE, 0);
       break;       
    case  13:
       tru_ep_debug_inject_packet((uint32_t)sub_opt, 0xBABE, 1);
       break;       
    
    case  100:
       tru_show_status(18) ;
       
    break;
    default:
       TRACE(TRACE_INFO, "TRU control options");
       TRACE(TRACE_INFO, "Usage: wrsw_rtu -u [<option> <value>]");
       TRACE(TRACE_INFO, "-u 0         show this info");
       TRACE(TRACE_INFO, "-u 1  1/0    TRU enable/disable");
       TRACE(TRACE_INFO, "-u 2         Real Time re-configuration reset (memory)");
       TRACE(TRACE_INFO, "-u 3  1/0    Real Time re-configuration enable[tx_id=1, rx_id=1, mode=1]/disable");
       TRACE(TRACE_INFO, "-u 4  1      Write TRU Table: FID=0, subFID=0");
       TRACE(TRACE_INFO, "             ports   = 10110001 [mask=11111111]");
       TRACE(TRACE_INFO, "             pattern = 00000000 [mask=00000000]");
       TRACE(TRACE_INFO, "-u 4  2      Write TRU Table: FID=0, subFID=0");
       TRACE(TRACE_INFO, "             ports   = 10110001 [mask=11111111]");
       TRACE(TRACE_INFO, "             pattern = 00000000 [mask=00000000]");
       TRACE(TRACE_INFO, "-u 4  3      Write TRU Table: FID=0, subFID=0");
       TRACE(TRACE_INFO, "             ports   = 00000010 [mask=00000011]");
       TRACE(TRACE_INFO, "             pattern = 00000001 [mask=00000011]");
       TRACE(TRACE_INFO, "-u 4  4      Write TRU Table: FID=0, subFID=0");
       TRACE(TRACE_INFO, "             ports   = 10000011 [mask=11111111]");
       TRACE(TRACE_INFO, "             pattern = 00000000 [mask=00000000]");       
       TRACE(TRACE_INFO, "-u 4  5      Write TRU Table: FID=0, subFID=0");
       TRACE(TRACE_INFO, "             ports   = 10000101 [mask=11111111]");
       TRACE(TRACE_INFO, "             pattern = 00000000 [mask=00000000]"); 
       TRACE(TRACE_INFO, "-u 4  6      Write TRU Table: FID=0, subFID=1");
       TRACE(TRACE_INFO, "             ports   = 10000100 [mask=00000110]");
       TRACE(TRACE_INFO, "             pattern = 00000010 [mask=00000110]");           
       TRACE(TRACE_INFO, "-u 4  7      Write TRU Table: FID=0, subFID=1");
       TRACE(TRACE_INFO, "             ports   = 10000010 [mask=00000110]");
       TRACE(TRACE_INFO, "             pattern = 00000100 [mask=00000110]"); 
       TRACE(TRACE_INFO, "-u 4  8      LACP config --  Ports are as follows: ");          
       TRACE(TRACE_INFO, "             -  0  - 3  *normal ports*      -- ports are not in any agg group");
       TRACE(TRACE_INFO, "             -  4  - 7  *aggregation group* -- ports in single aggr group");
       TRACE(TRACE_INFO, "             -  13 & 15 *aggregation group* -- ports in single aggr group (not for 8-port switch) ");
       TRACE(TRACE_INFO, "             -  14 & 16 *blocked ports*     -- (not for 8-port switch) ");
       TRACE(TRACE_INFO, "-u 4  10     Commit TRU TAB chagnes (swap banks)");
       TRACE(TRACE_INFO, "-u 5         Transition/injection/filter");
       TRACE(TRACE_INFO, "-u 6         Endpoint filter: show status");
       TRACE(TRACE_INFO, "-u 7  n      Endpoint filter: load + enable  on port n");
       TRACE(TRACE_INFO, "-u 8  n      Endpoint filter: disable on port n");
       TRACE(TRACE_INFO, "-u 9         DEBUG-EP: status ");
       TRACE(TRACE_INFO, "-u 10 n      DEBUG-EP: clear pfilter record on port n");
       TRACE(TRACE_INFO, "-u 11 n      DEBUG-EP: show  pfilter record on port n");
       TRACE(TRACE_INFO, "-u 12 n      DEBUG-EP: inject PAUSE  packet on port n [user_val=0xBABE]");
       TRACE(TRACE_INFO, "-u 13 n      DEBUG-EP: inject BPDU packet on port n [user_val=0xBABE]");
       TRACE(TRACE_INFO, "-u 100       show status");
  };
  exit(1);
  
}