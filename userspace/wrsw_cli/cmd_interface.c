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

enum interface_cmds {
    CMD_INTERFACE = 0,
    CMD_INTERFACE_PORT,
    CMD_INTERFACE_PORT_PVID,
    CMD_SHOW_INTERFACE,
    CMD_SHOW_INTERFACE_INFORMATION,
    NUM_INTERFACE_CMDS
};

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
 * \brief Command 'show interface information'.
 * This command shows general information on ports (including MVRP related
 * parameters).
 * @param cli CLI interpreter.
 * @param argc unused
 * @param agv unused
 */
void cli_cmd_show_port_info(struct cli_shell *cli, int argc, char **argv)
{
    oid _oid[MAX_OID_LEN];
    oid aux_oid[MAX_OID_LEN];
    oid new_oid[MAX_OID_LEN];
    char *base_oid = ".1.3.111.2.802.1.1.4.1.4.5.1.1";
    size_t length_oid;  /* Base OID length */
    int pvid, port;
    int mvrp_enabled;           /* MVRP port status */
    int mvrp_restricted;        /* MVRP port restricted registrations */
    uint64_t mvrp_failed;       /* MVRP port failed registrations */
    char *mvrp_lpo;             /* MVRP port last PDU origin */
    char mac_str[3 * ETH_ALEN];


    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid, &length_oid))
        return;

    /* We initialize the OIDs with the OID of the table */
    memcpy(new_oid, _oid, MAX_OID_LEN * sizeof(oid));

    /* Header */
    printf("\t            MVRP\n");
    printf("\t            ---------------------------------------------------\n");
    printf("\tPort  PVID  Status   Registration Failed      Last PDU from   \n");
    printf("\t----  ----  -------- ------------ ----------  -----------------\n");

    do {
        errno = 0;
        pvid = cli_snmp_getnext_int(new_oid, &length_oid);
        if (errno != 0)
            break;
        if (cmp_oid(_oid, new_oid, 11) < 0)
            break;
        if (cmp_oid(_oid, new_oid, 11) > 0)
            break;

        port = (int)new_oid[14];

        memcpy(aux_oid, new_oid, length_oid * sizeof(oid));

        aux_oid[12] = 4; /* MVRP port status column */
        errno = 0;
        mvrp_enabled = cli_snmp_get_int(aux_oid, length_oid);
        if (errno != 0)
            break;

        aux_oid[12] = 7; /* MVRP port restricted registration column */
        errno = 0;
        mvrp_restricted = cli_snmp_get_int(aux_oid, length_oid);
        if (errno != 0)
            break;

        aux_oid[12] = 5; /* MVRP port failed registrations */
        errno = 0;
        mvrp_failed = cli_snmp_get_counter(aux_oid, length_oid);
        if (errno != 0)
            break;

        aux_oid[12] = 6; /* MVRP port last PDU origin */
        errno = 0;
        mvrp_lpo = cli_snmp_get_string(aux_oid, length_oid);
        if (errno != 0)
            break;

        printf("\t%-4d  %-4d  %-8s %-12s %-10lld  %-17s\n",
               port,
               pvid,
               (mvrp_enabled == TV_TRUE) ? "Enabled" : "Disabled",
               (mvrp_restricted == TV_TRUE) ? "Restricted" : "    *",
               mvrp_failed,
               mac_to_str((uint8_t *)mvrp_lpo, mac_str));

        memcpy(_oid, new_oid, sizeof(oid) * MAX_OID_LEN);
        free(mvrp_lpo);
    } while(1);

    return;
}

/* Define the 'interface' commands family */
struct cli_cmd cli_interface[NUM_INTERFACE_CMDS] = {
    /* interface */
    [CMD_INTERFACE] = {
        .parent     = NULL,
        .name       = "interface",
        .handler    = NULL,
        .desc       = "Interface configuration",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* interface port <port number> */
    [CMD_INTERFACE_PORT] = {
        .parent     = cli_interface + CMD_INTERFACE,
        .name       = "port",
        .handler    = NULL,
        .desc       = "Port configuration",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<port number> Port Number"
    },
    /* interface port <port number> pvid <VLAN number> */
    [CMD_INTERFACE_PORT_PVID] = {
        .parent     = cli_interface + CMD_INTERFACE_PORT,
        .name       = "pvid",
        .handler    = cli_cmd_set_port_pvid,
        .desc       = "Sets the PVID value for the port",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<VLAN number> VLAN Number"
    },
    /* show interface */
    [CMD_SHOW_INTERFACE] = {
        .parent     = &cli_show,
        .name       = "interface",
        .handler    = NULL,
        .desc       = "Displays interface information",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* show interface information */
    [CMD_SHOW_INTERFACE_INFORMATION] = {
        .parent     = cli_interface + CMD_SHOW_INTERFACE,
        .name       = "information",
        .handler    = cli_cmd_show_port_info,
        .desc       = "Displays general interface information (including some "
                      "MVRP parameters)",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
};

/**
 * \brief Init function for the command family 'interface'.
 * @param cli CLI interpreter.
 */
void cmd_interface_init(struct cli_shell *cli)
{
    int i;

    for (i = 0; i < NUM_INTERFACE_CMDS; i++)
        cli_insert_command(cli, &cli_interface[i]);
}
