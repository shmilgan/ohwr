/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2013, CERN.
 *
 * Version:     wrsw_rtud v1.1
 *
 * Authors:     Maciej Lipinski (maciej.lipinski@cern.ch)
 *
 * Description: Nasty control of Endpoind configuration
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
#include <regs/endpoint-regs.h>

#include "rtu_ep_drv.h"
#include "wr_rtu.h"
#include "ep_pfilter.h"

#define m_dbg 1

#define ep_rd(reg, port) \
	 _fpga_readl(FPGA_BASE_EP0 + 0x400*port + offsetof(struct EP_WB, reg))

/*#define ep_rd(reg) \
	 _fpga_readl(FPGA_BASE_EP0 + offsetof(struct EP_WB, reg))	*/ 
	 
#define ep_wr(reg, port, val) \
	 _fpga_writel(FPGA_BASE_EP0 + 0x400*port + offsetof(struct EP_WB, reg), val)

extern int shw_fpga_mmap_init();
	 

int ep_init(int ep_enabled, int port_num)
{
   uint32_t val;
   int i;
   pck_inject_templ_t BPDU_templ = { "BPDU " /*info*/, 64, /*size*/
                                  /* pck content */
                                 { 0x01,0x80,0xC2,0x00,0x00,0x00, //0 - 5: dst addr
                                   0x00,0x00,0x00,0x00,0x00,0x00,  //6 -11: src addr (to be filled in ?)
                                   0x26,0x07,0x42,0x42,0x03,       //12-16: rest of the Eth Header
                                   0x00,0x00,                      //17-18: protocol ID
                                   0x00,                           //19   : protocol Version
                                   0x00,                           //20   : BPDU type =>: repleacable
                                   0x00,                           //21   : flags     =>: repleacable      
                                   0x00,0x00,0x00,0x00,0x00,0x00,  //22-27: padding
                                   0x00,0x00,0x00,0x00,0x00,0x00,  //28-33: padding
                                   0x00,0x00,0x00,0x00,0x00,0x00,  //34-39: padding
                                   0x00,0x00,0x00,0x00,0x00,0x00,  //40-45: padding
                                   0x00,0x00,0x00,0x00,0x00,0x00,  //46-51: padding
                                   0x00,0x00,0x00,0x00,0x00,0x00,  //52-57: padding
                                   0x00,0x00,0x00,0x00,0x00,0x00}};//58-63: padding
   pck_inject_templ_t PAUSE_templ ={ "PAUSE" /*info*/, 64 /*size*/,
                                     /* pck content */
				     {0x01,0x80,0xC2,0x00,0x00,0x01, //0 - 5: dst addr
                                    0x00,0x00,0x00,0x00,0x00,0x00, //6 -11: src addr (to be filled in ?)
                                    0x88,0x08,                     //12-13: Type Field = MAC control Frame
                                    0x00,0x01,                     //14-15: MAC Control Opcode = PAUSE
                                    0x00,0x00,                     //16-17: param: pause time: repleacable
                                    0x00,0x00,0x00,0x00,0x00,0x00, //18-23: padding
                                    0x00,0x00,0x00,0x00,0x00,0x00, //24-29: padding
                                    0x00,0x00,0x00,0x00,0x00,0x00, //30-35: padding
                                    0x00,0x00,0x00,0x00,0x00,0x00, //36-41: padding
                                    0x00,0x00,0x00,0x00,0x00,0x00, //42-47: padding
                                    0x00,0x00,0x00,0x00,0x00,0x00, //48-53: padding
                                    0x00,0x00,0x00,0x00,0x00,0x00, //54-59: padding
                                    0x00,0x00,0x00,0x00}};          //60-63: padding                                   
    /*
    pfilter_new();    
    pfilter_nop();                                          
    pfilter_cmp(0, 0xFFFF, 0xffff, MOV, 1); //setting bit 1 to HIGH if it 
    pfilter_cmp(1, 0xFFFF, 0xffff, AND, 1); // is righ kind of frame, i.e:
    pfilter_cmp(2, 0xFFFF, 0xffff, AND, 1); // 1. broadcast
    pfilter_nop();
    pfilter_nop();
    pfilter_nop();
    pfilter_nop();
    pfilter_nop();
    pfilter_cmp(8, 0xbabe, 0xffff, AND, 1); // 2. EtherType    
    pfilter_cmp(9, 0x0000, 0xffff, MOV, 2); // veryfing info in the frame for aggregation ID
    pfilter_cmp(9, 0x0001, 0xffff, MOV, 3); // veryfing info in the frame for aggregation ID   
    pfilter_cmp(9, 0x0002, 0xffff, MOV, 4); // veryfing info in the frame for aggregation ID   
    pfilter_cmp(9, 0x0003, 0xffff, MOV, 5); // veryfing info in the frame for aggregation ID   
    pfilter_logic2(24, 2, AND, 1); // recognizing class 0 in correct frame
    pfilter_logic2(25, 3, AND, 1); // recognizing class 0 in correct frame
    pfilter_logic2(26, 4, AND, 1); // recognizing class 0 in correct frame
    pfilter_logic2(27, 5, AND, 1); // recognizing class 0 in correct frame       
   */
    // test scenario 21 from main.sv
    pfilter_new();
    pfilter_nop();
    pfilter_cmp(0, 0x0180, 0xffff, MOV, 1);
    pfilter_cmp(1, 0xc200, 0xffff, AND, 1);
    pfilter_cmp(2, 0x0000, 0xffff, AND, 1);
    pfilter_nop();
    pfilter_nop();
    pfilter_nop();
    pfilter_cmp(6, 0xbabe, 0xffff, AND, 1);    
    pfilter_logic2(25, 1, MOV, 0);    
    
   TRACE(TRACE_INFO,"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
   TRACE(TRACE_INFO,"                       Endpoint init (EP)");
   ep_show_status(8);
   TRACE(TRACE_INFO," "); 
   TRACE(TRACE_INFO,"                       configure"); 
   TRACE(TRACE_INFO," ");  
   
   for(i=0;i<port_num;i++)
   {
     ep_set_vlan((uint32_t)i, 0x2/*qmode*/, 0 /*fix_prio*/, 0 /*prio_val*/, 0 /*pvid*/);
     ep_write_inj_pck_templ((uint32_t)i /*port*/, 0 /*slot*/, &PAUSE_templ,16);
     ep_write_inj_pck_templ((uint32_t)i /*port*/, 1 /*slot*/, &BPDU_templ,20);
     pfilter_load((uint32_t)i);
   }
   
   
   ep_show_status(8);
  return 0;
}

void ep_ctrl(int enable, int port)
{
   uint32_t val;
   val = ep_rd(ECR,port);
   if(enable)
     val = val | EP_ECR_RX_EN | EP_ECR_RX_EN;
   else
     val = val & ! (EP_ECR_RX_EN | EP_ECR_RX_EN);
   ep_wr(ECR,port,val);
   TRACE(TRACE_INFO,"EP [Port %2d]: enable=%1d",port,enable);
}

void ep_set_vlan(uint32_t port, int qmode, int fix_prio, int prio_val, int pvid)
{
   uint32_t val;
   val = EP_VCR0_PVID_W(pvid)         | 
         EP_VCR0_QMODE_W(qmode)       |
         EP_VCR0_PRIO_VAL_W(prio_val);
   if(fix_prio)
     val = val | EP_VCR0_FIX_PRIO;
   ep_wr(VCR0,port,val);
   TRACE(TRACE_INFO,"EP [Port %2d]: vlan setting:  qmode=%1d, fix_prio=%1d,"
                    "prio_val=%1d, pvid=%d",port, qmode, fix_prio, prio_val, pvid);
}

int ep_simple_test() 
{

  return 0;
}

void ep_show_status(int num_ports) 
{
  int i, rx_en,tx_en, feat_ptp, feat_vlan, feat_dpi, feat_dmtd, port_id, fixed_prio, qmode;
  uint32_t val;
  
  i=0;
  for(i=0;i<num_ports;i++)
  {
    val = ep_rd(ECR,i);
    if(val & EP_ECR_RX_EN)     rx_en = 1; else rx_en = 0;
    if(val & EP_ECR_TX_EN)     tx_en  =1; else tx_en = 0;
    if(val & EP_ECR_FEAT_VLAN) feat_vlan  =1; else feat_vlan = 0;
    if(val & EP_ECR_FEAT_DMTD) feat_dmtd  =1; else feat_dmtd = 0;
    if(val & EP_ECR_FEAT_PTP)  feat_ptp   =1; else feat_ptp = 0;
    if(val & EP_ECR_FEAT_DPI)  feat_dpi   =1; else feat_dpi = 0;
    port_id = EP_ECR_PORTID_R(val);
    
    val = ep_rd(VCR0,(uint32_t)i);
    qmode      = EP_VCR0_QMODE_R(val);
    fixed_prio = EP_VCR0_PRIO_VAL_R(val);
    
    TRACE(TRACE_INFO,"EP [Port %2d] status: rx_en=%1d, tx_en=%1d, id=%1d, qmode=%1d, fixed_prio=%1d"
                     "[featurs: vlan=%1d, ptp=%1d, dmtd=%1d, dpi=%1d]",
                     i, rx_en, tx_en, port_id, qmode, fixed_prio, feat_vlan, feat_ptp, 
                     feat_dmtd, feat_dpi);
    
  }
}
int ep_write_inj_pck_templ(uint32_t port, int slot, pck_inject_templ_t *pck_temp, 
                           int user_offset /* set -1 to have no replace offset*/)
{
  int i;
  uint32_t v = 0; 
  if(pck_temp->size & 1)
  {
    TRACE(TRACE_INFO,"EP [Port %2d] ep_write_inj_pck_templ(): data size must be even");
    return -1;
  }
  TRACE(TRACE_INFO,"EP [Port %2d] inj_pck: writing HW-injected pck template %s [size=%3d, slot=%2d,"
                   " user_offset=%2d]", port, pck_temp->info,pck_temp->size, slot, 
                   user_offset);
      
  for(i=0;i< pck_temp->size;i+=2)
  {
    v = ((pck_temp->data[i] << 8) | pck_temp->data[i+1]) & 0x0000FFFF;
    if(i == pck_temp->size - 2)
      v |= (1<<16);

    if(i == user_offset)
      v |= (1<<17);

    ep_vcr1_wr(port, 0,slot * 64 + i/2, v);
  }
}

void ep_vcr1_wr(uint32_t port,int is_vlan, int addr, uint32_t data)
{
  uint32_t val;
  val = EP_VCR1_OFFSET_W((is_vlan ? 0 : 0x200) + addr) |
        EP_VCR1_DATA_W(data);
  ep_wr(VCR1, port, val);
}

void ep_pfilter_status_N_ports(int port_num)
{
  int i;
  for(i=0;i<port_num;i++)
    pfilter_status((uint32_t)i);
  
}

void ep_pfilter_reload_code(int port)
{
  pfilter_new();
//   pfilter_nop();
//   pfilter_cmp(0, 0x0000, 0xFFFF, MOV, 1);
  
  pfilter_nop();
  pfilter_cmp(0, 0x0180, 0xffff, MOV, 1);
  pfilter_cmp(1, 0xc200, 0xffff, AND, 1);
  pfilter_cmp(2, 0x0000, 0xffff, AND, 1);
  pfilter_nop();
  pfilter_nop();
  pfilter_nop();
  pfilter_cmp(6, 0xbabe, 0xffff, AND, 1);    
  pfilter_logic2(25, 1, MOV, 0);  
  pfilter_load((uint32_t)port);//enable port sub_opt
}