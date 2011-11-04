/*
 * White Rabbit Switch CLI (Command Line Interface)
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizan   (miguel.baizan@integrasys.es)
 *              Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: Miscellaneous helper functions for the command handlers.
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

#include "cli_commands_utils.h"


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
        printf("\tAvailable commands\n"
               "\t------------------\n\n"
               "\t?                    Display help\n"
               "\thelp                 Display help\n");
        for (c = cli->root_cmd; c; c = c->next)
            printf("\t%-20s %s\n", c->name, c->desc);
    } else {
        printf("\tCommand DESCRIPTION\n"
               "\t-------------------\n"
               "\t%s\n", cmd->desc);
        switch (cmd->opt) {
        case CMD_ARG_MANDATORY:
            printf("\n\tArguments that must be provided:\n"
                   "\t%s\n", cmd->opt_desc);
            break;
        case CMD_ARG_OPTIONAL:
            printf("\n\tOptional arguments:\n"
                   "\t%s\n", cmd->opt_desc);
            break;
        default:
            break;
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
 * \brief Helper function to convert mac address into a string
*/
char *mac_to_string(uint8_t mac[ETH_ALEN], char str[3 * ETH_ALEN])
{
    snprintf(str, 3 * ETH_ALEN, "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    return str;
}

/* Helper function to compare OIDs */
int cmp_oid(oid old_oid[MAX_OID_LEN], oid new_oid[MAX_OID_LEN],
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

/**
 * \brief Helper function for debugg
*/
void print_oid(oid _oid[MAX_OID_LEN], int n)
{
    int i;
    for (i = 0; i < n; i++)
        printf(".%d", (int)_oid[i]);
}

/**
 * \brief Helper function to convert port masks into port numbers
*/
void mask_to_ports(char *mask, int *ports_range)
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

/**
 * \brief Helper function to convert port numbers (which can be formatted as a
 * group of numbers separatted by commas) in port masks. The character 'e' is
 * used when an error occurs
*/
void ports_to_mask(char *ports_range, char *mask)
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
