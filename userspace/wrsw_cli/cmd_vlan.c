/*
 * White Rabbit Switch CLI (Command Line Interface)
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizan   (miguel.baizan@integrasys.es)
 *              Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: Implementation of the commands family 'vlan'.
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
#include <string.h>
#include <ctype.h>

#include "cli_commands.h"
#include "cli_commands_utils.h"


/**
 * \brief Command 'vlan <VID> member <Port number>'.
 * This command creates a new VLAN.
 * @param cli CLI interpreter.
 * @param argc number of arguments. Only two arguments allowed.
 * @param agv Two arguments must be specified: the VLAN number (a decimal
 * number between 0 and MAX_VID+1) and the port number (decimal port numbers,
 * separatted by commas and with no blank spaces in between).
 */
void cli_cmd_set_vlan(struct cli_shell *cli, int argc, char **argv)
{
    oid _oid[2][MAX_OID_LEN];
    char *base_oid = "1.3.111.2.802.1.1.4.1.4.3.1.7";
    size_t length_oid[2]; /* Base OID length */
    int vid;
    char ports[(2*NUM_PORTS)+1];
    char mask[NUM_PORTS+1];
    char types[2];
    char *value[2];
    int i;

    /* Check that we have the three arguments */
    if (argc != 2) {
        printf("\tError. You have missed some argument\n");
        return;
    }

    /* Check the syntax of the vlan argument */
    for (i = 0; argv[0][i]; i++) {
        if (!isdigit(argv[0][i])) {
            printf("\tError. Only decimal values are allowed\n");
            return;
        }
    }
    if ((atoi(argv[0]) < 0) || (atoi(argv[0]) > (MAX_VID + 1))) {
        printf("\tError. Allowed values are in the range 0 to %d\n",
                (MAX_VID + 1));
        return;
    }
    vid = atoi(argv[0]);

    /* Parse port numbers to port mask and check the syntax */
    ports_to_mask(argv[1], mask);
    if (mask[0] == 'e') {
        printf("\tError. Ports must be decimal numbers separated by commas,\n");
        printf("\twith no blank spaces in between. Valid range: from 0 to %d\n",
                (NUM_PORTS-1));
        return;
    }
    memset(ports, '0', 2*NUM_PORTS);
    for (i = 0; i < NUM_PORTS; i++)
        ports[(2*i)+1] = mask[i];
    ports[2*NUM_PORTS] = '\0';

    memset(_oid[0], 0 , MAX_OID_LEN * sizeof(oid));
    memset(_oid[1], 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid[0] = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid[0], &length_oid[0]))
        return;

    /* Build the indexes */
    _oid[0][13] = 1;                /* Component ID column */
    _oid[0][14] = vid;              /* VID column */

    length_oid[0] += 2;
    memcpy(_oid[1], _oid[0], length_oid[0] * sizeof(oid));
    length_oid[1] = length_oid[0];

    /* Fill with data. Remember that we have to handle the Row Status */
    _oid[1][12] = 4;                     /* Egress ports column */
    value[0] = "4";                     /* Row status (create = 4) */
    value[1] = ports;                   /* Egress ports */
    types[0] = 'i';                     /* Type integer */
    types[1] = 'x';                     /* Type string */

    cli_snmp_set(_oid, length_oid, value, types, 2);

    return;
}

/**
 * \brief Registration function for the command family 'vlan'.
 * @param cli CLI interpreter.
 */
void cmd_vlan_init(struct cli_shell *cli)
{
    struct cli_cmd *c;

    c = cli_register_command(cli, NULL, "vlan", NULL, "VLAN configuration",
            CMD_ARG_MANDATORY, "<VID> VLAN number");

    /* vlan <VID> member <port number> */
    cli_register_command(cli, c, "member", cli_cmd_set_vlan,
        "Creates a new VLAN", CMD_ARG_MANDATORY,
        "<port number> port numbers separatted by commas");
}
