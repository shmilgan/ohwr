/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2010, CERN.
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


#ifndef __WHITERABBIT_RTU_TRU_DRV_H
#define __WHITERABBIT_RTU_TRU_DRV_H

#include "rtu.h"

int tru_simple_test() ;
void tru_write_tab_entry(int valid,      int fid,          int subfid, 
                         int patrn_mask, int patrn_match,  int patrn_mode,
                         int ports_mask, int ports_egress, int ports_ingress);
void tru_transition_config(int mode,      int rx_id,       int prio, int time_diff,
                           int port_a_id, int port_b_id);
void tru_transition_enable();
void tru_transition_disable();
void tru_transition_clear();
void tru_pattern_config(uint32_t replacement,  uint32_t addition);
void tru_enable();
void tru_disable();
void tru_swap_bank();
void tru_rx_frame_reset(uint32_t reset_rx);
void tru_rt_reconf_config(int tx_frame_id, int rx_frame_id, int mode);
void tru_rt_reconf_enable();
void tru_rt_reconf_disable();
void tru_rt_reconf_reset();
void tru_read_status(uint32_t *bank, uint32_t *ports_up, uint32_t *ports_stb_up, int display);
void tru_set_life(char *optarg);
void tru_show_status(int port_number);
int tru_port_state_up(int port_id);
uint32_t tru_port_stable_up_mask(void);
void tru_set_port_roles(int active_port, int backup_port);
void tru_lacp_config(uint32_t df_hp_id, uint32_t df_br_id, uint32_t df_un_id);
#endif /*__WHITERABBIT_RTU_DRV_H*/
