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


#ifndef __WHITERABBIT_RTU_EP_DRV_H
#define __WHITERABBIT_RTU_EP_DRV_H

#include "rtu.h"
#include "ep_pfilter.h"

typedef struct {
  char info[30];
  int size;
  unsigned char data[128];
} pck_inject_templ_t;



int ep_init(int ep_enabled, int port_num);
int ep_simple_test() ;
void ep_show_status(int num_ports) ;
void ep_vcr1_wr(uint32_t port,int is_vlan, int addr, uint32_t data);
int ep_write_inj_pck_templ(uint32_t port, int slot, pck_inject_templ_t *pck_temp, int user_offset);
int ep_write_inj_gen_templ(uint32_t port, pck_inject_templ_t *header_tmpl, int frame_size, int slot);
void ep_pfilter_status_N_ports(int port_num);
void ep_pfilter_reload_code(int port);
void ep_show_pause_config(uint32_t port);
void ep_pause_config_ena(uint32_t port, int txpause_802_3,int rxpause_802_3, int txpause_802_1q, int rxpause_802_1q);
void ep_pause_config_dis(uint32_t port);
void ep_snake_config(int option);
void ep_class_prio_map(uint32_t port, int prio_map[]);
void ep_pfilter_lacp_test_code(int port);
void ep_strange_config(int opt);
void ep_inj_gen_ctr_config(uint32_t port, int interframe_gap, int sel_id /*slot*/);
void ep_inj_gen_ctr_enable(uint32_t port);
void ep_gen_pck_start(uint32_t port);
void ep_gen_pck_stop(uint32_t port);
void ep_inj_gen_ctr_config_N_ports(int N_port, int ifg, int size, int sel_id /*slot*/);
void ep_gen_pck_config_show(uint32_t port);
#endif /*__WHITERABBIT_RTU_EP_DRV_H*/
