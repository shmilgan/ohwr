/*
 * White Rabbit Switch RSTP
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizán (miguel.baizan@integrasys-sa.com)
 *
 * Description: RSTP daemon.
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


#include <getopt.h>

#include <hw/trace.h>

#include "rstp_frame.h"
#include "rstp_epoll_loop.h"
#include "utils.h"
#include "rstp_data.h"


extern int epoll_fd;

int main(int argc, char *argv[])
{
    int op, err;
    char *s, *name, *optstring;
    int run_as_daemon = 0;

    trace_log_stderr();

    if (argc > 1) {
        /* Strip out path from argv[0] if exists, and extract command name */
        for (name = s = argv[0]; s[0]; s++) {
            if (s[0] == '/' && s[1]) {
                name = &s[1];
            }
        }
        /* Parse daemon options */
        optstring = "dh:";
        while ((op = getopt(argc, argv, optstring)) != -1) {
            switch(op) {
            case 'd':
                run_as_daemon = 1;
                break;
            case 'h':
                usage(name);
            default:
                usage(name);
            }
        }
    }

    /* Daemonize */
    if(run_as_daemon)
        daemonize();

    /* Create an event notification facility */
    if ((err = epoll_init()) < 0)
        return err;

    /* Create socket to get RSTP BPDUs from the kernel */
    if ((err = frame_socket_init()) < 0) {
        clear_epoll();
        return err;
    }

    /* Initialise data (rstp parameters, port states, timers, etc) */
    if ((err = init_data()) < 0) {
        clear_epoll();
        return err;
    }

    /* Main loop */
    if ((err = epoll_main_loop()) < 0) {
        clear_epoll();
        return err;
    }

    return 0;
}
