/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2013, CERN.
 *
 * Version:     wrsw_rtud v1.1
 *
 * Authors:     Maciej Lipinski (maciej.lipinski@cern.ch)
 *
 * Description: Nasty control of Time-Aware Traffic Shaper (TATSU)
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
#include <regs/hwdu-regs.h>

#include "rtu_hwdu_drv.h"
#include "wr_rtu.h"

#define m_dbg 1

#define hwdu_rd(reg) \
	 _fpga_readl(FPGA_BASE_HWDU + offsetof(struct HWDU_WB, reg))
	 
#define hwdu_wr(reg, val) \
	 _fpga_writel(FPGA_BASE_HWDU + offsetof(struct HWDU_WB, reg), val)

extern int shw_fpga_mmap_init();
	 
char alloc_FSM[][30]={
                      {"S_IDLE"},
                      {"S_PCKSTART_SET_USECNT"},
                      {"S_PCKSTART_PAGE_REQ"},
                      {"S_PCKINTER_PAGE_REQ"},
                      {"S_PCKSTART_SET_AND_REQ"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"}
                     };

char trans_FSM[][30]={ 
                      {"S_IDLE"},
                      {"S_READY"},
                      {"S_WAIT_RTU_VALID"},
                      {"S_WAIT_SOF"},
                      {"S_SET_USECNT"},
                      {"S_WAIT_WITH_TRANSFER"},
                      {"S_TOO_LONG_TRANSFER"},
                      {"S_TRANSFER"},
                      {"S_TRANSFERED"},
                      {"S_DROP"},
                      {"fucked"},
                      {"fucked"}, 
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"}
                     };
char rcv_p_FSM[][30]={ 
                      {"S_IDLE"},
                      {"S_READY"},
                      {"S_PAUSE"},
                      {"S_RCV_DATA"},
                      {"S_DROP"},
                      {"S_WAIT_FORCE_FREE"},
                      {"S_INPUT_STUCK"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"} 
                     };
char linkl_FSM[][30]={ 
                      {"S_IDLE"},
                      {"S_READY_FOR_PGR_AND_DLAST"},
                      {"S_READY_FOR_DLAST_ONLY"},
                      {"S_WRITE"},
                      {"S_EOF_ON_WR"},
                      {"S_SOF_ON_WR"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},  
                     };

char prep_FSM[][30]  ={
                      {"S_RETRY_READY"}, //{"S_IDLE"}, // 0
                      {"S_NEWPCK_PAGE_READY"},           // 1
                      {"S_NEWPCK_PAGE_SET_IN_ADVANCE"},  // 2
                      {"S_NEWPCK_PAGE_USED"},            // 3
                      {"S_RETRY_PREPARE"},               // 4
                      {"S_IDLE"}, //{"S_RETRY_READY"},   // 5
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"}
                      };
char send_FSM[][30]  ={
                      {"S_IDLE"},             // 0
                      {"S_DATA"},             // 1
                      {"S_FLUSH_STALL"},      // 2
                      {"S_FINISH_CYCLE"},     // 3
                      {"S_EOF"},              // 4
                      {"S_RETRY"},            // 5
                      {"S_WAIT_FREE_PCK"},    // 6
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"},
                      {"fucked"}
                      };
int tatsu_init()
{
  return 0;
}

uint32_t hwdu_read(uint32_t addr, int dbg)
{
  uint32_t val=0;
  val = HWDU_CR_RD_EN | HWDU_CR_ADR_W(addr);
  hwdu_wr(CR, val);
  val = hwdu_rd(CR);
  if(val & HWDU_CR_RD_ERR) 
  { 
    TRACE(TRACE_INFO,"[HWDP] RD_ERR (read error, provided address is out of range) [Control Reg=0x%x]",val);
    return 0;
  }
  if(val & HWDU_CR_RD_ERR) 
  { 
    TRACE(TRACE_INFO,"[HWDP] RD_EN (read 1: reading in progress), wait [Control Reg=0x%x]",val);
    sleep(1);
    val = hwdu_rd(CR);
    if(val & HWDU_CR_RD_ERR) 
    {
      TRACE(TRACE_INFO,"[HWDP] RD_EN (read 1: reading in progress), too much waiting, exit [Control Reg=0x%x]",val);
      return 0;
    }
  }
  val = hwdu_rd(REG_VAL);
  if(dbg)
    {TRACE(TRACE_INFO,"[HWDP] raw value: 0x%x, addr: %d",val, addr);}
  return val;
  
}

int hwdu_gw_version_dump()
{
  uint32_t val;
  val = hwdu_read(HWDU_GW_VERSION_ADDR,0);
  
  TRACE(TRACE_INFO,"[HWDP] day: %x, month: %x, year: 20%2x, version of day: %x",
                       0xFF & (val >> 24),0xFF & (val >> 16),0xFF & (val >> 8),0xFF & val);
  return 0;
}

int hwdu_mpm_resoruces_dump()
{
  uint32_t val;
  val = hwdu_read(HWDU_MPM_RESOURCE_ADDR,0);
  
  TRACE(TRACE_INFO,"[HWDP] unknown   resources, page cnt :%d",0x3FF & val);
  TRACE(TRACE_INFO,"[HWDP] high prio resources, page cnt :%d",0x3FF & (val >> 10));
  TRACE(TRACE_INFO,"[HWDP] normal    resources, page cnt :%d",0x3FF & (val >> 20));
  return 0;
}

   
int hwdu_swc_in_b_pstates_dump(int port_num)
{
  uint32_t val, i;
   
  int read_num = 0; /// = port_num/2 + (0x1 & port_num);
  
  for(i=0;i<port_num+1;i++)
  {
    if(i%2 == 0)
    {
      val = hwdu_read(HWDU_SWC_IN_B_P0_ADDR + read_num,0);
      read_num++;
    }
    else
      val = val >> 16;
    
    if(i==port_num)
    {
      TRACE(TRACE_INFO,"[HWDP] input  block @ NIC     =>> alloc: %s, trans_FSM: %s, rcv_p_FSM: %s, linkl_FSM:"
                       " %s, rtu_rsp_valid=%d",
                        alloc_FSM[(0xF & (val >> 12))],
                        trans_FSM[(0xF & (val >> 8 ))],
                        rcv_p_FSM[(0xF & (val >> 4 ))],
                        linkl_FSM[(0x7 & (val >> 0 ))],
                                   0x1 & (val >> 15));      
    }
    else
    {
      TRACE(TRACE_INFO,"[HWDP] input  block @ port %2d =>> alloc: %s, trans_FSM: %s, rcv_p_FSM: %s, linkl_FSM:"
                       " %s, rtu_rsp_valid=%d", i,
                        alloc_FSM[(0xF & (val >> 12))],
                        trans_FSM[(0xF & (val >> 8 ))],
                        rcv_p_FSM[(0xF & (val >> 4 ))],
                        linkl_FSM[(0x7 & (val >> 0 ))],
                                   0x1 & (val >> 15));
    }
    
  }
  
  return 0;
}
int hwdu_swc_out_b_pstates_dump(int port_num)
{
  uint32_t val, i;
   
  int read_num = 0; /// = port_num/2 + (0x1 & port_num);
  uint32_t addr_base = HWDU_SWC_IN_B_P0_ADDR + port_num/2;
  
  for(i=0;i<port_num+1;i++)
  {
    if(i==0)
    {
      val = hwdu_read(addr_base + read_num,0);
      read_num++;
      val = val >>16;
    }
    else if(i > 1 && (i-2)%4 == 0) // because there is another port for NIC, we start
    {
      val = hwdu_read(addr_base + read_num,0);
      read_num++;
    }
    else
      val = val >> 8;
    
    if(i==port_num)
    {
      TRACE(TRACE_INFO,"[HWDP] output block @ NIC     =>> prepare: %s, send: %s",
                        prep_FSM[(0x7 & (val >> 0))],
                        send_FSM[(0x7 & (val >> 4))]);   
    }
    else
    {
      TRACE(TRACE_INFO,"[HWDP] output block @ port %2d =>> prepare: %s, send: %s",
                        i,
                        prep_FSM[(0x7 & (val >> 0))],
                        send_FSM[(0x7 & (val >> 4))]);
    }
    
  }
  
  return 0;
}

int hwdu_chps_get_id()
{
  uint32_t val;
  val = hwdu_rd(CHPS_ID);
  
  TRACE(TRACE_INFO,"[HWDP] chipscope input ID get: :%d",0xFF & val);
  return 0;
}
int hwdu_chps_set_id(int id)
{
  uint32_t val;
  hwdu_wr(CHPS_ID, id);
  
  TRACE(TRACE_INFO,"[HWDP] chipscope input ID set: :%d",id);
  return 0;
}
