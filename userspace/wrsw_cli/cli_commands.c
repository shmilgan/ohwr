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
 * Depending on the previous parent commands, each subcommand has a different
 * handler. This is a graphical example of a commands tree:
 *
 * +--------------+
 * | root command |
 * +--------------+
 *        |
 *        |
 * +--------------+
 * | next command |
 * +--------------+
 *        |
 *        |
 * +--------------+         +---------------+        +--------------+
 * | next command |-------->| child command |------->| child command|
 * +--------------+         +---------------+        +--------------+
 *        |                         |
 *        |                         |
 * +--------------+         +--------------+
 * | next command |         | next command |
 * +--------------+         +--------------+
 *
 *
 */

#define NUM_PORTS 32



/**
 * \brief Register a command in the tree.
 * It inserts a command in the command tree.
 * @param cli CLI interpreter.
 * @param parent pointer to the parent command. If it has no parent, then this
 *        parameter is NULL.
 * @param handler pointer to the function that handles the command. It can be
 *        NULL if the command has no meaning itself, but it has meaning as a
 *        parent of other command(s).
 * @param desc description of the command. This is printed when help is
 *        requested.
 * @return pointer to the new created command. It can be NULL if any error
 *         occurs.
 */
struct cli_cmd *cli_register_command(
    struct cli_shell *cli, struct cli_cmd *parent, char *command,
    void (*handler)(struct cli_shell *, int, char **),char *desc,
    int opt, char *opt_desc)
{
    struct cli_cmd *cmd, *c;

    /* Check that the command name is not empty */
    if (!*command)
        cli_error(EINVAL);

    /* Allocate the structure for the command */
    cmd = (struct cli_cmd *)malloc(sizeof(struct cli_cmd));
    if (!cmd)
        cli_error(ENOMEM);

    /* Clear structure */
    memset(cmd, 0, sizeof(struct cli_cmd));

    /* Set the command information */
    cmd->name = command;
    cmd->parent = parent;
    cmd->handler = handler;
    cmd->desc = desc;
    cmd->opt = opt;
    cmd->opt_desc = opt_desc;

    /* Insert the command in the tree */
    if (parent) {
        /* Move to the parent and add the new command to the list of children */
        if (!parent->child) {
            parent->child = cmd;
        } else { /* Traverse the list */
            c = parent->child;
            while (c) {
                if (c->next) {
                    c = c->next;
                } else {
                    c->next = cmd;
                    break;
                }
            }
        }
    } else { /* The new command has no parent */
        c = cli->root_cmd;
        if(!c) {
            cli->root_cmd = cmd;
        } else {
            while (c) {
                if (c->next) {
                    c = c->next;
                } else {
                    c->next = cmd;
                    break;
                }
            }
        }
    }

    return cmd;
}

/**
 * \brief Build the commands tree.
 * The commands tree is built as a linked list of commands, starting with a
 * root command, whose reference is stored in the cli_shell structure
 * in order to keep a track of the whole list. Each command may have a
 * parent, a children and a reference to the next command with shared parent.
 * Here you can add new commands for registration in the commands tree. Remember
 * also that you have to implement the handler for the commands you create.
 * @param cli CLI interpreter.
 */
void cli_build_commands_tree(struct cli_shell *cli)
{
    /* 'exit' command */
    cmd_exit_init(cli);

    /* 'hostname' command */
    cmd_hostname_init(cli);

    /* 'show' commands family */
    cmd_show_init(cli);

    /* 'interface' commands family */
    cmd_interface_init(cli);

    /* 'vlan' commands family */
    cmd_vlan_init(cli);

    /* 'mac-address-table' commands family */
    cmd_mac_address_table_init(cli);

    /* 'no' commands family */
    cmd_no_init(cli);
}
