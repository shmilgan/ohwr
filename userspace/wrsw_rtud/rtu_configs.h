/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2013, CERN.
 *
 * Version:     wrsw_rtud v2.0-dev
 *
 * Authors:     Maciej Lipinski (maciej.lipinski@cern.ch)
 *              
 *
 * Description: RTU initial configuration
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

#ifndef __WHITERABBIT_RTU_CONFIGS_H
#define __WHITERABBIT_RTU_CONFIGS_H

#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>


#include <trace.h>
#include <switch_hw.h>
#include <hal_client.h>

#include "rtu.h"
#include "rtu_fd.h"
#include "rtu_drv.h"
#include "rtu_ep_drv.h"
#include "rtu_ext_drv.h"
#include "rtu_tru_drv.h"
#include "rtu_hwdu_drv.h"
#include "rtu_hash.h"
#include "utils.h"


int config_startup(int opt, int sub_opt, int port_num);


#endif /*__WHITERABBIT_RTU_CONFIGS_H*/
