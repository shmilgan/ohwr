/*
 * White Rabbit Switch CLI (Command Line Interface)
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizan   (miguel.baizan@integrasys.es)
 *              Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: Commands related stuff.
 *              Building of the commands tree.
 *              Implementation of the functions that handle each command.
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <linux/if_ether.h>

#include "cli_commands.h"
#include "cli_snmp.h"

/*
 * NOTE:
 * The commands tree is built as a linked list of subcommands, starting with a
 * root command, whose reference is stored in the cli_shell structure
 * in order to keep a track of the whole list. Each subcommand may have a
 * parent, a children and a reference to the next command with shared parent.
 * Depending on the previous parent commands, each subcommand has a different
 * handler. This is a graphical example of a commands tree:
 *
 * +--------------+
 * | root command |
 * +--------------+
 *        |
 *        |
 * +--------------+
 * | next command |
 * +--------------+
 *        |
 *        |
 * +--------------+         +---------------+        +--------------+
 * | next command |-------->| child command |------->| child command|
 * +--------------+         +---------------+        +--------------+
 *        |                         |
 *        |                         |
 * +--------------+         +--------------+
 * | next command |         | next command |
 * +--------------+         +--------------+
 *
 *
 */

#define NUM_PORTS 32


/* Help functions */
static void print_oid(oid _oid[MAX_OID_LEN], int n);
static char *mac_to_string(uint8_t mac[ETH_ALEN]);
static int cmp_oid(oid old_oid[MAX_OID_LEN], oid new_oid[MAX_OID_LEN],
    int base_oid_length);
static void ports_to_mask(char *ports_range, char *mask);
static void mask_to_ports(char *mask, int *ports_range);
static void show_cam_static(char *base_oid);
static void set_cam_static(int argc, char **argv, char *base_oid,
    int egress_ports_column);
static void del_cam_static_entry(int argc, char **argv, char *base_oid);


/**
 * \brief Defines the commands tree.
 * The commands tree is built as a linked list of commands, starting with a
 * root command, whose reference is stored in the cli_shell structure
 * in order to keep a track of the whole list. Each command may have a
 * parent, a children and a reference to the next command with shared parent.
 * Here you can add new commands for registration in the commands tree. Remember
 * also that you have to implement the handler for the commands you create.
 * @param cli CLI interpreter.
 */
void cli_build_commands_tree(struct cli_shell *cli)
{
    struct cli_cmd *c, *s, *m, *no;

    /* exit */
    c = cli_register_command(cli, NULL, "exit", cli_cmd_exit,
            "Exit from Command Line Interface", 0, NULL);

    /* Set the root parent command from which the tree will be build */
    cli->root_cmd = c;

    /* hostname */
    cli_register_command(cli, NULL, "hostname", cli_cmd_hostname,
        "Displays or sets (if argument is present) the host name",
        1, "<hostname> Optional. New hostname");


    /* SHOW commands */
    s = cli_register_command(cli, NULL, "show", NULL,
        "Shows the device configurations", 0, NULL);

    /* show interface */
    c = cli_register_command(cli, s, "interface", NULL,
            "Displays interface information", 0, NULL);
    /* show interface information */
    cli_register_command(cli, c, "information", cli_cmd_show_port_info,
            "Displays general interface information", 0, NULL);

    /* show mac-address-table */
    c = cli_register_command(cli, s, "mac-address-table", cli_cmd_show_cam,
            "Displays static and dynamic information of unicast entries"
            " in the FDB", 0, NULL);
    /* show mac-address-table aging-time */
    cli_register_command(cli, c, "aging-time", cli_cmd_show_cam_aging,
        "Displays the Filtering Database aging time", 0, NULL);
    /* show mac-address-table multicast */
    cli_register_command(cli, c, "multicast", cli_cmd_show_cam_multi,
        "Displays static and dynamic information of multicast entries"
        " in the FDB", 0, NULL);
    /* show mac-address-table static */
    m = cli_register_command(cli, c, "static", cli_cmd_show_cam_static,
        "Displays all the static unicast MAC address entries"
        " in the FDB", 0, NULL);
    /* show mac-address-table static multicast */
    cli_register_command(cli, m, "multicast", cli_cmd_show_cam_static_multi,
        "Displays all the static multicast MAC address entries"
        " in the FDB", 0, NULL);

    /* show vlan */
    c = cli_register_command(cli, s, "vlan", cli_cmd_show_vlan,
            "Displays VLAN information", 0, NULL);

    /* INTERFACE commands */
    c = cli_register_command(cli, NULL, "interface", NULL,
            "Interface configuration", 0, NULL);
    /* interface port <port number> */
    s = cli_register_command(cli, c, "port", NULL,
        "Port configuration", 1, "<port number> Port Number");
    /* interface port <port number> pvid <VLAN number> */
    cli_register_command(cli, s, "pvid", cli_cmd_set_port_pvid,
        "Sets the PVID value for the port", 1, "<VLAN number> VLAN Number");

    /* mac-address-table */
    c = cli_register_command(cli, NULL, "mac-address-table", NULL,
            "Configure MAC address table", 0, NULL);
    /* mac-address-table aging-time <aging> */
    cli_register_command(cli, c, "aging-time", cli_cmd_set_cam_aging,
        "Sets the MAC address table aging time",
        1, "<aging value> New aging time");
    /* mac-address-table static <MAC Addrress> */
    s = cli_register_command(cli, c, "static", NULL,
            "Adds a static unicast entry in the filtering database",
            1, "<MAC Addrress> MAC Address");
    /* mac-address-table static <MAC Addrress> vlan <VID> */
    m = cli_register_command(cli, s, "vlan", NULL,
            "Adds a static unicast entry in the filtering database",
            1, "<VID> VLAN number");
    /* mac-address-table static <MAC Addrress> vlan <VID> port <port number> */
    cli_register_command(cli, m, "port", cli_cmd_set_cam_static_entry,
        "Adds a static unicast entry in the filtering database",
        1, "<port number> Port numbers separatted by commas");
    /* mac-address-table multicast <MAC Addrress> */
    s = cli_register_command(cli, c, "multicast", NULL,
            "Adds a static multicast entry in the filtering database",
            1, "<MAC Addrress> MAC Address");
    /* mac-address-table multicast <MAC Addrress> vlan <VID> */
    m = cli_register_command(cli, s, "vlan", NULL,
            "Adds a static multicast entry in the filtering database",
            1, "<VID> VLAN number");
    /* mac-address-table multicast <MAC Addrress> vlan <VID> port <port number> */
    cli_register_command(cli, m, "port", cli_cmd_set_cam_multi_entry,
        "Adds a static multicast entry in the filtering database",
        1, "<port number> port numbers separatted by commas");

    /* VLAN commands */
    c = cli_register_command(cli, NULL, "vlan", NULL, "VLAN configuration",
            1, "<VID> VLAN number");
    /* vlan <VID> member <port number> */
    cli_register_command(cli, c, "member", cli_cmd_set_vlan,
        "Creates a new VLAN", 1,
        "<port number> port numbers separatted by commas");

    /* NO commands */
    no = cli_register_command(cli, NULL, "no", NULL,
        "Use 'no' to delete/disable some configured parameters", 0, NULL);
    /* no mac-address-table */
    c = cli_register_command(cli, no, "mac-address-table", NULL,
            "Removes static entries from the filtering database", 0, NULL);
    /* no mac-address-table static <MAC Addrress> */
    s = cli_register_command(cli, c, "static", NULL,
            "Removes a static unicast entry from the filtering database",
            1, "<MAC Addrress> MAC Address");
    /* no mac-address-table static <MAC Addrress> vlan <VID> */
    cli_register_command(cli, s, "vlan", cli_cmd_del_cam_static_entry,
            "Removes a static unicast entry from the filtering database",
            1, "<VID> VLAN number");
    /* no mac-address-table multicast <MAC Addrress> */
    s = cli_register_command(cli, c, "multicast", NULL,
            "Removes a static multicast entry from the filtering database",
            1, "<MAC Addrress> MAC Address");
    /* no mac-address-table multicast <MAC Addrress> vlan <VID> */
    cli_register_command(cli, s, "vlan", cli_cmd_del_cam_multi_entry,
            "Removes a static multicast entry from the filtering database",
            1, "<VID> VLAN number");

    /* no vlan <VID> */
    cli_register_command(cli, no, "vlan", cli_cmd_del_vlan, "Removes VLAN",
            1, "<VID> VLAN number");
}

/**
 * \brief Register a command in the tree.
 * It inserts a command in the command tree.
 * @param cli CLI interpreter.
 * @param parent pointer to the parent command. If it has no parent, then this
 *        parameter is NULL.
 * @param handler pointer to the function that handles the command. It can be
 *        NULL if the command has no meaning itself, but it has meaning as a
 *        parent of other command(s).
 * @param desc description of the command. This is printed when help is
 *        requested.
 * @return pointer to the new created command. It can be NULL if any error
 *         occurs.
 */
struct cli_cmd *cli_register_command(
    struct cli_shell *cli, struct cli_cmd *parent, char *command,
    void (*handler)(struct cli_shell *, int, char **),char *desc,
    int opt, char *opt_desc)
{
    struct cli_cmd *cmd, *c;

    /* Allocate the structure for the command */
    cmd = (struct cli_cmd *)malloc(sizeof(struct cli_cmd));
    if (!cmd)
        cli_error(cli, CLI_REG_ERROR);

    /* Clear structure */
    memset(cmd, 0, sizeof(struct cli_cmd));

    /* Set the parent command */
    cmd->parent = parent;

    /* Set the command name */
    if (!(cmd->name = strdup(command)))
        cli_error(cli, CLI_REG_ERROR);

    /* Set the handler function */
    cmd->handler = handler;

    /* Set the description of the command */
    if (desc)
        cmd->desc = strdup(desc);

    /* Set the options if they exists */
    if (opt) {
        cmd->opt = opt;
        cmd->opt_desc = strdup(opt_desc);
    } else {
        cmd->opt = 0;
        cmd->opt_desc = NULL;
    }

    /* Init the pointer to the next command. This'll be the last in the list */
    cmd->next = NULL;

    /* Insert the command in the tree */
    if (parent) {
        /* Move to the parent and add the new command to the list of children */
        if (!parent->child) {
            parent->child = cmd;
        } else { /* Traverse the list */
            c = parent->child;
            while (c) {
                if (c->next) {
                    c = c->next;
                } else {
                    c->next = cmd;
                    break;
                }
            }
        }
    } else { /* The new command has no parent */
        c = cli->root_cmd;
        while (c) {
            if (c->next) {
                c = c->next;
            } else {
                c->next = cmd;
                break;
            }
        }
    }

    return cmd;
}

/**
 * \brief Unregister commands to free memory.
 * It starts freeing the remotest subcommands in the tree by making nested
 * callings to the function when a child is found. If we see our commands
 * tree like a real tree, it first deallocates the leaves, then the branches,
 * then the trunk and finally the root.
 * @param cli CLI interpreter.
 * @param top_cmd The top command to start freeing memory.
 */
void cli_unregister_commands(struct cli_cmd *top_cmd)
{
    struct cli_cmd *c, *p, *free_cmd;

    if (!top_cmd)
        return;

    for (c = top_cmd; c;) {
        p = c->next;
        /* Unregister first children */
        while (c->child) {
            free_cmd = c->child;
            /* Reappoint the child pointer to the next command, in order to
               not loose the parent's reference to children */
            if (c->child->next)
                c->child = c->child->next;
            cli_unregister_commands(free_cmd);
        }

        if (c->name)
            free(c->name);
        if (c->desc)
            free(c->desc);
        if (c->opt_desc)
            free(c->opt_desc);
        if (c) {
            if (!c->child && !c->next)
                /* Set the parent's reference to 0 */
                if (c->parent)
                    c->parent->child = NULL;
            free(c);
        }

        /* Take the next pointer */
        c = p;
    }
}


/********************************************************************/
/*************************** COMMANDS *******************************/
/********************************************************************/

/**
 * \brief Command 'help'.
 * The 'help' command is the only one not registered in the tree. It has a
 * special treatment since it can be any command's child.
 * @param cli CLI interpreter.
 * @param cmd command for which help has been requested. If NULL, general help
 *        is printed.
 */
void cli_cmd_help(struct cli_shell *cli, struct cli_cmd *cmd)
{
    struct cli_cmd *c;

    printf("\n");

    /* If not command specified, print general help */
    if (!cmd) {
        printf("\tAvailable commands\n");
        printf("\t------------------\n\n");
        printf("\t?                    Display help\n");
        printf("\thelp                 Display help\n");
        for (c = cli->root_cmd; c; c = c->next)
            printf("\t%-20s %s\n", c->name, c->desc);
    } else {
        printf("\tCommand DESCRIPTION\n");
        printf("\t-------------------\n");
        printf("\t%s\n", cmd->desc);
        if (cmd->opt) {
            printf("\n\tThis command accepts arguments:\n");
            printf("\t%s\n", cmd->opt_desc);
        }
        if (cmd->child) {
            printf("\n\tSUBCOMMANDS\n");
            for (c = cmd->child; c; c = c->next)
                printf("\t  %-20s %s\n", c->name, c->desc);
        }
    }

    printf("\n");
}

/**
 * \brief Command 'exit'.
 * The execution of this command handles also the memory de-allocation of all
 * the commmand and CLI structures, so it is used also to exit the application.
 * @param cli CLI interpreter.
 * @param argc unused.
 * @param agv unused.
 */
void cli_cmd_exit(struct cli_shell *cli, int argc, char **argv)
{
    /* Free the memory of the registered commands */
    cli_unregister_commands(cli->root_cmd);

    /* Close SNMP */
    cli_snmp_close();

    /* Free cli shell structure */
    if (cli->hostname)
        free(cli->hostname);
    if (cli->prompt)
        free(cli->prompt);
    if (cli)
        free(cli);

    printf("GOODBYE!\n\n");
    exit(0);
}

/**
 * \brief Command 'hostname'.
 * This command shows the name of the host. If an argument is provided, this
 * command changes the name of the host to the new name passed as argument.
 * @param cli CLI interpreter.
 * @param argc number of arguments (only 0 or 1 are allowed).
 * @param agv if used, this is meant to be the new name for the host.
 */
void cli_cmd_hostname(struct cli_shell *cli, int argc, char **argv)
{
    /* Check if this command has been called with arguments */
    if (argc) {
        free(cli->hostname);
        cli->hostname = strdup(argv[0]);
        cli_build_prompt(cli);
        printf("\tThe new name for the host is '%s'\n", cli->hostname);
    } else {
        printf("\tHostname is '%s'\n", cli->hostname);
    }

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
    } else {
        printf("Error: %d\n", errno);
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
        if (errno == 0) {
            if (cmp_oid(_oid, new_oid, 11) < 0)
                break;
            if (cmp_oid(_oid, new_oid, 11) > 0)
                break;
            port = (int)new_oid[14];
            printf("\t%-4d   %d\n", port, pvid);
        } else {
            printf("Error: %d\n", errno);
            break;
        }
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
        if (errno == 0) {
            if (cmp_oid(_oid, new_oid, 11) < 0)
                break;
            if (cmp_oid(_oid, new_oid, 11) > 0)
                break;
            fid = (int)new_oid[14];
            for (i = 0; i < ETH_ALEN; i++){
                mac[i] = (int) new_oid[15+i];
            }
            printf("\t%-3d   %-17s      ", fid, mac_to_string(mac));

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
        } else {
            printf("Error: %d\n", errno);
            break;
        }
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
        if (errno == 0) {
            if (cmp_oid(_oid, new_oid, 11) < 0)
                break;
            if (cmp_oid(_oid, new_oid, 11) > 0)
                break;
            fid = (int)new_oid[14];
            for (i = 0; i < ETH_ALEN; i++){
                mac[i] = (int) new_oid[15+i];
            }
            printf("\t%-3d   %-17s      ", fid, mac_to_string(mac));

            /* Parse the port mask */
            memset(ports_range, 0, sizeof(ports_range));
            mask_to_ports(egress_ports, ports_range);
            for (i = 0; ports_range[i] >= 0 && i < NUM_PORTS; i++) {
                printf("%d", ports_range[i]);
                if (ports_range[i + 1] >= 0)
                    printf(", ");
                if ((i != 0) && ((i % 8) == 0))
                    printf("\n\t                             ");
            }
            printf("\n");
        } else {
            printf("Error: %d\n", errno);
            break;
        }
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
        if (errno == 0) {
            if (cmp_oid(_oid, new_oid, 11) < 0)
                break;
            if (cmp_oid(_oid, new_oid, 11) > 0)
                break;

            vid = (int)new_oid[15];

            memcpy(aux_oid, new_oid, length_oid * sizeof(oid));
            aux_oid[12] = 4; /* FID column */
            errno = 0;
            fid = cli_snmp_get_int(aux_oid, length_oid);
            if (errno != 0) {
                printf("Error: %d\n", errno);
                break;
            }
            printf("\t%-4d   %-3d    ", vid, fid);

            /* Parse the port mask */
            memset(ports_range, 0, sizeof(ports_range));
            mask_to_ports(ports, ports_range);
            for (i = 0; ports_range[i] >= 0 && i < NUM_PORTS; i++) {
                printf("%d", ports_range[i]);
                if (ports_range[i + 1] >= 0)
                    printf(", ");
                if ((i != 0) && ((i % 8) == 0))
                    printf("\n\t              ");
            }
            printf("\n");
        } else {
            printf("Error: %d\n", errno);
            break;
        }
        memcpy(_oid, new_oid, sizeof(oid)*MAX_OID_LEN);
    } while(1);

    return;
}


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

    errno = 0;
    cli_snmp_set_int(_oid, length_oid, argv[1], 'i');

    if (errno != 0)
        printf("Error: %d\n", errno);

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

    errno = 0;
    cli_snmp_set_int(_oid, length_oid, argv[0], 'i');

    if (errno != 0)
        printf("Error: %d\n", errno);

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
 * \brief Command 'no mac-address-table static <MAC Addrress> vlan <VID>'.
 * This command deletes a unicast static entry in the FDB.
 * @param cli CLI interpreter.
 * @param argc number of arguments. Only two arguments allowed.
 * @param agv Two arguments must be specified: the MAC Address (formatted as
 * XX:XX:XX:XX:XX:XX) and the VLAN number (a decimal number between 0 and
 * MAX_VID+1).
 */
void cli_cmd_del_cam_static_entry(struct cli_shell *cli, int argc, char **argv)
{
    del_cam_static_entry(argc, argv, ".1.3.111.2.802.1.1.4.1.3.1.1.8");
    return;
}

/**
 * \brief Command 'no mac-address-table multicast <MAC Addrress> vlan <VID>'.
 * This command deletes a multicast static entry in the FDB.
 * @param cli CLI interpreter.
 * @param argc number of arguments. Only two arguments allowed.
 * @param agv Two arguments must be specified: the MAC Address (formatted as
 * XX:XX:XX:XX:XX:XX) and the VLAN number (a decimal number between 0 and
 * MAX_VID+1).
 */
void cli_cmd_del_cam_multi_entry(struct cli_shell *cli, int argc, char **argv)
{
    del_cam_static_entry(argc, argv, ".1.3.111.2.802.1.1.4.1.3.2.1.6");
    return;
}

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

    errno = 0;
    cli_snmp_set(_oid, length_oid, value, types, 2);
    if (errno != 0)
        printf("Error: %d\n", errno);

    return;
}

/**
 * \brief Command 'no vlan <VID>'.
 * This command removes a VLAN.
 * @param cli CLI interpreter.
 * @param argc number of arguments. Only one argument allowed.
 * @param agv One argument must be specified: the VLAN number (a decimal
 * number between 0 and MAX_VID+1).
 */
void cli_cmd_del_vlan(struct cli_shell *cli, int argc, char **argv)
{
    oid _oid[MAX_OID_LEN];
    char *base_oid = "1.3.111.2.802.1.1.4.1.4.3.1.7";
    size_t length_oid; /* Base OID length */
    int vid;
    int i;


    /* Check that we have the three arguments */
    if (argc != 1) {
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

    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid, &length_oid))
        return;

    /* Build the indexes */
    _oid[13] = 1;                /* Component ID column */
    _oid[14] = vid;              /* VID column */

    length_oid += 2;

    errno = 0;
    /* Row status (delete = 6) */
    cli_snmp_set_int(_oid, length_oid, "6", 'i');

    if (errno != 0)
        printf("Error: %d\n", errno);

    return;
}


/******************** Help functions ******************************/

/* Helper function for debugg */
static void print_oid(oid _oid[MAX_OID_LEN], int n)
{
    int i;
    for (i = 0; i < n; i++)
        printf(".%d", (int)_oid[i]);
}

/* Helper function to convert mac address into a string*/
static char *mac_to_string(uint8_t mac[ETH_ALEN])
{
 	char str[40];
    snprintf(str, 40, "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    return strdup(str); //FIXME: can't be static but this takes memory
}

/* Helper function to compare OIDs */
static int cmp_oid(oid old_oid[MAX_OID_LEN], oid new_oid[MAX_OID_LEN],
    int base_oid_length)
{
    /* The OID for the table must be the same */
    if (memcmp(new_oid, old_oid, base_oid_length*sizeof(oid)) == 0) {
        if (memcmp(old_oid, new_oid, ((base_oid_length + 2)*sizeof(oid))) == 0) {
            return 0; /* We are still in the same column */
        } else {
            return 1; /* We have changed the column */
        }
    }
    else {
        return -1; /* We have changed the table */
    }

}

/* Helper function to convert port numbers (which can be formatted as a
   group of numbers separatted by commas) in port masks */
static void ports_to_mask(char *ports_range, char *mask)
{
    int i = 0;
    int len = 0;
    int num_ports = 0;
    char *port_start;
    char *port = ports_range;
    char *ports[NUM_PORTS];

    memset(mask, '0', (NUM_PORTS+1));

    for (i = 0; i < NUM_PORTS; i++) {
        if (!isdigit(*port) && (*port != ',')) {
            mask[0] = 'e';
            return; /* Only decimal numbers or commas accepted */
        }

        while (*port == ',') /* Detect commas */
            port++;

        port_start = port;

        while (*port && isdigit(*port)) { /* Detect numbers */
            port++;
            len++;
        }
        if (len > 2) {
            mask[0] = 'e';
            return; /* A port should not have more than two digits */
        }

        /* Allocate memory for the ports array */
        ports[i] = (char*)malloc(len + 1);
        if (!ports[i]) {
            mask[0] = 'e';
            return;
        }

        /* Copy the detected command to the array */
        memcpy(ports[i], port_start, len);
        ports[i][len] = 0;

        if (atoi(ports[i]) < 0 || atoi(ports[i]) >= NUM_PORTS) {
            mask[0] = 'e';
            return; /* Valid range: from 0 to NUM_PORTS-1 */
        }

        num_ports++;

        /* Clean for the next loop */
        len = 0;

        /* Be sure that we are not yet at the end of the string */
        if (!*port)
            break;
    }

    /* Create the mask */
    for (i = 0; i < num_ports ; i++)
        mask[atoi(ports[i])] = '1';

    mask[NUM_PORTS] = '\0';

    for (i = 0; i < num_ports ; i++)
        free(ports[i]);

    return;
}

/* Helper function to convert port masks into port numbers */
static void mask_to_ports(char *mask, int *ports_range)
{
    int i, j = 0;

    for (i = 0; i < NUM_PORTS; i++) {
        if (mask[i] == 1) {
            ports_range[j] = i;
            j++;
        }
    }

    if (j < NUM_PORTS)
        ports_range[j] = -1; /* This marks the end of the ports array */

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
        if (errno == 0) {
            if (cmp_oid(_oid, new_oid, 11) < 0)
                break;
            if (cmp_oid(_oid, new_oid, 11) > 0)
                break;
            vid = (int)new_oid[14];
            for (i = 0; i < ETH_ALEN; i++){
                mac[i] = (int) new_oid[15+i];
            }
            printf("\t%-4d   %-17s      ", vid, mac_to_string(mac));

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
        } else {
            printf("Error: %d\n", errno);
            break;
        }
        memcpy(_oid, new_oid, sizeof(oid)*MAX_OID_LEN);
    } while(1);
}

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

    errno = 0;
    cli_snmp_set(_oid, length_oid, value, types, 2);
    if (errno != 0)
        printf("Error: %d\n", errno);
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


    /* Check that we have the two arguments */
    if (argc != 2) {
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

    errno = 0;
    /* Row status (delete = 6) */
    cli_snmp_set_int(_oid, length_oid, "6", 'i');

    if (errno != 0)
        printf("Error: %d\n", errno);
}
