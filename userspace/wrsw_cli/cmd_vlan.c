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

enum vlan_cmds {
    CMD_VLAN = 0,
    CMD_VLAN_MEMBER,
    CMD_SHOW_VLAN,
    CMD_NO_VLAN,
    NUM_VLAN_CMDS
};

/**
 * \brief Command 'vlan <VID> member <Port number>'.
 * This command creates a new VLAN.
 * @param cli CLI interpreter.
 * @param valc number of arguments. Only two arguments allowed.
 * @param valv Two arguments must be specified: the VLAN number and the port
 * number.
 */
void cli_cmd_set_vlan(struct cli_shell *cli, int valc, char **valv)
{
    oid _oid[2][MAX_OID_LEN];
    char *base_oid = "1.3.111.2.802.1.1.4.1.4.3.1.7";
    size_t length_oid[2]; /* Base OID length */
    int vid;
    char ports[(2*NUM_PORTS)+1];
    char mask[NUM_PORTS];
    char types[2];
    char *value[2];
    int i;

    if (valc != 2) {
        printf("\tError. You have missed some command option\n");
        return;
    }

    /* Check the syntax of the vlan argument */
    if ((vid = cli_check_param(valv[0], VID_PARAM)) < 0)
        return;

    /* Parse port numbers to port mask and check the syntax */
    if (ports_to_mask(valv[1], mask) != 0)
        return;

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
 * \brief Command 'show vlan'.
 * This command displays the VLANs information.
 * @param cli CLI interpreter.
 * @param valc unused
 * @param valv unused
 */
void cli_cmd_show_vlan(struct cli_shell *cli, int valc, char **valv)
{
    oid _oid[MAX_OID_LEN];
    oid new_oid[MAX_OID_LEN];
    oid aux_oid[MAX_OID_LEN];
    char *base_oid = ".1.3.111.2.802.1.1.4.1.4.2.1.5";
    size_t length_oid;  /* Base OID length */
    char *ports = NULL;
    int ports_range[NUM_PORTS + 1];
    int vid;
    int fid;
    int i = 0;


    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid, &length_oid))
        return;

    /* We initialize the OIDs with the OID of the table */
    memcpy(new_oid, _oid, MAX_OID_LEN * sizeof(oid));

    /* Header */
    printf("\tVLAN   FID    Ports\n");
    printf("\t----   ---    --------------------------------\n");

    do {
        errno = 0;
        ports = cli_snmp_getnext_string(new_oid, &length_oid);
        if (errno != 0)
            break;
        if (cmp_oid(_oid, new_oid, 11) < 0)
            break;
        if (cmp_oid(_oid, new_oid, 11) > 0)
            break;

        vid = (int)new_oid[15];
        memcpy(aux_oid, new_oid, length_oid * sizeof(oid));
        aux_oid[12] = 4; /* FID column */

        errno = 0;
        fid = cli_snmp_get_int(aux_oid, length_oid);
        if (errno != 0)
            break;

        printf("\t%-4d   %-3d    ", vid, fid);

        /* Parse the port mask */
        memset(ports_range, 0, NUM_PORTS * sizeof(int));
        mask_to_ports(ports, ports_range);
        for (i = 0; ports_range[i] >= 0; i++) {
            printf("%d", ports_range[i]);
            if (ports_range[i + 1] >= 0)
                printf(", ");
            if ((i != 0) && ((i % 8) == 0))
                printf("\n\t              ");
        }
        printf("\n");

        memcpy(_oid, new_oid, sizeof(oid)*MAX_OID_LEN);
    } while(1);

    return;
}


/**
 * \brief Command 'no vlan <VID>'.
 * This command removes a VLAN.
 * @param cli CLI interpreter.
 * @param valc number of arguments. Only one argument allowed.
 * @param valv One argument must be specified: the VLAN number.
 */
void cli_cmd_del_vlan(struct cli_shell *cli, int valc, char **valv)
{
    oid _oid[MAX_OID_LEN];
    char *base_oid = "1.3.111.2.802.1.1.4.1.4.3.1.7";
    size_t length_oid; /* Base OID length */
    int vid;

    if (valc != 1) {
        printf("\tError. You have missed some command option\n");
        return;
    }

    /* Check the syntax of the vlan argument */
    if ((vid = cli_check_param(valv[0], VID_PARAM)) < 0)
        return;

    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid, &length_oid))
        return;

    /* Build the indexes */
    _oid[13] = 1;                /* Component ID column */
    _oid[14] = vid;              /* VID column */

    length_oid += 2;

    /* Row status (delete = 6) */
    cli_snmp_set_int(_oid, length_oid, "6", 'i');

    return;
}

/* Define the 'vlan' commands family */
struct cli_cmd cli_vlan[NUM_VLAN_CMDS] = {
    /* vlan <VID> */
    [CMD_VLAN] = {
        .parent     = NULL,
        .name       = "vlan",
        .handler    = NULL,
        .desc       = "VLAN configuration",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<VID> VLAN number"
    },
    /* vlan <VID> member <port number> */
    [CMD_VLAN_MEMBER] = {
        .parent     = cli_vlan + CMD_VLAN,
        .name       = "member",
        .handler    = cli_cmd_set_vlan,
        .desc       = "Creates a new VLAN",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<port number> port numbers separated by commas"
    },
    /* show vlan */
    [CMD_SHOW_VLAN] = {
        .parent     = &cli_show,
        .name       = "vlan",
        .handler    = cli_cmd_show_vlan,
        .desc       = "Displays VLAN information",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* no vlan <VID> */
    [CMD_NO_VLAN] = {
        .parent     = &cli_no,
        .name       = "vlan",
        .handler    = cli_cmd_del_vlan,
        .desc       = "Removes a VLAN",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<VID> VLAN number"
    }
};

/**
 * \brief Init function for the command family 'vlan'.
 * @param cli CLI interpreter.
 */
void cmd_vlan_init(struct cli_shell *cli)
{
    int i;

    for (i = 0; i < NUM_VLAN_CMDS; i++)
        cli_insert_command(cli, &cli_vlan[i]);
}
