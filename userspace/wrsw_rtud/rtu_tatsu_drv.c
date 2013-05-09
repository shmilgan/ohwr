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
#include <regs/tatsu-regs.h>

#include "rtu_tatsu_drv.h"
#include "wr_rtu.h"

#define m_dbg 1

#define tatsu_rd(reg) \
	 _fpga_readl(FPGA_BASE_TATSU + offsetof(struct TATSU_WB, reg))
	 
#define tatsu_wr(reg, val) \
	 _fpga_writel(FPGA_BASE_TATSU + offsetof(struct TATSU_WB, reg), val)

extern int shw_fpga_mmap_init();
	 

int tatsu_drop_nonHP_enable()
{
  uint32_t val;
  val = tatsu_rd(TCR);
  val = val | TATSU_TCR_DROP_ENA;
  tatsu_wr(TCR, val);
  TRACE(TRACE_INFO,"TATSU: drop_nonHP_enable (when HP comes) - enabled ");
  return 0;
  
}

int tatsu_drop_nonHP_disable()
{
  uint32_t val;
  val = tatsu_rd(TCR);
  val = val | TATSU_TCR_DROP_ENA;
  tatsu_wr(TCR, val);
  TRACE(TRACE_INFO,"TATSU: drop_nonHP_enable (when HP comes) - enabled ");
  return 0;
  
}
