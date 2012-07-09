/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_rtud v1.0
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *              Miguel Baizan   (miguel.baizan@integrasys.es)
 *              Maciej Lipinski (maciej.lipinski@cern.ch)
 *
 * Description: RTU driver module in user space.
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


#ifndef __WHITERABBIT_RTU_DRV_H
#define __WHITERABBIT_RTU_DRV_H

#include "rtu.h"

#define RTU_MFIFO_R0_DATA_SEL   0x00000000
#define RTU_MFIFO_R1_ADDR_MASK  0x0007FFFF

#define RTU_DEVNAME             "/dev/wr_rtu"

int rtu_init(void)
        __attribute__((warn_unused_result));
void rtu_exit(void);

// UFIFO

int rtu_ufifo_is_empty(void);
int rtu_read_learning_queue_cnt(void);
int rtu_read_learning_queue(struct rtu_request *req);

// MFIFO

int rtu_read_mfifo_cnt(void);
int rtu_mfifo_is_full(void);
int rtu_mfifo_is_empty(void);
void rtu_clean_mfifo(void);
void rtu_write_htab_entry(uint16_t zbt_addr, struct filtering_entry *ent);
void rtu_clean_htab_entry(uint16_t zbt_addr);
void rtu_clean_htab(void);

// HCAM

void rtu_read_hcam_entry(uint16_t cam_addr, struct filtering_entry *ent);
void rtu_write_hcam_entry(uint16_t cam_addr, struct filtering_entry *ent);
void rtu_clean_hcam_entry(uint8_t cam_addr);
void rtu_clean_hcam(void);

// AGING - HTAB

uint32_t rtu_read_agr_htab(uint32_t addr);
void rtu_clean_agr_htab(void);

// AGING - HCAM

uint32_t rtu_read_agr_hcam(void);
void rtu_clean_agr_hcam(void);

// VLAN TABLE

void rtu_write_vlan_entry(int vid, struct vlan_table_entry *ent);
void rtu_clean_vlan_entry(int vid);
void rtu_clean_vlan(void);

// GLOBAL CONTROL REGISTER

void rtu_enable(void);
void rtu_disable(void);
void rtu_write_hash_poly(uint16_t hash_poly);
uint16_t rtu_read_hash_poly(void);
void rtu_set_active_htab_bank(uint8_t bank);
void rtu_set_active_hcam_bank(uint8_t bank);
void rtu_set_active_bank(uint8_t bank);

// PORT SETTINGS

int rtu_set_fixed_prio_on_port(int port, uint8_t prio)
        __attribute__((warn_unused_result));
int rtu_unset_fixed_prio_on_port(int port)
        __attribute__((warn_unused_result));
int rtu_learn_enable_on_port(int port, int flag)
        __attribute__((warn_unused_result));
int rtu_pass_bpdu_on_port(int port, int flag)
        __attribute__((warn_unused_result));
int rtu_pass_all_on_port(int port, int pass_all)
        __attribute__((warn_unused_result));
int rtu_set_unrecognised_behaviour_on_port(int port, int flag)
        __attribute__((warn_unused_result));

// IRQs

void rtu_enable_irq(void);
void rtu_disable_irq(void);
void rtu_clear_irq(void);


#endif /*__WHITERABBIT_RTU_DRV_H*/
