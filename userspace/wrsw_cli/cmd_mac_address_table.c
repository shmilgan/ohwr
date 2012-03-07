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

#include <rtu_fd_proxy.h>

#include "cli_commands.h"
#include "cli_commands_utils.h"

enum cam_cmds {
    CMD_CAM = 0,
    CMD_CAM_AGING,
    CMD_CAM_UNICAST,
    CMD_CAM_UNICAST_VLAN,
    CMD_CAM_UNICAST_VLAN_PORT,
    CMD_CAM_MULTICAST,
    CMD_CAM_MULTICAST_VLAN,
    CMD_CAM_MULTICAST_VLAN_PORT,
    CMD_SHOW_CAM,
    CMD_SHOW_CAM_UNICAST,
    CMD_SHOW_CAM_AGING_TIME,
    CMD_SHOW_CAM_MULTICAST,
    CMD_SHOW_CAM_STATIC,
    CMD_SHOW_CAM_STATIC_UNICAST,
    CMD_SHOW_CAM_STATIC_MULTICAST,
    CMD_NO_CAM,
    CMD_NO_CAM_UNICAST,
    CMD_NO_CAM_UNICAST_VLAN,
    CMD_NO_CAM_MULTICAST,
    CMD_NO_CAM_MULTICAST_VLAN,
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

    if (argc != 3) {
        printf("\tError. You have missed some command option\n");
        return;
    }

    /* Parse the MAC address */
    addr = argv[0];
    if (sscanf(addr, "%02x:%02x:%02x:%02x:%02x:%02x",
        &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6) {
        printf("\tError: wrong MAC address format. Try: XX:XX:XX:XX:XX:XX\n");
        return;
    }

    /* Check the syntax of the vlan argument */
    if (is_vid(argv[1]) < 0)
        return;

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
        printf("\t%-4d   %-17s      ", vid, mac_to_str(mac, mac_str));

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

/* Helper function to remove the static entries in the FDB (both
    unicast or multicast, depending on the OID passed as argument) */
static void del_cam_static_entry(int argc, char **argv, char *base_oid)
{
    oid _oid[MAX_OID_LEN];
    size_t length_oid; /* Base OID length */
    char *addr;
    unsigned int mac[ETH_ALEN];
    int vid;
    int i;

    if (argc != 2) {
        printf("\tError. You have missed some command option\n");
        return;
    }

    /* Parse the MAC address */
    addr = argv[0];
    if (sscanf(addr, "%02x:%02x:%02x:%02x:%02x:%02x",
        &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6) {
        printf("\tError: wrong MAC address format. Try: XX:XX:XX:XX:XX:XX\n");
        return;
    }

    /* Check the syntax of the vlan argument */
    if (is_vid(argv[1]) < 0)
        return;

    vid = atoi(argv[1]);

    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid, &length_oid))
        return;

    /* Build the indexes */
    _oid[13] = 1;                /* Component ID column */
    _oid[14] = vid;              /* VID column */
    for (i = 15; i < 21 ; i++)   /* MAC address columns */
        _oid[i] = mac[i-15];
    _oid[21] = 0;                /* Receive Port column */

    length_oid += 9;

    /* Row status (delete = 6) */
    cli_snmp_set_int(_oid, length_oid, "6", 'i');

    return;
}

/**
 * \brief Command 'mac-address-table aging-time <aging time>'.
 * This command sets a new aging time.
 * @param cli CLI interpreter.
 * @param argc number of arguments. Only one argument allowed.
 * @param agv new value for the aging time.
 */
void cli_cmd_set_cam_aging(struct cli_shell *cli, int argc, char **argv)
{
    oid _oid[MAX_OID_LEN];
    char *base_oid = "1.3.111.2.802.1.1.4.1.2.1.1.5.1.0";
    size_t length_oid;  /* Base OID length */

    if (!argc) {
        printf("\tError. You must specify the new aging value\n");
        return;
    }

    /* Check the syntax of the argument */
    if (is_aging(argv[0]) < 0)
        return;

    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid, &length_oid))
        return;

    cli_snmp_set_int(_oid, length_oid, argv[0], 'i');

    return;
}

/**
 * \brief Command 'mac-address-table unicast <MAC Addrress> vlan <VID>
 * port <port number>'.
 * This command creates a unicast static entry in the FDB.
 * @param cli CLI interpreter.
 * @param argc number of arguments. Only three arguments allowed.
 * @param agv Three arguments must be specified: the MAC Address, the VLAN
 * number and the port number.
 */
void cli_cmd_set_cam_uni_entry(struct cli_shell *cli, int argc, char **argv)
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
 * @param agv Three arguments must be specified: the MAC Address, the VLAN
 * number and the port number.
 */
void cli_cmd_set_cam_multi_entry(struct cli_shell *cli, int argc, char **argv)
{
    set_cam_static(argc, argv, ".1.3.111.2.802.1.1.4.1.3.2.1.6", 3);
    return;
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
    char *base_oid = ".1.3.111.2.802.1.1.4.1.2.1.1.5.1.0";
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
 * \brief Command 'show mac-address-table'.
 * This command shows general information on the Filtering Database
 * @param cli CLI interpreter.
 * @param argc unused
 * @param agv unused
 */
void cli_cmd_show_cam(struct cli_shell *cli, int argc, char **argv)
{
    int fdb_size;
    int fdb_num_static, fdb_num_dynamic;
    int vfdb_num_static, vfdb_num_dynamic;

    errno = 0;
    fdb_size = rtu_fdb_proxy_get_size();
    if (errno != 0)
        return;

    errno = 0;
    fdb_num_static = rtu_fdb_proxy_get_num_all_static_entries();
    if (errno != 0)
        return;

    errno = 0;
    fdb_num_dynamic = rtu_fdb_proxy_get_num_all_dynamic_entries();
    if (errno != 0)
        return;

    errno = 0;
    vfdb_num_static = rtu_vfdb_proxy_get_num_all_static_entries();
    if (errno != 0)
        return;

    errno = 0;
    vfdb_num_dynamic = rtu_vfdb_proxy_get_num_all_dynamic_entries();
    if (errno != 0)
        return;

    printf("\tFiltering Database Size:                      %d entries\n"
           "\tNumber of Static Filtering Entries:           %d\n"
           "\tNumber of Dynamic Filtering Entries:          %d\n"
           "\tNumber of Static VLAN Registration Entries:   %d\n"
           "\tNumber of Dynamic VLAN Registration Entries:  %d\n",
           fdb_size,
           fdb_num_static, fdb_num_dynamic,
           vfdb_num_static, vfdb_num_dynamic);
}

/**
 * \brief Command 'show mac-address-table unicast'.
 * This command shows the unicast entries in the FDB.
 * @param cli CLI interpreter.
 * @param argc unused
 * @param agv unused
 */
void cli_cmd_show_cam_uni(struct cli_shell *cli, int argc, char **argv)
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
        printf("\t%-3d   %-17s      ", fid, mac_to_str(mac, mac_str));

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
        printf("\t%-3d   %-17s      ", fid, mac_to_str(mac, mac_str));

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
 * \brief Command 'show mac-address-table static unicast'.
 * This command shows the unicast static entries in the FDB.
 * @param cli CLI interpreter.
 * @param argc unused
 * @param agv unused
 */
void cli_cmd_show_cam_static_uni(struct cli_shell *cli, int argc, char **argv)
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
 * \brief Command 'no mac-address-table unicast <MAC Addrress> vlan <VID>'.
 * This command deletes a unicast static entry in the FDB.
 * @param cli CLI interpreter.
 * @param argc number of arguments. Only two arguments allowed.
 * @param agv Two arguments must be specified: the MAC Address and the VLAN
 * number.
 */
void cli_cmd_del_cam_uni_entry(struct cli_shell *cli, int argc, char **argv)
{
    del_cam_static_entry(argc, argv, ".1.3.111.2.802.1.1.4.1.3.1.1.8");
    return;
}

/**
 * \brief Command 'no mac-address-table multicast <MAC Addrress> vlan <VID>'.
 * This command deletes a multicast static entry in the FDB.
 * @param cli CLI interpreter.
 * @param argc number of arguments. Only two arguments allowed.
 * @param agv Two arguments must be specified: the MAC Address and the VLAN
 * number.
 */
void cli_cmd_del_cam_multi_entry(struct cli_shell *cli, int argc, char **argv)
{
    del_cam_static_entry(argc, argv, ".1.3.111.2.802.1.1.4.1.3.2.1.6");
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
    /* mac-address-table unicast <MAC Addrress> */
    [CMD_CAM_UNICAST] = {
        .parent     = cli_cam + CMD_CAM,
        .name       = "unicast",
        .handler    = NULL,
        .desc       = "Adds a static unicast entry in the filtering database",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<MAC Addrress> MAC Address"
    },
    /* mac-address-table unicast <MAC Addrress> vlan <VID> */
    [CMD_CAM_UNICAST_VLAN] = {
        .parent     = cli_cam + CMD_CAM_UNICAST,
        .name       = "vlan",
        .handler    = NULL,
        .desc       = "Adds a static unicast entry in the filtering database",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<VID> VLAN number"
    },
    /* mac-address-table unicast <MAC Addrress> vlan <VID> port <port number> */
    [CMD_CAM_UNICAST_VLAN_PORT] = {
        .parent     = cli_cam + CMD_CAM_UNICAST_VLAN,
        .name       = "port",
        .handler    = cli_cmd_set_cam_uni_entry,
        .desc       = "Adds a static unicast entry in the filtering database",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<port number> Port numbers separated by commas"
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
        .opt_desc   = "<port number> Port numbers separated by commas"
    },
    /* show mac-address-table */
    [CMD_SHOW_CAM] = {
        .parent     = &cli_show,
        .name       = "mac-address-table",
        .handler    = cli_cmd_show_cam,
        .desc       = "Displays general information on the FDB",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* show mac-address-table unicast */
    [CMD_SHOW_CAM_UNICAST] = {
        .parent     = cli_cam + CMD_SHOW_CAM,
        .name       = "unicast",
        .handler    = cli_cmd_show_cam_uni,
        .desc       = "Displays static and dynamic information on unicast "
                      "entries in the FDB",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* show mac-address-table aging-time */
    [CMD_SHOW_CAM_AGING_TIME] = {
        .parent     = cli_cam + CMD_SHOW_CAM,
        .name       = "aging-time",
        .handler    = cli_cmd_show_cam_aging,
        .desc       = "Displays the Filtering Database aging time",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* show mac-address-table multicast */
    [CMD_SHOW_CAM_MULTICAST] = {
        .parent     = cli_cam + CMD_SHOW_CAM,
        .name       = "multicast",
        .handler    = cli_cmd_show_cam_multi,
        .desc       = "Displays static and dynamic information on multicast "
                      "entries in the FDB",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* show mac-address-table static */
    [CMD_SHOW_CAM_STATIC] = {
        .parent     = cli_cam + CMD_SHOW_CAM,
        .name       = "static",
        .handler    = NULL,
        .desc       = "Displays static MAC address entries information present"
                      " in the FDB",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* show mac-address-table static unicast */
    [CMD_SHOW_CAM_STATIC_UNICAST] = {
        .parent     = cli_cam + CMD_SHOW_CAM_STATIC,
        .name       = "unicast",
        .handler    = cli_cmd_show_cam_static_uni,
        .desc       = "Displays all the static unicast MAC address entries in"
                      " the FDB",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* show mac-address-table static multicast */
    [CMD_SHOW_CAM_STATIC_MULTICAST] = {
        .parent     = cli_cam + CMD_SHOW_CAM_STATIC,
        .name       = "multicast",
        .handler    = cli_cmd_show_cam_static_multi,
        .desc       = "Displays all the static multicast MAC address entries"
                      " in the FDB",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* no mac-address-table */
    [CMD_NO_CAM] = {
        .parent     = &cli_no,
        .name       = "mac-address-table",
        .handler    = NULL,
        .desc       = "Removes static entries from the filtering database",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* no mac-address-table unicast <MAC Addrress> */
    [CMD_NO_CAM_UNICAST] = {
        .parent     = cli_cam + CMD_NO_CAM,
        .name       = "unicast",
        .handler    = NULL,
        .desc       = "Removes a static unicast entry from the filtering"
                      " database",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<MAC Addrress> MAC Address"
    },
    /* no mac-address-table unicast <MAC Addrress> vlan <VID> */
    [CMD_NO_CAM_UNICAST_VLAN] = {
        .parent     = cli_cam + CMD_NO_CAM_UNICAST,
        .name       = "vlan",
        .handler    = cli_cmd_del_cam_uni_entry,
        .desc       = "Removes a static unicast entry from the filtering"
                      " database",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<VID> VLAN number"
    },
    /* no mac-address-table multicast <MAC Addrress> */
    [CMD_NO_CAM_MULTICAST] = {
        .parent     = cli_cam + CMD_NO_CAM,
        .name       = "multicast",
        .handler    = NULL,
        .desc       = "Removes a static multicast entry from the filtering"
                      " database",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<MAC Addrress> MAC Address"
    },
    /* no mac-address-table multicast <MAC Addrress> vlan <VID> */
    [CMD_NO_CAM_MULTICAST_VLAN] = {
        .parent     = cli_cam + CMD_NO_CAM_MULTICAST,
        .name       = "vlan",
        .handler    = cli_cmd_del_cam_multi_entry,
        .desc       = "Removes a static multicast entry from the filtering"
                      " database",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<VID> VLAN number"
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
