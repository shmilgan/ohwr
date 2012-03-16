/*
 * White Rabbit Switch CLI (Command Line Interface)
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizan   (miguel.baizan@integrasys.es)
 *              Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: Miscellaneous helper functions for the command handlers.
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

#ifndef __WHITERABBIT_CLI_COMMANDS_UTILS_H
#define __WHITERABBIT_CLI_COMMANDS_UTILS_H


#include <linux/if_ether.h>

#include <rtu.h>

#include "cli.h"
#include "cli_snmp.h"

#define LC_MAX_SET_ID   31  /* TODO: read this constant from RTU */

/* The help command is treated specially */
void cli_cmd_help(struct cli_shell *cli, struct cli_cmd *cmd);

char *mac_to_str(uint8_t mac[ETH_ALEN], char str[3 * ETH_ALEN]);
int cmp_oid(oid old_oid[MAX_OID_LEN], oid new_oid[MAX_OID_LEN],
            int base_oid_length);
void print_oid(oid _oid[MAX_OID_LEN], int n) __attribute__((unused));
void mask_to_ports(char *mask, int *ports_range);
int ports_to_mask(char *ports_range, char *mask);

/* Check command options syntax */
int is_port(char *port);
int is_vid(char *vid);
int is_aging(char *aging);
int is_sid(char *sid);
int is_fid(char *fid);


#endif /*__WHITERABBIT_CLI_COMMANDS_H*/
