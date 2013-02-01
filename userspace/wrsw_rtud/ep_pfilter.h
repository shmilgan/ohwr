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


#ifndef __WHITERABBIT_EP_PFILTER_H
#define __WHITERABBIT_EP_PFILTER_H

#include "rtu.h"

   typedef enum 
    {
     AND = 0,
     NAND = 4,
     OR = 1,
     NOR = 5,
     XOR = 2,
     XNOR = 6,
     MOV = 3,
     NOT = 7
     } pfilter_op_t;

void pfilter_new();
void pfilter_cmp(int offset, int value, int mask, pfilter_op_t op, int rd);
void pfilter_btst(int offset, int bit_index, pfilter_op_t op, int rd);
void pfilter_nop() ;
void pfilter_logic2(int rd, int ra, pfilter_op_t op, int rb);
void pfilter_logic3(int rd, int ra, pfilter_op_t op, int rb, pfilter_op_t op2, int rc);
void pfilter_load(uint32_t port);
int pfilter_status(uint32_t port);
void pfilter_disable(uint32_t port);
#endif /*__WHITERABBIT_EP_PFILTER_H*/
