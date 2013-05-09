/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2013, CERN.
 *
 * Version:     wrsw_rtud v1.1
 *
 * Authors:     Maciej Lipinski (maciej.lipinski@cern.ch)
 *
 * Description: 
 *
 * Fixes:
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


#ifndef __WHITERABBIT_RTU_HWDU_DRV_H
#define __WHITERABBIT_RTU_HWDU_DRV_H

#include "rtu.h"

#define HWDU_GW_VERSION_ADDR   0
#define HWDU_MPM_RESOURCE_ADDR 1

int hwdu_mpm_resoruces_dump();
int hwdu_gw_version_dump();

#endif /*__WHITERABBIT_RTU_TATSU_DRV_H*/
