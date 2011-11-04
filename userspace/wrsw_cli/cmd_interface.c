/*
 * White Rabbit Switch CLI (Command Line Interface)
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizan   (miguel.baizan@integrasys.es)
 *              Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: Implementation of the commands family 'interface'.
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
 * \brief Command 'interface port <port number> pvid <VLAN number>'.
 * This command sets the Port VLAN number (PVID) value.
 * @param cli CLI interpreter.
 * @param argc number of arguments. Only two arguments allowed.
 * @param agv Two arguments must be specified: the port number (a decimal
 * number between 0 and NUM_PORTS-1) and the PVID value (a decimal number
 * between 0 and MAX_VID)
 */
void cli_cmd_set_port_pvid(struct cli_shell *cli, int argc, char **argv)
{
    oid _oid[MAX_OID_LEN];
    char *base_oid = "1.3.111.2.802.1.1.4.1.4.5.1.1.1";
    size_t length_oid;  /* Base OID length */
    int i;
    int port;

    /* Check that we have the three arguments */
    if (argc != 2) {
        printf("\tError. You have missed some argument\n");
        return;
    }

    /* Check the syntax of the port argument */
    for (i = 0; argv[0][i]; i++) {
        if (!isdigit(argv[0][i])) {
            printf("\tError. Only decimal values are allowed\n");
            return;
        }
    }
    if ((atoi(argv[0]) < 0) || (atoi(argv[0]) >= NUM_PORTS)) {
        printf("\tError. Allowed values are in the range 0 to %d\n",
                (NUM_PORTS-1));
        return;
    }
    port = atoi(argv[0]);

    /* Check the syntax of the vlan argument */
    for (i = 0; argv[1][i]; i++) {
        if (!isdigit(argv[1][i])) {
            printf("\tError. Only decimal values are allowed\n");
            return;
        }
    }
    if ((atoi(argv[1]) <= 0) || (atoi(argv[1]) > (MAX_VID))) {
        printf("\tError. Allowed values are in the range 1 to %d\n",
                MAX_VID);
        return;
    }


    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid, &length_oid))
        return;

    /* Build the indexes */
    _oid[14] = port;              /* PVID column */
    length_oid++;

    cli_snmp_set_int(_oid, length_oid, argv[1], 'i');

    return;
}

/**
 * \brief Registration function for the command family 'interface'.
 * @param cli CLI interpreter.
 */
void cmd_interface_init(struct cli_shell *cli)
{
    struct cli_cmd *c, *s;

    c = cli_register_command(cli, NULL, "interface", NULL,
            "Interface configuration", CMD_NO_ARG, NULL);

    /* interface port <port number> */
    s = cli_register_command(cli, c, "port", NULL,
        "Port configuration", CMD_ARG_MANDATORY, "<port number> Port Number");
    /* interface port <port number> pvid <VLAN number> */
    cli_register_command(cli, s, "pvid", cli_cmd_set_port_pvid,
        "Sets the PVID value for the port",
        CMD_ARG_MANDATORY, "<VLAN number> VLAN Number");
}
