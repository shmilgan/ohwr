/*
 * White Rabbit Switch CLI (Command Line Interface)
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizan   (miguel.baizan@integrasys.es)
 *              Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: Implementation of the commands family 'show'.
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
#include <errno.h>

#include "cli_commands.h"
#include "cli_commands_utils.h"

enum show_cmds {
    CMD_SHOW = 0,
    CMD_SHOW_INTERFACE,
    CMD_SHOW_INTERFACE_INFORMATION,
    CMD_SHOW_CAM,
    CMD_SHOW_CAM_AGING_TIME,
    CMD_SHOW_CAM_MULTICAST,
    CMD_SHOW_CAM_STATIC,
    CMD_SHOW_CAM_STATIC_MULTICAST,
    CMD_SHOW_VLAN,
    NUM_SHOW_CMDS
};

/* Helper function to get the static entries of the FDB (both unicast
   or multicast, depending on the OID passed as argument) */
static void show_cam_static(char *base_oid)
{
    oid _oid[MAX_OID_LEN];
    oid new_oid[MAX_OID_LEN];
    size_t length_oid;  /* Base OID length */
    char *egress_ports = NULL;
    int ports_range[NUM_PORTS];
    int i;
    int vid;
    uint8_t mac[ETH_ALEN];
    char mac_str[3 * ETH_ALEN];


    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid, &length_oid))
        return;

    /* We initialize the OIDs with the OID of the table */
    memcpy(new_oid, _oid, MAX_OID_LEN * sizeof(oid));

    /* Header */
    printf("\tVLAN      MAC Address         Ports\n");
    printf("\t----   -----------------      --------------------------------\n");

    do {
        errno = 0;
        egress_ports = cli_snmp_getnext_string(new_oid, &length_oid);
        if (errno != 0)
            break;
        if (cmp_oid(_oid, new_oid, 11) < 0)
            break;
        if (cmp_oid(_oid, new_oid, 11) > 0)
            break;

        vid = (int)new_oid[14];
        for (i = 0; i < ETH_ALEN; i++){
            mac[i] = (int) new_oid[15+i];
        }
        printf("\t%-4d   %-17s      ", vid, mac_to_string(mac, mac_str));

        /* Parse the port mask */
        memset(ports_range, 0, sizeof(ports_range));
        mask_to_ports(egress_ports, ports_range);
        for (i = 0; ports_range[i] >= 0 && i < NUM_PORTS; i++) {
            printf("%d", ports_range[i]);
            if (ports_range[i + 1] >= 0)
                printf(", ");
            if ((i != 0) && ((i % 8) == 0))
                printf("\n\t                              ");
        }
        printf("\n");

        memcpy(_oid, new_oid, sizeof(oid)*MAX_OID_LEN);
    } while(1);
}

/**
 * \brief Command 'show mac-address-table aging-time'.
 * This command shows the aging time.
 * @param cli CLI interpreter.
 * @param argc unused
 * @param agv unused
 */
void cli_cmd_show_cam_aging(struct cli_shell *cli, int argc, char **argv)
{
    oid _oid[MAX_OID_LEN];
    char *base_oid = "1.3.111.2.802.1.1.4.1.2.1.1.5.1.0";
    size_t length_oid;  /* Base OID length */
    int aging;


    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid, &length_oid))
        return;

    errno = 0;
    aging = cli_snmp_get_int(_oid, length_oid);

    if (errno == 0) {
        printf("\tAging time: %d\n", aging);
    }

    return;
}

/**
 * \brief Command 'show interface information'.
 * This command shows general information on ports.
 * @param cli CLI interpreter.
 * @param argc unused
 * @param agv unused
 */
void cli_cmd_show_port_info(struct cli_shell *cli, int argc, char **argv)
{
    oid _oid[MAX_OID_LEN];
    oid new_oid[MAX_OID_LEN];
    char *base_oid = ".1.3.111.2.802.1.1.4.1.4.5.1.1";
    size_t length_oid;  /* Base OID length */
    int pvid, port;


    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid, &length_oid))
        return;

    /* We initialize the OIDs with the OID of the table */
    memcpy(new_oid, _oid, MAX_OID_LEN * sizeof(oid));

    /* Header */
    printf("\tPort   PVID\n");
    printf("\t----   ----\n");

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
        printf("\t%-4d   %d\n", port, pvid);

        memcpy(_oid, new_oid, sizeof(oid) * MAX_OID_LEN);
    } while(1);

    return;
}

/**
 * \brief Command 'show mac-address-table'.
 * This command shows the unicast entries in the FDB.
 * @param cli CLI interpreter.
 * @param argc unused
 * @param agv unused
 */
void cli_cmd_show_cam(struct cli_shell *cli, int argc, char **argv)
{
    oid _oid[MAX_OID_LEN];
    oid new_oid[MAX_OID_LEN];
    char *base_oid = ".1.3.111.2.802.1.1.4.1.2.2.1.2";
    size_t length_oid;  /* Base OID length */
    int port;
    int i, j;
    int fid;
    uint8_t mac[ETH_ALEN];
    char mac_str[3 * ETH_ALEN];


    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid, &length_oid))
        return;

    /* We initialize the OIDs with the OID of the table */
    memcpy(new_oid, _oid, MAX_OID_LEN * sizeof(oid));

    /* Header */
    printf("\tFID      MAC Address         Ports\n");
    printf("\t---   -----------------      --------------------------------\n");

    do {
        errno = 0;
        port = cli_snmp_getnext_int(new_oid, &length_oid);
        if (errno != 0)
            break;
        if (cmp_oid(_oid, new_oid, 11) < 0)
            break;
        if (cmp_oid(_oid, new_oid, 11) > 0)
            break;

        fid = (int)new_oid[14];
        for (i = 0; i < ETH_ALEN; i++){
            mac[i] = (int) new_oid[15+i];
        }
        printf("\t%-3d   %-17s      ", fid, mac_to_string(mac, mac_str));

        /* Parse the port value */
        j = 0;
        for (i = 0; i < NUM_PORTS; i++) {
            if (port & (1 << i)) {
                if (j > 0) {
                    printf(", ");
                    if ((j % 8) == 0)
                        printf("\n\t                             ");
                }
                printf("%d", i);
                j++;
            }
        }
        printf("\n");

        memcpy(_oid, new_oid, sizeof(oid)*MAX_OID_LEN);
    } while(1);

    return;
}

/**
 * \brief Command 'show mac-address-table multicast'.
 * This command shows the multicast static entries in the FDB.
 * @param cli CLI interpreter.
 * @param argc unused
 * @param agv unused
 */
void cli_cmd_show_cam_multi(struct cli_shell *cli, int argc, char **argv)
{
    oid _oid[MAX_OID_LEN];
    oid new_oid[MAX_OID_LEN];
    char *base_oid = ".1.3.111.2.802.1.1.4.1.2.3.1.2";
    size_t length_oid;  /* Base OID length */
    char *egress_ports = NULL;
    int ports_range[NUM_PORTS];
    int i;
    int fid;
    uint8_t mac[ETH_ALEN];
    char mac_str[3 * ETH_ALEN];


    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid, &length_oid))
        return;

    /* We initialize the OIDs with the OID of the table */
    memcpy(new_oid, _oid, MAX_OID_LEN * sizeof(oid));

    /* Header */
    printf("\tFID      MAC Address         Ports\n");
    printf("\t---   -----------------      --------------------------------\n");

    do {
        errno = 0;
        egress_ports = cli_snmp_getnext_string(new_oid, &length_oid);
        if (errno != 0)
            break;
        if (cmp_oid(_oid, new_oid, 11) < 0)
            break;
        if (cmp_oid(_oid, new_oid, 11) > 0)
            break;

        fid = (int)new_oid[14];
        for (i = 0; i < ETH_ALEN; i++){
            mac[i] = (int) new_oid[15+i];
        }
        printf("\t%-3d   %-17s      ", fid, mac_to_string(mac, mac_str));

        /* Parse the port mask */
        memset(ports_range, 0, NUM_PORTS * sizeof(int));
        mask_to_ports(egress_ports, ports_range);
        for (i = 0; ports_range[i] >= 0 && i < NUM_PORTS; i++) {
            printf("%d", ports_range[i]);
            if (ports_range[i + 1] >= 0)
                printf(", ");
            if ((i != 0) && ((i % 8) == 0))
                printf("\n\t                             ");
        }
        printf("\n");

        memcpy(_oid, new_oid, sizeof(oid)*MAX_OID_LEN);
    } while(1);

    return;
}

/**
 * \brief Command 'show mac-address-table static'.
 * This command shows the unicast static entries in the FDB.
 * @param cli CLI interpreter.
 * @param argc unused
 * @param agv unused
 */
void cli_cmd_show_cam_static(struct cli_shell *cli, int argc, char **argv)
{
    show_cam_static(".1.3.111.2.802.1.1.4.1.3.1.1.5");
    return;
}

/**
 * \brief Command 'show mac-address-table static multicast'.
 * This command shows the multicast static entries in the FDB.
 * @param cli CLI interpreter.
 * @param argc unused
 * @param agv unused
 */
void cli_cmd_show_cam_static_multi(struct cli_shell *cli, int argc, char **argv)
{
    show_cam_static(".1.3.111.2.802.1.1.4.1.3.2.1.3");
    return;
}


/**
 * \brief Command 'show vlan'.
 * This command displays the VLANs information.
 * @param cli CLI interpreter.
 * @param argc unused
 * @param agv unused
 */
void cli_cmd_show_vlan(struct cli_shell *cli, int argc, char **argv)
{
    oid _oid[MAX_OID_LEN];
    oid new_oid[MAX_OID_LEN];
    oid aux_oid[MAX_OID_LEN];
    char *base_oid = ".1.3.111.2.802.1.1.4.1.4.2.1.5";
    size_t length_oid;  /* Base OID length */
    char *ports = NULL;
    int ports_range[NUM_PORTS];
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
        for (i = 0; ports_range[i] >= 0 && i < NUM_PORTS; i++) {
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

/* Define the 'show' commands family */
struct cli_cmd cli_show[NUM_SHOW_CMDS] = {
    /* show */
    [CMD_SHOW] = {
        .parent     = NULL,
        .name       = "show",
        .handler    = NULL,
        .desc       = "Shows the device configurations",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* show interface */
    [CMD_SHOW_INTERFACE] = {
        .parent     = cli_show + CMD_SHOW,
        .name       = "interface",
        .handler    = NULL,
        .desc       = "Displays interface information",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* show interface information */
    [CMD_SHOW_INTERFACE_INFORMATION] = {
        .parent     = cli_show + CMD_SHOW_INTERFACE,
        .name       = "information",
        .handler    = cli_cmd_show_port_info,
        .desc       = "Displays general interface information",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* show mac-address-table */
    [CMD_SHOW_CAM] = {
        .parent     = cli_show + CMD_SHOW,
        .name       = "mac-address-table",
        .handler    = cli_cmd_show_cam,
        .desc       = "Displays static and dynamic information of unicast "
                      "entries in the FDB",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* show mac-address-table aging-time */
    [CMD_SHOW_CAM_AGING_TIME] = {
        .parent     = cli_show + CMD_SHOW_CAM,
        .name       = "aging-time",
        .handler    = cli_cmd_show_cam_aging,
        .desc       = "Displays the Filtering Database aging time",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* show mac-address-table multicast */
    [CMD_SHOW_CAM_MULTICAST] = {
        .parent     = cli_show + CMD_SHOW_CAM,
        .name       = "multicast",
        .handler    = cli_cmd_show_cam_multi,
        .desc       = "Displays static and dynamic information of multicast "
                      "entries in the FDB",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* show mac-address-table static */
    [CMD_SHOW_CAM_STATIC] = {
        .parent     = cli_show + CMD_SHOW_CAM,
        .name       = "static",
        .handler    = cli_cmd_show_cam_static,
        .desc       = "Displays all the static unicast MAC address entries in"
                      " the FDB",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* show mac-address-table static multicast */
    [CMD_SHOW_CAM_STATIC_MULTICAST] = {
        .parent     = cli_show + CMD_SHOW_CAM_STATIC,
        .name       = "multicast",
        .handler    = cli_cmd_show_cam_static_multi,
        .desc       = "Displays all the static multicast MAC address entries"
                      " in the FDB",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* show vlan */
    [CMD_SHOW_VLAN] = {
        .parent     = cli_show + CMD_SHOW,
        .name       = "vlan",
        .handler    = cli_cmd_show_vlan,
        .desc       = "Displays VLAN information",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    }
};

/**
 * \brief Init function for the command family 'show'.
 * @param cli CLI interpreter.
 */
void cmd_show_init(struct cli_shell *cli)
{
    int i;

    for (i = 0; i < NUM_SHOW_CMDS; i++)
        cli_insert_command(cli, &cli_show[i]);
}
