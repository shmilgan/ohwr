/*
 * White Rabbit Switch RSTP
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizán (miguel.baizan@integrasys-sa.com)
 *
 *              This code is based on the code found at git://git.kernel.org/
 *              pub/scm/linux/kernel/git/shemminger/rstp.git, by
 *              Srinivas Aji <Aji_Srinivas@emc.com>
 *
 *
 * Description: Set raw sockets to send/receive only BPDUs. The socket is
 *              handled with the epoll facility.
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


#ifndef __WHITERABBIT_RSTP_FRAME_H
#define __WHITERABBIT_RSTP_FRAME_H


void frame_send(int ifindex, const unsigned char *data, int len);
int frame_socket_init(void);

#endif /* __WHITERABBIT_RSTP_FRAME_H */
