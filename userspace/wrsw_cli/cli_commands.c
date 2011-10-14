/*
 * White Rabbit Switch CLI (Command Line Interface)
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizan   (miguel.baizan@integrasys.es)
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
    struct cli_cmd *c, *s;

    /* exit */
    c = cli_register_command(cli, NULL, "exit", cli_cmd_exit,
            "Exit from Command Line Interface", 0, NULL);

    /* Set the root parent command from which the tree will be build */
    cli->root_cmd = c;

    /* hostname */
    cli_register_command(cli, NULL, "hostname", cli_cmd_hostname,
        "Displays or sets the host name", 1, "New hostname");


    /* SHOW commands */
    s = cli_register_command(cli, NULL, "show", NULL,
        "Shows the device configurations", 0, NULL);


    /* show mac-address-table */
    c = cli_register_command(cli, s, "mac-address-table", cli_cmd_show_cam,
            "Dispalys unicast entries in the Filtering Database", 0, NULL);
    /* show mac-address-table aging-time */
    cli_register_command(cli, c, "aging-time", cli_cmd_show_cam_aging,
        "Displays the Filtering Database aging time", 0, NULL);
    /* show mac-address-table multicast */
    cli_register_command(cli, c, "multicast", cli_cmd_show_cam_multi,
        "Displays all multicast MAC address entries in the Filtering Database",
        0, NULL);
    /* show mac-address-table static */
    cli_register_command(cli, c, "static", cli_cmd_show_cam_static,
        "Displays all the static MAC address entries in the Filtering Database",
        0, NULL);
    /* show mac-address-table dynamic */
    cli_register_command(cli, c, "dynamic", cli_cmd_show_cam_dynamic,
        "Displays all dynamic MAC address entries in the Filtering Database",
        0, NULL);

    /* show vlan */
    c = cli_register_command(cli, s, "vlan", cli_cmd_show_vlan,
            "Displays the VLAN information", 1, "VLAN ID");

    /* mac-address-table */
    c = cli_register_command(cli, NULL, "mac-address-table", NULL,
            "Configure MAC address table", 0, NULL);
    /* mac-address-table aging-time */
    cli_register_command(cli, c, "aging-time", cli_cmd_set_cam_aging,
        "Sets the MAC address table aging time", 1, "New aging time");

    /* If something has gone wrong during registration, exit the application */
    if (cli->error < 0)
        cli_print_error(cli);
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
    int (*handler)(struct cli_shell *, int, char **),char *desc,
    int opt, char *opt_desc)
{
    struct cli_cmd *cmd, *c;

    /* Allocate the structure for the command */
    cmd = (struct cli_cmd *)malloc(sizeof(struct cli_cmd));
    if (!cmd) {
        cli->error = CLI_ALLOC_ERROR;
        return cmd;
    }

    /* Clear structure */
    memset(cmd, 0, sizeof(struct cli_cmd));

    /* Set the parent command */
    cmd->parent = parent;

    /* Set the command name */
    if (!(cmd->name = strdup(command)))
        cli->error = CLI_ALLOC_ERROR;

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
            printf("\t<opt>\t%s\n", cmd->opt_desc);
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
int cli_cmd_exit(struct cli_shell *cli, int argc, char **argv)
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
 * This commands shows the name of the host. If an argument is provided, this
 * command changes the name of the host to the new name passed as argument.
 * @param cli CLI interpreter.
 * @param argc number of arguments (only 0 or 1 are allowed).
 * @param agv if used, this is meant to be the new name for the host.
 */
int cli_cmd_hostname(struct cli_shell *cli, int argc, char **argv)
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

    return CLI_OK;
}


/**
 * \brief Command 'show mac-address-table aging-time'.
 * This commands shows the aging time.
 * @param cli CLI interpreter.
 * @param argc unused
 * @param agv unused
 */
int cli_cmd_show_cam_aging(struct cli_shell *cli, int argc, char **argv)
{
    oid _oid[MAX_OID_LEN];
    size_t length_oid;  /* Base OID length */
    int aging;

    errno = 0;

    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid("1.3.111.2.802.1.1.4.1.2.1.1.5.1.0", _oid, &length_oid)) {
        return 0;
    }

    aging = cli_snmp_get_int(_oid, length_oid);

    if (errno == 0) {
        printf("\tAging time: %d\n", aging);
    } else {
        printf("Error\n");
    }

    return CLI_OK;
}

/* For debugg */
static void print_oid(oid _oid[MAX_OID_LEN], int n)
{
    int i;
    for (i = 0; i < n; i++)
        printf(".%d", (int)_oid[i]);
}

static char *mac_to_string(uint8_t mac[ETH_ALEN])
{
 	char str[40];
    snprintf(str, 40, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    return strdup(str); //FIXME: can't be static but this takes memory
}

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

static void show_cam(char *base_oid)
{
    oid _oid[MAX_OID_LEN];
    oid new_oid[MAX_OID_LEN];
    size_t length_oid;  /* Base OID length */
    char *egress_ports = NULL;
    int i;
    int vid;
    uint8_t mac[ETH_ALEN];


    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid, &length_oid)) {
        return;
    }

    /* We initialize the OIDs with the OID of the table */
    memcpy(new_oid, _oid, MAX_OID_LEN * sizeof(oid));

    /* Header */
    printf("\tVLAN      MAC Address         Ports\n");
    printf("\t----   -----------------      -----\n");

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
            printf("\t%d   %s    ", vid, mac_to_string(mac));
            for (i = 0; i < NUM_PORTS; i++) {
                printf("%d", egress_ports[i]);
            }
            printf("\n");
        } else {
            printf("Error\n");
            break;
        }
        memcpy(_oid, new_oid, sizeof(oid)*MAX_OID_LEN);
    } while(1);
}


/**
 * \brief Command 'show mac-address-table'.
 * This commands shows the unicast static entries in the FDB.
 * @param cli CLI interpreter.
 * @param argc unused
 * @param agv unused
 */
int cli_cmd_show_cam(struct cli_shell *cli, int argc, char **argv)
{
    show_cam(".1.3.111.2.802.1.1.4.1.3.1.1.5");
    return CLI_OK;
}


/**
 * \brief Command 'show mac-address-table multicast'.
 * This commands shows the multicast static entries in the FDB.
 * @param cli CLI interpreter.
 * @param argc unused
 * @param agv unused
 */
int cli_cmd_show_cam_multi(struct cli_shell *cli, int argc, char **argv)
{
    show_cam(".1.3.111.2.802.1.1.4.1.3.2.1.3");
    return CLI_OK;
}


/*************** TODO **************************/
int cli_cmd_show_cam_static(struct cli_shell *cli, int argc, char **argv)
{
    printf("show_cam_static function called\n");
    return CLI_OK;
}

int cli_cmd_show_cam_dynamic(struct cli_shell *cli, int argc, char **argv)
{
    printf("show_cam_dynamic function called\n");
    return CLI_OK;
}

int cli_cmd_show_vlan(struct cli_shell *cli, int argc, char **argv)
{
    printf("show_vlan function called\n");
    return CLI_OK;
}

int cli_cmd_set_cam_aging(struct cli_shell *cli, int argc, char **argv)
{
    if (!argc) {
        printf("\tYou must specify the new aging value\n");
        return CLI_OK;
    }

    printf("set_cam_aging function called\n");
    return CLI_OK;
}
