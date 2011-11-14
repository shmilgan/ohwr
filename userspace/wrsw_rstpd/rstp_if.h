/*
 * White Rabbit Switch RSTP
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizán (miguel.baizan@integrasys-sa.com)
 *
 * Description: Functions that use exported functions of other daemons and HW,
 *              to get/set some values related with the RSTP operation.
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

#ifndef __WHITERABBIT_RSTP_IF_H
#define __WHITERABBIT_RSTP_IF_H

#include "rstp_data.h"


/* Custom IOCTLs */
#define PRIV_IOCGGETECR (SIOCDEVPRIVATE+5)
#define WRN_ECR_GET_PORTID 4

/* For custom ioctl operations on endpoint registers */
struct wrn_register_req {
    int         cmd;
    uint32_t    val;
};


int rstp_if_init_port_data(struct bridge_data *br);
int rstp_if_get_bridge_addr(struct bridge_data *br);


#endif /* __WHITERABBIT_RSTP_IF_H */
