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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <getopt.h>

#include "rstp_frame.h"
#include "rstp_epoll_loop.h"
#include "rstp_data.h"


/* Helper function to show the correct usage of the daemon */
static void usage(char *name)
{
    fprintf(stderr,
        "Usage: %s [-dh] [-f FILENAME] \n"
        "\t-d   daemonize\n"
        "\t-f   Sets the file to store the log messages\n"
        "\t-h   help\n",
        name);
    exit(1);
}

/* Helper function to daemonize the process */
static void daemonize(void)
{
    pid_t pid, sid;

    /* Already a daemon. Only allowed one daemon active at the same time */
    if ( getppid() == 1 ) return;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        TRACE(TRACE_FATAL, "Unable to fork daemon: error %d: %s",
              errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then we can exit the parent process. */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* At this point we are executing as the child process */

    /* Change the file mode mask */
    umask(0);

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        TRACE(TRACE_FATAL, "Unable to create a new session: error %d: %s",
              errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Change the current working directory.  This prevents the current
       directory from being locked; hence not being able to remove it. */
    if ((chdir("/")) < 0) {
        TRACE(TRACE_FATAL, "Unable to change directory to \"/\": error %d: %s",
              errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Close standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

}

/* Main function*/
int main(int argc, char *argv[])
{
    int op, err;
    char *optstring;
    char *log_file = DEFAULT_LOG_FILE;
    int run_as_daemon = 0;


    if (argc > 1) {
        /* Parse daemon options */
        optstring = "dhf:";
        while ((op = getopt(argc, argv, optstring)) != -1) {
            switch(op) {
            case 'd':
                run_as_daemon = 1;
                break;
            case 'f':
                log_file = optarg;
                break;
            case 'h':
                usage(argv[0]);
            default:
                usage(argv[0]);
            }
        }
    }

    /* Set log type */
    if (run_as_daemon) {
        errno = 0;
        trace_log_file(log_file);
        if (errno != 0) {
            fprintf(stderr, "Unable to open the log file: error %d: %s\n",
                    errno, strerror(errno));
            return errno;
        }
    } else {
        trace_log_stderr();
    }

    /* Create an event notification facility */
    if ((err = epoll_init()) < 0)
        return err;

    /* Create socket to get RSTP BPDUs from the kernel */
    if ((err = frame_socket_init()) < 0) {
        epoll_clear();
        return err;
    }

    /* Initialise data (rstp parameters, port states, timers, etc) */
    if ((err = init_data()) < 0) {
        epoll_clear();
        return err;
    }

    /* Daemonize */
    if(run_as_daemon)
        daemonize();

    /* Main loop */
    err = epoll_main_loop();

    epoll_clear();
    return err;
}
