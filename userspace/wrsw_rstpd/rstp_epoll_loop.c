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

#include <sys/epoll.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "rstp_epoll_loop.h"
#include "rstp_data.h"


/* Globals */
static int epoll_fd = -1;

int epoll_init(void)
{
    /* Level-triggered by default */
    int ret = epoll_create(EPOLL_MAX_NUM_DESCRIPTORS);
    if (ret < 0) {
        TRACE(TRACE_FATAL, "epoll_create failed: %d", ret);
        return ret;
    }

    epoll_fd = ret;
    return 0;
}

int epoll_add(struct epoll_event_handler *h)
{
    struct epoll_event ev = {
        .events = EPOLLIN, /* Input file descriptor ready event */
        .data.ptr = h,
    };

    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, h->fd, &ev);
    if (ret < 0)
        TRACE(TRACE_FATAL, "add_epoll failed: %d", ret);

    return ret;
}

int epoll_remove(struct epoll_event_handler *h)
{
    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, h->fd, NULL);
    if (ret < 0)
        TRACE(TRACE_FATAL, "remove_epoll failed: %d", ret);

    return ret;
}

void epoll_clear(void)
{
    if (epoll_fd >= 0)
        close(epoll_fd);
}

/* The one second steps are managed by this function */
int epoll_main_loop(void)
{
    int num_fd, i;
    int timeout;
    struct timeval nexttimeout, tv;
    struct epoll_event ev[EPOLL_MAX_NUM_EVENTS]; /* Array of available events */

    gettimeofday(&nexttimeout, NULL);
    nexttimeout.tv_sec++;

    while (1) {
        gettimeofday(&tv, NULL);
        timeout = time_diff(&nexttimeout, &tv);
        if (timeout < 0) { /* tick */
            nexttimeout.tv_sec++;
            //recompute_stmchs(); /* TODO Tick has been generated. Recompute STMs */
            TRACEV(TRACE_INFO, "A tick has been raised");
            timeout = time_diff(&nexttimeout, &tv);
        } /* TODO what if more than 1 sec. has expired (timeout < -1000)? */

        /* Wait for events or for timeout to expire */
        num_fd = epoll_wait(epoll_fd, ev, EPOLL_MAX_NUM_EVENTS, timeout);
        if (num_fd < 0 && errno != EINTR) {
            TRACE(TRACE_FATAL, "epoll_wait failed: %d", num_fd);
            return -1;
        }

        /* Handle triggered events */
        for (i = 0; i < num_fd; i++) {
            struct epoll_event_handler *p = ev[i].data.ptr;
            if (p && p->handler)
                p->handler(ev[i].events, p);
        }
    }

    return -1;
}
