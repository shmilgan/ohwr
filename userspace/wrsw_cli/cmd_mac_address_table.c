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
    ports_to_mask(argv[2], mask);
    if (mask[0] == 'e') {
        printf("\tError. Ports must be decimal numbers separated by commas,\n");
        printf("\twith no blank spaces in between. Valid range: from 0 to %d\n",
                (NUM_PORTS-1));
        return;
    }
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

/**
 * \brief Registration function for the command family 'mac-address-table'.
 * @param cli CLI interpreter.
 */
void cmd_mac_address_table_init(struct cli_shell *cli)
{
    struct cli_cmd *c, *s, *m;

    c = cli_register_command(cli, NULL, "mac-address-table", NULL,
            "Configure MAC address table", CMD_NO_ARG, NULL);

    /* mac-address-table aging-time <aging> */
    cli_register_command(cli, c, "aging-time", cli_cmd_set_cam_aging,
        "Sets the MAC address table aging time",
        CMD_ARG_MANDATORY, "<aging value> New aging time");

    /* mac-address-table static <MAC Addrress> */
    s = cli_register_command(cli, c, "static", NULL,
            "Adds a static unicast entry in the filtering database",
            CMD_ARG_MANDATORY, "<MAC Addrress> MAC Address");
    /* mac-address-table static <MAC Addrress> vlan <VID> */
    m = cli_register_command(cli, s, "vlan", NULL,
            "Adds a static unicast entry in the filtering database",
            CMD_ARG_MANDATORY, "<VID> VLAN number");
    /* mac-address-table static <MAC Addrress> vlan <VID> port <port number> */
    cli_register_command(cli, m, "port", cli_cmd_set_cam_static_entry,
        "Adds a static unicast entry in the filtering database",
        CMD_ARG_MANDATORY, "<port number> Port numbers separatted by commas");

    /* mac-address-table multicast <MAC Addrress> */
    s = cli_register_command(cli, c, "multicast", NULL,
            "Adds a static multicast entry in the filtering database",
            CMD_ARG_MANDATORY, "<MAC Addrress> MAC Address");
    /* mac-address-table multicast <MAC Addrress> vlan <VID> */
    m = cli_register_command(cli, s, "vlan", NULL,
            "Adds a static multicast entry in the filtering database",
            CMD_ARG_MANDATORY, "<VID> VLAN number");
    /* mac-address-table multicast <MAC Addrress> vlan <VID> port <port number> */
    cli_register_command(cli, m, "port", cli_cmd_set_cam_multi_entry,
        "Adds a static multicast entry in the filtering database",
        CMD_ARG_MANDATORY, "<port number> port numbers separatted by commas");
}
