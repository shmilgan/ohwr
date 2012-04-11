/*
 * White Rabbit Switch CLI (Command Line Interface)
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizan   (miguel.baizan@integrasys.es)
 *              Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: Implementation of the 'hostname' command.
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

#include "cli_commands.h"

enum hostname_cmds {
    CMD_HOSTNAME = 0,
    NUM_HOSTNAME_CMDS
};

/**
 * \brief Command 'hostname'.
 * This command shows the name of the host. If an argument is provided, this
 * command changes the name of the host to the new name passed as argument.
 * @param cli CLI interpreter.
 * @param valc number of arguments (only 0 or 1 are allowed).
 * @param valv if used, this is meant to be the new name for the host.
 */
void cli_cmd_hostname(struct cli_shell *cli, int valc, char **valv)
{
    int i = 0;

    if (valc) {
        for (i = 0; i < MAX_HOSTNAME_LEN; i++)
            if (valv[0][i] == '\0')
                break;
        if (i == 32)
            valv[0][i] = '\0';
        free(cli->hostname);
        cli->hostname = strdup(valv[0]);
        cli_build_prompt(cli);
        printf("\tThe new name for the host is '%s'\n", cli->hostname);
    } else {
        printf("\tHostname is '%s'\n", cli->hostname);
    }

    return;
}

/* Define the 'hostname' commands family */
struct cli_cmd cli_hostname[NUM_HOSTNAME_CMDS] = {
    /* hostname <hostname> */
    [CMD_HOSTNAME] = {
        .parent     = NULL,
        .name       = "hostname",
        .handler    = cli_cmd_hostname,
        .desc       = "Displays or sets (if argument is present) the host name",
        .opt        = CMD_ARG_OPTIONAL,
        .opt_desc   = "<hostname> New hostname"
    }
};

/**
 * \brief Init function for the 'hostname' command.
 * @param cli CLI interpreter.
 */
void cmd_hostname_init(struct cli_shell *cli)
{
    cli_insert_command(cli, cli_hostname);
}
