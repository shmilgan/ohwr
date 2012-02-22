/*
 * White Rabbit
 *
 * Authors:    Miguel Baizan   (miguel.baizan@integrasys.es)
 *
 * Description: Wrapper function pototypes for HAL IPC methods.
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
#ifndef __WHITERABBIT_HAL_IPC_H
#define __WHITERABBIT_HAL_IPC_H

#include <hal_exports.h>

int hal_get_port_list(hexp_port_list_t *port_list);
int hal_get_port_info(hexp_port_state_t *port_info, const char *port_name);

#endif /* __WHITERABBIT_HAL_IPC_H */
