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
#include <errno.h>

#include "cli_commands_utils.h"


struct cli_range {
    int min;
    int max;
    char *type;
};

/* Define allowed values for each parameter type */
static struct cli_range cli_ranges[NUM_OF_PARAM_TYPES] = {
    [PORT_PARAM]    = {0, NUM_PORTS - 1, "port"},
    [VID_PARAM]     = {DEFAULT_VID, WILDCARD_VID, "VID"},
    [AGING_PARAM]   = {MIN_AGING_TIME, MAX_AGING_TIME, "aging"},
    [SID_PARAM]     = {0, LC_MAX_SET_ID, "SID"},
    [FID_PARAM]     = {1, NUM_FIDS - 1, "FID"}
};

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
 * \brief Check the syntax for a given command argument.
 * This function checks that a given command argument (expressed as
 * a string) is in an allowed range, and if so it returns the argument as
 * an integer.
 * @param string command argument to be checked
 * @param type type of argument
 * @return The command argument expressed as an integer. -1 if it's an invalid
 * parameter or it's not in the allowed range.
*/
int cli_check_param(char *string, enum param_type type)
{
    int val;
    char *rest;
    struct cli_range *range = &cli_ranges[type];

    val = strtol(string, &rest, 10);
    /* check for errors */
    if (*rest) {
        printf("Invalid %s \"%s\": must be a decimal value\n",
               range->type, string);
        return -1;
    }

    if (val < range->min || val > range->max) {
        printf("Invalid %s \"%s\": valid range is %i to %i\n",
               range->type, string, range->min, range->max);
        return -1;
    }

    return val;
}

/**
 * \brief Helper function to convert mac address into a string.
 * @param mac MAC address expressed as an array of integers.
 * @param str The string buffer where the MAC address is to be stored.
 * @return A pointer to the string buffer representing the MAC address in the
 * format XX:XX:XX:XX:XX:XX.
*/
char *mac_to_str(uint8_t mac[ETH_ALEN], char str[3 * ETH_ALEN])
{
    snprintf(str, 3 * ETH_ALEN, "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    return str;
}

/**
 * \brief Helper function to convert mac address string into a mac address
 * format (array of integers).
 * @param str MAC address string.
 * @param mac MAC address expressed as an array of integers.
 * @return 0 on success. -1 on error.
*/
int str_to_mac(char *str, unsigned int mac[ETH_ALEN])
{
    if (sscanf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
        &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6) {
        printf("\tError: wrong MAC address format. Try: XX:XX:XX:XX:XX:XX,"
               " being 'X' an hexadecimal number\n");
        return -1;
    }
    return 0;
}

/**
 * \brief Helper function to compare OIDs.
 * @param old_oid Old OID.
 * @param new_oid New OID.
 * @param base_oid_length OIDs length to be compared.
 * @return 0 if they match. 1 or -1 if not.
*/
int cmp_oid(oid old_oid[MAX_OID_LEN], oid new_oid[MAX_OID_LEN],
    int base_oid_length)
{
    /* The OID for the table must be the same */
    if (memcmp(new_oid, old_oid, base_oid_length*sizeof(oid)) == 0) {
        if (memcmp(old_oid, new_oid, ((base_oid_length + 2)*sizeof(oid))) == 0)
            return 0; /* We are still in the same column */
        else
            return 1; /* We have changed the column */
    } else {
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
 * @param mask Port mask.
 * @param ports_range Array containing the port numbers corresponding to the
 * positions where the bits in the port mask are set to '1'.
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

    ports_range[j] = -1; /* This marks the end of the ports array */

    return;
}

/**
 * \brief Helper function to convert port numbers (which can be formatted as a
 * group of numbers separatted by commas) into port masks.
 * @param port_list A string containing the port list.
 * @param mask Port mask to be build from the port list.
 * @return 0 on success; a negative value otherwise.
*/
int ports_to_mask(char *port_list, char *mask)
{
    int i = 0;
    int len, port_num;
    char *rest;
    char *p = port_list;
    char *port_start;
    char port[3];

    memset(mask, '0', NUM_PORTS);

    for (i = 0; i < NUM_PORTS; i++) {
        len = 0;
        memset(port, 0, 3);

        if (!isdigit(*p) && (*p != ','))
            goto fail;

        while (*p == ',') /* Detect commas */
            p++;

        if (!*p)
            break;

        port_start = p;

        while (*p && isdigit(*p)) { /* Detect numbers */
            p++;
            len++;
        }

        if (len > 2)
            goto fail;

        memcpy(port, port_start, len);

        port_num = strtol(port, &rest, 10);

        if (*rest || port_num < 0 || port_num >= NUM_PORTS)
            goto fail;

        mask[port_num] = '1';

        /* Be sure that we are not yet at the end of the string */
        if (!*p)
            break;
    }

    return 0;

fail:
    printf("\tError. Ports must be decimal numbers separated by commas,\n"
           "\twith no blank spaces in between. Valid range: from 0 to %d\n",
           (NUM_PORTS-1));
    return -1;
}
