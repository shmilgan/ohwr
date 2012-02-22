/*
 * White Rabbit
 *
 * Authors:     Miguel Baizan   (miguel.baizan@integrasys.es)
 *
 * Description: Wrapper functions for HAL IPC methods.
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
 #include <wr_ipc.h>
#include "wrsw_hal_ipc.h"

int hal_get_port_info(hexp_port_state_t *port_info, const char *port_name)
{
    int ret = 0;
    wripc_handle_t hal_ipc;

    /* Connect to HAL */
    hal_ipc = wripc_connect("wrsw_hal");
    if (hal_ipc < 0)
        return -1;

    /* Get port state */
    if (wripc_call(hal_ipc, "halexp_get_port_state", port_info, 1,
                   A_STRING(port_name)) < 0)
        ret = -1;

    wripc_close(hal_ipc);
    return ret;
}

int hal_get_port_list(hexp_port_list_t *port_list)
{
    int ret = 0;
    wripc_handle_t hal_ipc;

    /* Connect to HAL */
    hal_ipc = wripc_connect("wrsw_hal");
    if (hal_ipc < 0)
        return -1;

    /* Get ports list */
    if (wripc_call(hal_ipc, "halexp_query_ports", port_list, 0) < 0)
        ret = -1;

    wripc_close(hal_ipc);
    return ret;
}
