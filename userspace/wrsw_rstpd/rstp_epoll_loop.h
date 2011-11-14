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
 * Description: epoll event notification facility. It is on charge of gathering
 *              the events triggered for the different sockets maintained by the
 *              daemon (i.e. currently sockets for IPCs and sockets to
 *              rcv/snd BPDUs)
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


#ifndef __WHITERABBIT_RSTP_EPOLL_LOOP_H
#define __WHITERABBIT_RSTP_EPOLL_LOOP_H

#include <sys/epoll.h>


/* Maximum number of descriptors that can be registered in the epoll set */
#define EPOLL_MAX_NUM_DESCRIPTORS   8

/* Maximum number of events that the epoll will serve */
#define EPOLL_MAX_NUM_EVENTS    4

/* Structure created to keep track of the different file descriptors
   and its handlers for the epoll set */
struct epoll_event_handler {
    int fd;
    void (*handler) (uint32_t events, struct epoll_event_handler * p);
};

/* Functions */
int epoll_init(void);
void epoll_clear(void);
int epoll_add(struct epoll_event_handler *h);
int epoll_remove(struct epoll_event_handler *h);
int epoll_main_loop(void);

static inline int time_diff(struct timeval *second, struct timeval *first)
{
    return (second->tv_sec - first->tv_sec) * 1000 +
           (second->tv_usec - first->tv_usec) / 1000;
} /* Time difference meassured in milliseconds */

#endif /* __WHITERABBIT_RSTP_EPOLL_LOOP_H */
