/*
 * White Rabbit Switch CLI (Command Line Interface)
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizan   (miguel.baizan@integrasys.es)
 *              Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: Implementation of the commands family 'mvrp'.
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

enum mvrp_cmds {
    CMD_MVRP = 0,
    CMD_MVRP_ENABLE,
    CMD_MVRP_DISABLE,
    CMD_MVRP_PORT,
    CMD_MVRP_PORT_ENABLE,
    CMD_MVRP_PORT_DISABLE,
    CMD_MVRP_PORT_RESTRICTED_REGISTRATION,
    CMD_MVRP_PORT_RESTRICTED_REGISTRATION_ENABLE,
    CMD_MVRP_PORT_RESTRICTED_REGISTRATION_DISABLE,
    NUM_MVRP_CMDS
};

static void mvrp_enabled_status(char *val)
{
    oid _oid[MAX_OID_LEN];
    char *base_oid = ".1.3.111.2.802.1.1.4.1.1.1.1.6.1";
    size_t length_oid;  /* Base OID length */

    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid, &length_oid))
        return;

    cli_snmp_set_int(_oid, length_oid, val, 'i');
}

/**
 * \brief Command 'mvrp enable'.
 * This command enables MVRP on the device.
 * @param cli CLI interpreter.
 * @param argc unused.
 * @param agv unused.
 */
void cli_cmd_mvrp_enable(struct cli_shell *cli, int argc, char **argv)
{
    mvrp_enabled_status(S_TRUE);
    return;
}

/**
 * \brief Command 'mvrp disable'.
 * This command disables MVRP on the device.
 * @param cli CLI interpreter.
 * @param argc unused.
 * @param agv unused.
 */
void cli_cmd_mvrp_disable(struct cli_shell *cli, int argc, char **argv)
{
    mvrp_enabled_status(S_FALSE);
    return;
}

static void mvrp_port_enabled_status(int port, char *val)
{
    oid _oid[MAX_OID_LEN];
    char *base_oid = ".1.3.111.2.802.1.1.4.1.4.5.1.4.1";
    size_t length_oid;  /* Base OID length */

    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid, &length_oid))
        return;

    /* Build the indexes */
    _oid[14] = port;              /* port number */
    length_oid++;

    cli_snmp_set_int(_oid, length_oid, val, 'i');
}

/**
 * \brief Command 'mvrp port <port number> enable'.
 * This command enables MVRP on a given port/ports.
 * @param cli CLI interpreter.
 * @param argc number of arguments. Only one argument allowed.
 * @param agv One argument must be specified: the port number (a decimal
 * number between 0 and NUM_PORTS-1).
 */
void cli_cmd_mvrp_port_enable(struct cli_shell *cli, int argc, char **argv)
{
    char mask[NUM_PORTS + 1];
    int i;

    /* Check that we have the argument */
    if (argc != 1) {
        printf("\tError. You have missed some argument\n");
        return;
    }

    /* Parse port numbers to port mask and check the syntax */
    if (ports_to_mask(argv[0], mask) != 0)
        return;

    for (i = 0; i < NUM_PORTS; i++)
        if (mask[i] == '1')
            mvrp_port_enabled_status(i, "1");

    return;
}

/**
 * \brief Command 'mvrp port <port number> disable'.
 * This command disables MVRP on a given port/ports.
 * @param cli CLI interpreter.
 * @param argc number of arguments. Only one argument allowed.
 * @param agv One argument must be specified: the port number (a decimal
 * number between 0 and NUM_PORTS-1).
 */
void cli_cmd_mvrp_port_disable(struct cli_shell *cli, int argc, char **argv)
{
    char mask[NUM_PORTS + 1];
    int i;

    /* Check that we have the argument */
    if (argc != 1) {
        printf("\tError. You have missed some argument\n");
        return;
    }

    /* Parse port numbers to port mask and check the syntax */
    if (ports_to_mask(argv[0], mask) != 0)
        return;


    for (i = 0; i < NUM_PORTS; i++)
        if (mask[i] == '1')
            mvrp_port_enabled_status(i, "2");

    return;
}

static void mvrp_restricted_registration_status(int port, char *val)
{
    oid _oid[MAX_OID_LEN];
    char *base_oid = ".1.3.111.2.802.1.1.4.1.4.5.1.7.1";
    size_t length_oid;  /* Base OID length */

    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid, &length_oid))
        return;

    /* Build the indexes */
    _oid[14] = port;              /* port number */
    length_oid++;

    cli_snmp_set_int(_oid, length_oid, val, 'i');
}

/**
 * \brief Command 'mvrp port <port number> restricted-registration enable'.
 * This command enables the Restricted VLAN Registration state on a given
 * port/ports.
 * @param cli CLI interpreter.
 * @param argc number of arguments. Only one argument allowed.
 * @param agv One argument must be specified: the port number (a decimal
 * number between 0 and NUM_PORTS-1).
 */
void cli_cmd_mvrp_restricted_registration_enable(struct cli_shell *cli, int argc,
                                                 char **argv)
{
    char mask[NUM_PORTS + 1];
    int i;

    /* Check that we have the argument */
    if (argc != 1) {
        printf("\tError. You have missed some argument\n");
        return;
    }

    /* Parse port numbers to port mask and check the syntax */
    if (ports_to_mask(argv[0], mask) != 0)
        return;

    for (i = 0; i < NUM_PORTS; i++)
        if (mask[i] == '1')
            mvrp_restricted_registration_status(i, S_TRUE);

    return;
}

/**
 * \brief Command 'mvrp port <port number> restricted-registration disable'.
 * This command disables the Restricted VLAN Registration state on a given
 * port/ports.
 * @param cli CLI interpreter.
 * @param argc number of arguments. Only one argument allowed.
 * @param agv One argument must be specified: the port number (a decimal
 * number between 0 and NUM_PORTS-1).
 */
void cli_cmd_mvrp_restricted_registration_disable(struct cli_shell *cli,
                                                  int argc, char **argv)
{
    char mask[NUM_PORTS + 1];
    int i;

    /* Check that we have the argument */
    if (argc != 1) {
        printf("\tError. You have missed some argument\n");
        return;
    }

    /* Parse port numbers to port mask and check the syntax */
    if (ports_to_mask(argv[0], mask) != 0)
        return;

    for (i = 0; i < NUM_PORTS; i++)
        if (mask[i] == '1')
            mvrp_restricted_registration_status(i, S_FALSE);

    return;
}

/* Define the 'mvrp' commands family */
struct cli_cmd cli_mvrp[NUM_MVRP_CMDS] = {
    /* mvrp */
    [CMD_MVRP] = {
        .parent     = NULL,
        .name       = "mvrp",
        .handler    = NULL,
        .desc       = "MVRP configuration",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* mvrp enable */
    [CMD_MVRP_ENABLE] = {
        .parent     = cli_mvrp + CMD_MVRP,
        .name       = "enable",
        .handler    = cli_cmd_mvrp_enable,
        .desc       = "Enables MVRP in the device",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* mvrp disable */
    [CMD_MVRP_DISABLE] = {
        .parent     = cli_mvrp + CMD_MVRP,
        .name       = "disable",
        .handler    = cli_cmd_mvrp_disable,
        .desc       = "Disables MVRP in the device",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* mvrp port <port number> */
    [CMD_MVRP_PORT] = {
        .parent     = cli_mvrp + CMD_MVRP,
        .name       = "port",
        .handler    = NULL,
        .desc       = "Configure MVRP on a given port",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<port number> port numbers separated by commas"
    },
    /* mvrp port <port number> enable */
    [CMD_MVRP_PORT_ENABLE] = {
        .parent     = cli_mvrp + CMD_MVRP_PORT,
        .name       =  "enable",
        .handler    = cli_cmd_mvrp_port_enable,
        .desc       = "Enable MVRP operation on this port",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* mvrp port <port number> disable */
    [CMD_MVRP_PORT_DISABLE] = {
        .parent     = cli_mvrp + CMD_MVRP_PORT,
        .name       = "disable",
        .handler    = cli_cmd_mvrp_port_disable,
        .desc       = "Disable MVRP operation on this port",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* mvrp port <port number> restricted-registration */
    [CMD_MVRP_PORT_RESTRICTED_REGISTRATION] = {
        .parent     = cli_mvrp + CMD_MVRP_PORT,
        .name       = "restricted-registration",
        .handler    = NULL,
        .desc       = "Sets the state of the Restricted VLAN Registration on "
                      "this port",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* mvrp port <port number> restricted-registration enable */
    [CMD_MVRP_PORT_RESTRICTED_REGISTRATION_ENABLE] = {
        .parent     = cli_mvrp + CMD_MVRP_PORT_RESTRICTED_REGISTRATION,
        .name       = "enable",
        .handler    = cli_cmd_mvrp_restricted_registration_enable,
        .desc       = "Enable the state of the Restricted VLAN Registration on"
                      " this port",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* mvrp port <port number> restricted-registration disable */
    [CMD_MVRP_PORT_RESTRICTED_REGISTRATION_DISABLE] = {
        .parent     = cli_mvrp + CMD_MVRP_PORT_RESTRICTED_REGISTRATION,
        .name       = "disable",
        .handler    = cli_cmd_mvrp_restricted_registration_disable,
        .desc       = "Disable the state of the Restricted VLAN Registration on"
                      " this port",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    }
};

/**
 * \brief Init function for the command family 'mvrp'.
 * @param cli CLI interpreter.
 */
void cmd_mvrp_init(struct cli_shell *cli)
{
    int i;

    for (i = 0; i < NUM_MVRP_CMDS; i++)
        cli_insert_command(cli, &cli_mvrp[i]);
}
