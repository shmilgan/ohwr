/*
 * White Rabbit Switch CLI (Command Line Interface)
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizan   (miguel.baizan@integrasys.es)
 *              Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: Commands related stuff.
 *              Building of the commands tree.
 *              Implementation of the functions that handle each command.
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


/* TODO Take these values from the RTU code */
#define MIN_AGING_TIME          10
#define MAX_AGING_TIME          1000000
#define MAX_VID                 4094
#define NUM_PORTS               32

/* Function to build the commands tree */
void cli_build_commands_tree(struct cli_shell *cli);

/* Command Registration and de-registration functions */
struct cli_cmd *cli_register_command(struct cli_shell *cli,
                                            struct cli_cmd *parent,
                                            char *command,
                                            void (*handler) (struct cli_shell *,
                                                            int, char **),
                                            char *desc,
                                            int opt,
                                            char *opt_desc);
void cli_unregister_commands(struct cli_cmd *top_cmd);

/* Commands */
void cli_cmd_help(struct cli_shell *cli, struct cli_cmd *cmd);
void cli_cmd_exit(struct cli_shell *cli, int argc, char **argv);
void cli_cmd_hostname(struct cli_shell *cli, int argc, char **argv);
void cli_cmd_show_port_info(struct cli_shell *cli, int argc, char **argv);
void cli_cmd_show_cam_aging(struct cli_shell *cli, int argc, char **argv);
void cli_cmd_show_cam(struct cli_shell *cli, int argc, char **argv);
void cli_cmd_show_cam_multi(struct cli_shell *cli, int argc, char **argv);
void cli_cmd_show_cam_static(struct cli_shell *cli, int argc, char **argv);
void cli_cmd_show_cam_static_multi(struct cli_shell *cli, int argc, char **argv);
void cli_cmd_show_vlan(struct cli_shell *cli, int argc, char **argv);
void cli_cmd_set_port_pvid(struct cli_shell *cli, int argc, char **argv);
void cli_cmd_set_cam_aging(struct cli_shell *cli, int argc, char **argv);
void cli_cmd_set_cam_static_entry(struct cli_shell *cli, int argc, char **argv);
void cli_cmd_set_cam_multi_entry(struct cli_shell *cli, int argc, char **argv);
void cli_cmd_del_cam_static_entry(struct cli_shell *cli, int argc, char **argv);
void cli_cmd_del_cam_multi_entry(struct cli_shell *cli, int argc, char **argv);
void cli_cmd_set_vlan(struct cli_shell *cli, int argc, char **argv);
void cli_cmd_del_vlan(struct cli_shell *cli, int argc, char **argv);


#endif /*__WHITERABBIT_CLI_COMMANDS_H*/
