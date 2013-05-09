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
	 

int tatsu_init()
{
  return 0;
}

uint32_t hwdu_read(uint32_t addr)
{
  uint32_t val;
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
  TRACE(TRACE_INFO,"[HWDP] raw value: 0x%x, addr: %d",val, addr);
  return val;
  
}

int hwdu_gw_version_dump()
{
  uint32_t val;
  val = hwdu_read(HWDU_GW_VERSION_ADDR);
  
  TRACE(TRACE_INFO,"[HWDP] day: %x, month: %x, year: 20%2x, version of day: %x",
                       0xFF & (val >> 24),0xFF & (val >> 16),0xFF & (val >> 8),0xFF & val);
  return 0;
}

int hwdu_mpm_resoruces_dump()
{
  uint32_t val;
  val = hwdu_read(HWDU_MPM_RESOURCE_ADDR);
  
  TRACE(TRACE_INFO,"[HWDP] unknown   resources, page cnt :%d",0x3FF & val);
  TRACE(TRACE_INFO,"[HWDP] high prio resources, page cnt :%d",0x3FF & (val >> 10));
  TRACE(TRACE_INFO,"[HWDP] normal    resources, page cnt :%d",0x3FF & (val >> 20));
  return 0;
}