/*
 * White Rabbit
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: Functions for inquiring about network interfaces (and not
 *              available at net/if.h)
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
#ifndef __WHITERABBIT_IF_INDEX_H
#define __WHITERABBIT_IF_INDEX_H

#include <stdint.h>

/* Custom IOCTLs */
#define PRIV_IOCGGETECR (SIOCDEVPRIVATE+5)
#define WRN_ECR_GET_PORTID 4

/* For custom ioctl operations on endpoint registers */
struct wrn_register_req {
    int         cmd;
    uint32_t    val;
};

int wr_nametoport(const char *ifname);

#endif /*__WHITERABBIT_IF_INDEX_H*/
