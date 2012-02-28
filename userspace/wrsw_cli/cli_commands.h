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

#ifndef __WHITERABBIT_CLI_COMMANDS_H
#define __WHITERABBIT_CLI_COMMANDS_H

#include "cli.h"

/* Function to build the commands tree */
void cli_build_commands_tree(struct cli_shell *cli);

void cli_insert_command(struct cli_shell *cli, struct cli_cmd *cmd);

/* Functions to init each commands family. Look in cmd_*.c for implementation */
void cmd_exit_init(struct cli_shell *cli);
void cmd_hostname_init(struct cli_shell *cli);
void cmd_show_init(struct cli_shell *cli);
void cmd_mac_address_table_init(struct cli_shell *cli);
void cmd_no_init(struct cli_shell *cli);
void cmd_interface_init(struct cli_shell *cli);
void cmd_vlan_init(struct cli_shell *cli);
void cmd_mvrp_init(struct cli_shell *cli);

#endif /*__WHITERABBIT_CLI_COMMANDS_H*/
