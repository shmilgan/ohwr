/*
 * White Rabbit Switch CLI (Command Line Interface)
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizan   (miguel.baizan@integrasys.es)
 *              Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: Building of the commands tree.
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
#include <errno.h>

#include "cli_commands.h"

/*
 * NOTE:
 * The commands tree is built as a linked list of subcommands, starting with a
 * root command, whose reference is stored in the cli_shell structure
 * in order to keep a track of the whole list. Each subcommand may have a
 * parent, a children and a reference to the next command with shared parent.
 * Each parent will only have a child (to access other childs we use 'next').
 * Depending on the previous parent commands, each subcommand has a different
 * handler. This is a graphical example of a commands tree:
 *
 * +--------------+
 * | root Command |
 * +--------------+
 *        |
 *        V
 * +--------------+
 * |  Command A   |
*  |   next B     |
 * +--------------+
 *        |
 *        V
 * +--------------+         +---------------+
 * | Command B    |<------->|  Command D    |        +--------------+
*  |  next C      |         |   parent B    |        |  Command F   |
*  |  child D     |         |   child F     |<------>|   parent D   |
*  |              |<----    |   next E      |        |              |
 * +--------------+    |    +---------------+        +--------------+
 *        |            |            |
 *        V            |            V
 * +--------------+    |    +---------------+
 * | Command C    |    |____|  Command E    |
 * +--------------+         |   parent B    |
 *                          +---------------+
 *
 *
 */

/* Define the 'no' and 'show' commands globally, so other subcommands in
   different command families can share these commands as parents */
struct cli_cmd cli_no = {
    .parent     = NULL,
    .name       = "no",
    .handler    = NULL,
    .desc       = "Use 'no' to delete/disable some configured parameters",
    .opt        = CMD_NO_ARG,
    .opt_desc   = NULL
};

struct cli_cmd cli_show = {
    .parent     = NULL,
    .name       = "show",
    .handler    = NULL,
    .desc       = "Shows the device configurations",
    .opt        = CMD_NO_ARG,
    .opt_desc   = NULL
};

/**
 * \brief Inserts a command in the commands tree.
 * It inserts a command in the command tree, by updating the child and next
 * pointers of the commands.
 * @param cli CLI interpreter.
 * @param parent pointer to the command to register.
 */
void cli_insert_command(struct cli_shell *cli, struct cli_cmd *cmd)
{
    struct cli_cmd *c;

    if (cmd->parent) {
        if (!cmd->parent->child) { /* parent has no child yet */
            cmd->parent->child = cmd;
        } else { /* traverse the list to add the command at the end */
            c = cmd->parent->child;
            while (c) {
                if (c->next) {
                    c = c->next;
                } else {
                    c->next = cmd;
                    break;
                }
            }
        }
    } else { /* the command has no parent */
        c = cli->root_cmd;
        if(!c) { /* this is the first command in the tree */
            cli->root_cmd = cmd;
        } else {
            while (c) { /* traverse the list to add the command at the end */
                if (c->next) {
                    c = c->next;
                } else {
                    c->next = cmd;
                    break;
                }
            }
        }
    }
    return;
}

/**
 * \brief Builds the commands tree.
 * The commands tree is built as a linked list of commands, starting with a
 * root command, whose reference is stored in the cli_shell structure
 * in order to keep a track of the whole list. Each command may have a
 * parent, a children and a reference to the next command with shared parent.
 * Each parent will only have a child (to access other childs we use 'next').
 * @param cli CLI interpreter.
 */
void cli_build_commands_tree(struct cli_shell *cli)
{
    /* Insert the 'no' and 'show' commands in the commands tree */
    cli_insert_command(cli, &cli_no);
    cli_insert_command(cli, &cli_show);

    /* 'exit' command */
    cmd_exit_init(cli);

    /* 'hostname' command */
    cmd_hostname_init(cli);

    /* 'interface' commands family */
    cmd_interface_init(cli);

    /* 'vlan' commands family */
    cmd_vlan_init(cli);

    /* 'mac-address-table' commands family */
    cmd_mac_address_table_init(cli);

    /* 'mvrp' commands family */
    cmd_mvrp_init(cli);
}
