/*
 * White Rabbit Switch CLI (Command Line Interface)
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizan   (miguel.baizan@integrasys.es)
 *
 * Description: Basic CLI implementation.
 *              It has been built taking into account the CISCO's CLI command
 *              syntax (given that it has become an industry
 *              standard for the management of network devices). However, in WR
 *              we don't envisage to use the CLI as the front-end for managing
 *              the operating system, so some simplifications have been
 *              introduced.
 *
 *              Part of the code written here is based on ideas taken from the
 *              LGPL libcli package (https://github.com/dparrish/libcli.git)
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


#ifndef __WHITERABBIT_CLI_H
#define __WHITERABBIT_CLI_H

#define VERSION "1.0"

/* Prompt */
#define EXEC_PROMPT  "> "

/* Default hostname */
#define DEFAULT_HOSTNAME    "WR-Switch"

/* Define a maximum number of commands and options allowed per line */
#define MAX_CMDS_IN_LINE    12

/* Define a maximum length for the commands inserted by the user (meassured in
   characters) */
#define MAX_CMD_LENGTH      2048


/* Define the types of argument expected by the command */
enum type_of_arg {
    CMD_NO_ARG = 0,
    CMD_ARG_MANDATORY,
    CMD_ARG_OPTIONAL
};

/* Main data structures */
struct cli_cmd;
struct cli_shell;

/* Command information */
struct cli_cmd {
    char    *name;          /* Absolute name of the command */
    char    *desc;          /* Description of the command */
    int     opt;            /* Flag to know if this command accepts arguments */
    char    *opt_desc;      /* Description for the arguments */

    void (*handler) (struct cli_shell *, int, char **); /* Handler for the
                                                          command */

    struct cli_cmd *child;      /* Next possible commands in the line */
    struct cli_cmd *parent;     /* Previous command in the line */
    struct cli_cmd *next;       /* Commands with the same parent. Useful for
                                   autocompletion functions */
};


/* CLI interpreter information */
struct cli_shell {
    char    *hostname;          /* Name of the host */
    char    *prompt;            /* Prompt for the cli shell */

    struct cli_cmd *root_cmd;    /* Reference to the root of the command tree */
};


/* Functions */
void cli_error(int error_code);
struct cli_shell *cli_init(void);
void cli_build_prompt(struct cli_shell *cli);
struct cli_cmd *cli_find_command(struct cli_shell *cli,
                                 struct cli_cmd *top_cmd,
                                 char *cmd);
void cli_run_command(struct cli_shell *cli, char *string);
void cli_main_loop(struct cli_shell *cli);


#endif /*__WHITERABBIT_CLI_H*/
