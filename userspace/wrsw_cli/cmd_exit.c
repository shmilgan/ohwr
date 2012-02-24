/*
 * White Rabbit Switch CLI (Command Line Interface)
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizan   (miguel.baizan@integrasys.es)
 *              Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: Implementation of the 'exit' command.
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

#include "cli_commands.h"
#include "cli_snmp.h"

enum exit_cmds {
    CMD_EXIT = 0,
    NUM_EXIT_CMDS
};

/**
 * \brief Command 'exit'.
 * @param cli CLI interpreter.
 * @param argc unused.
 * @param agv unused.
 */
void cli_cmd_exit(struct cli_shell *cli, int argc, char **argv)
{
    /* Close SNMP */
    cli_snmp_close();

    /* Free cli shell structure */
    if (cli->hostname)
        free(cli->hostname);
    if (cli->prompt)
        free(cli->prompt);
    if (cli)
        free(cli);

    printf("GOODBYE!\n\n");
    exit(0);
}

/* Define the 'exit' commands family */
struct cli_cmd cli_exit[NUM_EXIT_CMDS] = {
    /* exit */
    [CMD_EXIT] = {
        .parent     = NULL,
        .name       = "exit",
        .handler    = cli_cmd_exit,
        .desc       = "Exit from Command Line Interface",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    }
};

/**
 * \brief Init function for the 'exit' command.
 * @param cli CLI interpreter.
 */
void cmd_exit_init(struct cli_shell *cli)
{
    cli_insert_command(cli, cli_exit);
}
