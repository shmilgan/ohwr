/*
 * White Rabbit Switch CLI (Command Line Interface)
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizan   (miguel.baizan@integrasys.es)
 *              Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: Implementation of the commands family 'mac-address-table'.
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

enum cam_cmds {
    CMD_CAM = 0,
    CMD_CAM_AGING,
    CMD_CAM_STATIC,
    CMD_CAM_STATIC_VLAN,
    CMD_CAM_STATIC_VLAN_PORT,
    CMD_CAM_MULTICAST,
    CMD_CAM_MULTICAST_VLAN,
    CMD_CAM_MULTICAST_VLAN_PORT,
    NUM_CAM_CMDS
};

/* Helper function to create static entries in the FDB (both
   unicast or multicast, depending on the OID passed as argument). The column
   identifier for the port mask is also needed, since it's different for the
   unicast and multicast tables  */
static void set_cam_static(int argc, char **argv, char *base_oid,
    int egress_ports_column)
{
    oid _oid[2][MAX_OID_LEN];
    size_t length_oid[2]; /* Base OID length */
    char *addr;
    unsigned int mac[ETH_ALEN];
    int vid;
    char mask[NUM_PORTS+1];
    char ports[(2*NUM_PORTS)+1];
    char types[2];
    char *value[2];
    int i;

    /* Check that we have the three arguments */
    if (argc != 3) {
        printf("\tError. You have missed some argument\n");
        return;
    }

    /* Parse the MAC address */
    addr = argv[0];
    if (sscanf(addr, "%02x:%02x:%02x:%02x:%02x:%02x",
        &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6) {
        printf("\tError. Wrong MAC address format. Try: XX:XX:XX:XX:XX:XX\n");
        return;
    }

    /* Check the syntax of the vlan argument */
    for (i = 0; argv[1][i]; i++) {
        if (!isdigit(argv[1][i])) {
            printf("\tError. Only decimal values are allowed\n");
            return;
        }
    }
    if ((atoi(argv[1]) < 0) || (atoi(argv[1]) > (MAX_VID + 1))) {
        printf("\tError. Allowed values are in the range 0 to %d\n",
                (MAX_VID + 1));
        return;
    }
    vid = atoi(argv[1]);

    /* Parse port numbers to port mask and check the syntax */
    if (ports_to_mask(argv[2], mask) != 0)
        return;

    memset(ports, '0', 2*NUM_PORTS);
    for (i = 0; i < NUM_PORTS; i++)
        ports[(2*i)+1] = mask[i];
    ports[64] = '\0';

    memset(_oid[0], 0 , MAX_OID_LEN * sizeof(oid));
    memset(_oid[1], 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid[0] = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid[0], &length_oid[0]))
        return;

    /* Build the indexes */
    _oid[0][13] = 1;                /* Component ID column */
    _oid[0][14] = vid;              /* VID column */
    for (i = 15; i < 21 ; i++)      /* MAC address columns */
        _oid[0][i] = mac[i-15];
    _oid[0][21] = 0;                /* Receive Port column */

    length_oid[0] += 9;
    memcpy(_oid[1], _oid[0], length_oid[0] * sizeof(oid));
    length_oid[1] = length_oid[0];

    /* Fill with data. Remember that we have to handle the Row Status */
    _oid[1][12] = egress_ports_column;  /* Egress ports column */
    value[0] = "4";                     /* Row status (create = 4) */
    value[1] = ports;                   /* Egress ports */
    types[0] = 'i';                     /* Type integer */
    types[1] = 'x';                     /* Type string */

    cli_snmp_set(_oid, length_oid, value, types, 2);

    return;
}

/**
 * \brief Command 'mac-address-table aging-time <aging time>'.
 * This command sets a new aging time.
 * @param cli CLI interpreter.
 * @param argc number of arguments. Only one argument allowed.
 * @param agv new value for the aging time. Allowed values are between 10 and
 * 1000000.
 */
void cli_cmd_set_cam_aging(struct cli_shell *cli, int argc, char **argv)
{
    oid _oid[MAX_OID_LEN];
    char *base_oid = "1.3.111.2.802.1.1.4.1.2.1.1.5.1.0";
    size_t length_oid;  /* Base OID length */
    int i;

    /* Check that we have an argument */
    if (!argc) {
        printf("\tError. You must specify the new aging value\n");
        return;
    }

    /* Check the syntax of the argument */
    for (i = 0; argv[0][i]; i++) {
        if (!isdigit(argv[0][i])) {
            printf("\tError. Only decimal values are allowed\n");
            return;
        }
    }
    if ((atoi(argv[0]) < MIN_AGING_TIME) || (atoi(argv[0]) > MAX_AGING_TIME)) {
        printf("\tError. Allowed values are in the range %d to %d\n",
                MIN_AGING_TIME, MAX_AGING_TIME);
        return;
    }


    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid, &length_oid))
        return;

    cli_snmp_set_int(_oid, length_oid, argv[0], 'i');

    return;
}

/**
 * \brief Command 'mac-address-table static <MAC Addrress> vlan <VID>
 * port <port number>'.
 * This command creates a unicast static entry in the FDB.
 * @param cli CLI interpreter.
 * @param argc number of arguments. Only three arguments allowed.
 * @param agv Three arguments must be specified: the MAC Address (formatted as
 * XX:XX:XX:XX:XX:XX), the VLAN number (a decimal number between 0 and
 * MAX_VID+1) and the port number (decimal port numbers, separatted by commas
 * and with no blank spaces in between).
 */
void cli_cmd_set_cam_static_entry(struct cli_shell *cli, int argc, char **argv)
{
    set_cam_static(argc, argv, ".1.3.111.2.802.1.1.4.1.3.1.1.8", 5);
    return;
}

/**
 * \brief Command 'mac-address-table multicast <MAC Addrress> vlan <VID>
 * port <port number>'.
 * This command creates a multicast static entry in the FDB.
 * @param cli CLI interpreter.
 * @param argc number of arguments. Only three arguments allowed.
 * @param agv Three arguments must be specified: the MAC Address (formatted as
 * XX:XX:XX:XX:XX:XX), the VLAN number (a decimal number between 0 and
 * MAX_VID+1) and the port number (decimal port numbers, separatted by commas
 * and with no blank spaces in between).
 */
void cli_cmd_set_cam_multi_entry(struct cli_shell *cli, int argc, char **argv)
{
    set_cam_static(argc, argv, ".1.3.111.2.802.1.1.4.1.3.2.1.6", 3);
    return;
}

/* Define the 'mac-address-table' commands family */
struct cli_cmd cli_cam[NUM_CAM_CMDS] = {
    /* mac-address-table */
    [CMD_CAM] = {
        .parent     = NULL,
        .name       = "mac-address-table",
        .handler    = NULL,
        .desc       = "Configure MAC address table",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* mac-address-table aging-time <aging> */
    [CMD_CAM_AGING] = {
        .parent     = cli_cam + CMD_CAM,
        .name       = "aging-time",
        .handler    = cli_cmd_set_cam_aging,
        .desc       = "Sets the MAC address table aging time",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<aging value> New aging time"
    },
    /* mac-address-table static <MAC Addrress> */
    [CMD_CAM_STATIC] = {
        .parent     = cli_cam + CMD_CAM,
        .name       = "static",
        .handler    = NULL,
        .desc       = "Adds a static unicast entry in the filtering database",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<MAC Addrress> MAC Address"
    },
    /* mac-address-table static <MAC Addrress> vlan <VID> */
    [CMD_CAM_STATIC_VLAN] = {
        .parent     = cli_cam + CMD_CAM_STATIC,
        .name       = "vlan",
        .handler    = NULL,
        .desc       = "Adds a static unicast entry in the filtering database",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<VID> VLAN number"
    },
    /* mac-address-table static <MAC Addrress> vlan <VID> port <port number> */
    [CMD_CAM_STATIC_VLAN_PORT] = {
        .parent     = cli_cam + CMD_CAM_STATIC_VLAN,
        .name       = "port",
        .handler    = cli_cmd_set_cam_static_entry,
        .desc       = "Adds a static unicast entry in the filtering database",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<port number> Port numbers separatted by commas"
    },
    /* mac-address-table multicast <MAC Addrress> */
    [CMD_CAM_MULTICAST] = {
        .parent     = cli_cam + CMD_CAM,
        .name       = "multicast",
        .handler    = NULL,
        .desc       = "Adds a static multicast entry in the filtering database",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<MAC Addrress> MAC Address"
    },
    /* mac-address-table multicast <MAC Addrress> vlan <VID> */
    [CMD_CAM_MULTICAST_VLAN] = {
        .parent     = cli_cam + CMD_CAM_MULTICAST,
        .name       = "vlan",
        .handler    = NULL,
        .desc       = "Adds a static multicast entry in the filtering database",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<VID> VLAN number"
    },
    /* mac-address-table multicast <MAC Addrress> vlan <VID> port <port number> */
    [CMD_CAM_MULTICAST_VLAN_PORT] = {
        .parent     = cli_cam + CMD_CAM_MULTICAST_VLAN,
        .name       = "port",
        .handler    = cli_cmd_set_cam_multi_entry,
        .desc       = "Adds a static multicast entry in the filtering database",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<port number> port numbers separatted by commas"
    }
};

/**
 * \brief Init function for the command family 'mac-address-table'.
 * @param cli CLI interpreter.
 */
void cmd_mac_address_table_init(struct cli_shell *cli)
{
    int i;

    for (i = 0; i < NUM_CAM_CMDS; i++)
        cli_insert_command(cli, &cli_cam[i]);
}
