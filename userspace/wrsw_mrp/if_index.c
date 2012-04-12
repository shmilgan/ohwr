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
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "if_index.h"

/* Get port number for the given interface name */
int wr_nametoport(const char *ifname)
{
    int fd, ret;
    struct ifreq ifr;
    struct wrn_register_req req;

    fd = socket(PF_PACKET, SOCK_RAW, 0);
    if (fd < 0)
        return -1;

    strncpy (ifr.ifr_name, ifname, IF_NAMESIZE);

    ifr.ifr_data = (caddr_t) &req;
    req.cmd = WRN_ECR_GET_PORTID;
    ret = (ioctl(fd, PRIV_IOCGGETECR, &ifr) < 0) ? -1:req.val;
    close(fd);
    return ret;
}
