/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_rtud v1.0
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *              Miguel Baizan   (miguel.baizan@integrasys.es)
 *
 * Description: Handles HTAB/HCAM HW writes.
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

#ifndef __WHITERABBIT_RTU_HW_H
#define __WHITERABBIT_RTU_HW_H

#include "rtu.h"

int rtu_hw_write_htab_entry(uint16_t addr, struct filtering_entry *e);
int rtu_hw_write_hcam_entry(uint16_t addr, struct filtering_entry *e);
int rtu_hw_clean_htab_entry(uint16_t addr);
int rtu_hw_clean_hcam_entry(uint16_t addr);
void rtu_hw_clean_fdb(void);
void rtu_hw_commit(void);
void rtu_hw_rollback(void);

#endif /*__WHITERABBIT_RTU_HW_H*/

