/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2014, CERN.
 *
 * Version:     wrsw_rtud v1.1
 *
 * Authors:     Maciej Lipinski (maciej.lipinski@cern.ch)
 *
 * Description: RTU extension driver module in user space.
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

#ifndef __WHITERABBIT_RTU_EXT_DRV_H
#define __WHITERABBIT_RTU_EXT_DRV_H

#include "rtu.h"

int rtux_init(void);
void rtux_add_ff_mac_single(int mac_id, int valid, uint8_t mac[ETH_ALEN]);
void rtux_add_ff_mac_range(int mac_id, int valid, uint8_t mac_lower[ETH_ALEN],
			   uint8_t mac_upper[ETH_ALEN]);
void rtux_set_port_mirror(uint32_t mirror_src_mask, uint32_t mirror_dst_mask,
			  int rx, int tx);
void rtux_set_hp_prio_mask(uint8_t hp_prio_mask);
uint8_t rtux_get_hp_prio_mask();

int rtux_get_cpu_port();
void rtux_set_feature_ctrl(int mr, int mac_ptp, int mac_ll, int mac_single,
			   int mac_range, int mac_br, int at_fm);
void rtux_set_fw_to_CPU(int hp, int unrec);
void rtux_disp_ctrl(void);

#endif /*__WHITERABBIT_RTU_DRV_H*/
