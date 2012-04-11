/*
 * White Rabbit Switch CLI (Command Line Interface)
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizan   (miguel.baizan@integrasys.es)
 *              Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: Implementation of the commands family 'mvrp'.
 *              Some MVRP management functions are implemented in the
 *              'interface' commands family.
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
    CMD_SHOW_MVRP,
    CMD_SHOW_MVRP_STATUS,
    NUM_MVRP_CMDS
};

enum mvrp_status_entities {
    MVRP_STATUS_PROTOCOL = 0,
    MVRP_STATUS_PORT,
    MVRP_STATUS_RESTRICTED_REGISTRATION
};

/* Used to enable/disable MVRP entities */
struct mvrp_status {
    int     entity;     /* One of the mvrp_status_entities */
    int     port;       /* Port number (when applicable) */
    char    *enabled;   /* TRUE or FALSE as defined by SNMP (strings) */
    char    *base_oid;
};

/* Enable/Disable MVRP, MVRP in specific ports or Restricted Registration
parameter in specific ports */
static void mvrp_set_status(struct mvrp_status *status)
{
    oid _oid[MAX_OID_LEN];
    size_t length_oid;  /* Base OID length */

    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid(status->base_oid, _oid, &length_oid))
        return;

    /* Build the indexes if info applies to ports */
    if (status->entity != MVRP_STATUS_PROTOCOL) {
        _oid[14] = status->port;
        length_oid++;
    }

    cli_snmp_set_int(_oid, length_oid, status->enabled, 'i');
}

/* Set the port number and base OID when operation applies to Port or
Restricted Registration entities */
static void mvrp_port_status(int valc, char **valv, struct mvrp_status *status)
{
    char mask[NUM_PORTS + 1];
    int i;

    if (valc != 1) {
        printf("\tError. You have missed some command option\n");
        return;
    }

    /* Parse port numbers to port mask and check the syntax */
    if (ports_to_mask(valv[0], mask) != 0)
        return;

    status->base_oid = (status->entity == MVRP_STATUS_PORT) ?
        ".1.3.111.2.802.1.1.4.1.4.5.1.4.1" :    /* Port */
        ".1.3.111.2.802.1.1.4.1.4.5.1.7.1";     /* Restricted Registration */

    for (i = 0; i < NUM_PORTS; i++)
        if (mask[i] == '1') {
            status->port = i;
            mvrp_set_status(status);
        }

    return;
}

/**
 * \brief Command 'mvrp enable'.
 * This command enables MVRP on the device.
 * @param cli CLI interpreter.
 * @param valc unused.
 * @param valv unused.
 */
void cli_cmd_mvrp_enable(struct cli_shell *cli, int valc, char **valv)
{
    static struct mvrp_status status = {
        .entity = MVRP_STATUS_PROTOCOL,
        .enabled = S_TRUE,
        .base_oid = ".1.3.111.2.802.1.1.4.1.1.1.1.6.1"
    };

    mvrp_set_status(&status);
}

/**
 * \brief Command 'mvrp disable'.
 * This command disables MVRP on the device.
 * @param cli CLI interpreter.
 * @param valc unused.
 * @param valv unused.
 */
void cli_cmd_mvrp_disable(struct cli_shell *cli, int valc, char **valv)
{
    static struct mvrp_status status = {
        .entity = MVRP_STATUS_PROTOCOL,
        .enabled = S_FALSE,
        .base_oid = ".1.3.111.2.802.1.1.4.1.1.1.1.6.1"
    };

    mvrp_set_status(&status);
}

/**
 * \brief Command 'mvrp port <port number> enable'.
 * This command enables MVRP on a given port/ports.
 * @param cli CLI interpreter.
 * @param valc number of arguments. Only one argument allowed.
 * @param valv One argument must be specified: the port number.
 */
void cli_cmd_mvrp_port_enable(struct cli_shell *cli, int valc, char **valv)
{
    static struct mvrp_status status = {
        .entity = MVRP_STATUS_PORT,
        .enabled = S_TRUE,
    };

    mvrp_port_status(valc, valv, &status);
}

/**
 * \brief Command 'mvrp port <port number> disable'.
 * This command disables MVRP on a given port/ports.
 * @param cli CLI interpreter.
 * @param valc number of arguments. Only one argument allowed.
 * @param valv One argument must be specified: the port number.
 */
void cli_cmd_mvrp_port_disable(struct cli_shell *cli, int valc, char **valv)
{
    static struct mvrp_status status = {
        .entity = MVRP_STATUS_PORT,
        .enabled = S_FALSE,
    };

    mvrp_port_status(valc, valv, &status);
}

/**
 * \brief Command 'mvrp port <port number> restricted-registration enable'.
 * This command enables the Restricted VLAN Registration state on a given
 * port/ports.
 * @param cli CLI interpreter.
 * @param valc number of arguments. Only one argument allowed.
 * @param valv One argument must be specified: the port number.
 */
void cli_cmd_mvrp_restricted_registration_enable(struct cli_shell *cli, int valc,
                                                 char **valv)
{
    static struct mvrp_status status = {
        .entity = MVRP_STATUS_RESTRICTED_REGISTRATION,
        .enabled = S_TRUE,
    };

    mvrp_port_status(valc, valv, &status);

}

/**
 * \brief Command 'mvrp port <port number> restricted-registration disable'.
 * This command disables the Restricted VLAN Registration state on a given
 * port/ports.
 * @param cli CLI interpreter.
 * @param valc number of arguments. Only one argument allowed.
 * @param valv One argument must be specified: the port number.
 */
void cli_cmd_mvrp_restricted_registration_disable(struct cli_shell *cli,
                                                  int valc, char **valv)
{
    static struct mvrp_status status = {
        .entity = MVRP_STATUS_RESTRICTED_REGISTRATION,
        .enabled = S_FALSE,
    };

    mvrp_port_status(valc, valv, &status);
}

/**
 * \brief Command 'show mvrp status'.
 * This command displays the MVRP status on the device.
 * @param cli CLI interpreter.
 * @param valc unused.
 * @param valv unused.
 */
void cli_cmd_show_mvrp_status(struct cli_shell *cli, int valc, char **valv)
{
    oid _oid[MAX_OID_LEN];
    char *base_oid = ".1.3.111.2.802.1.1.4.1.1.1.1.6.1";
    size_t length_oid;  /* Base OID length */
    int enabled;


    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid, &length_oid))
        return;

    errno = 0;
    enabled = cli_snmp_get_int(_oid, length_oid);

    if (errno == 0)
        printf("\tMVRP status: %s\n",
               (enabled == TV_TRUE) ? "Enabled" : "Disabled" );

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
    },
    /* show mvrp */
    [CMD_SHOW_MVRP] = {
        .parent     = &cli_show,
        .name       = "mvrp",
        .handler    = NULL,
        .desc       = "Displays MVRP information",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* show mvrp status */
    [CMD_SHOW_MVRP_STATUS] = {
        .parent     = cli_mvrp + CMD_SHOW_MVRP,
        .name       = "status",
        .handler    = cli_cmd_show_mvrp_status,
        .desc       = "Displays the MVRP Enabled Status",
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
