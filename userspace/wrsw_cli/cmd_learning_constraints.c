/*
 * White Rabbit Switch CLI (Command Line Interface)
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizan   (miguel.baizan@integrasys.es)
 *              Juan Luis Manas (juan.manas@integrasys.es)
 *
 * Description: Implementation of the commands family 'learning-constraints'.
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
#include <stdlib.h>

#include <rtu.h>

#include "cli_commands.h"
#include "cli_commands_utils.h"

enum lc_cmds {
    CMD_LC = 0,
    CMD_LC_SHARED,
    CMD_LC_SHARED_SID,
    CMD_LC_SHARED_SID_VLAN,
    CMD_LC_INDEPENDENT,
    CMD_LC_INDEPENDENT_SID,
    CMD_LC_INDEPENDENT_SID_VLAN,
    CMD_SHOW_LC,
    CMD_SHOW_LC_VLAN,
    CMD_NO_LC,
    CMD_NO_LC_SID,
    CMD_NO_LC_SID_VLAN,
    NUM_LC_CMDS
};

struct cli_lc_entry {
    int type;                   /* Type of LC */
    int num_vlans;              /* Number of VLANs for this Set ID */
    uint16_t vid[NUM_VLANS];    /* List of VLANs in this Set ID */
};

/* Internal LC table. It will be used to re-order the values got through SNMP,
   so we can get LC table data indexed by Set ID */
struct cli_lc_entry lc_table[NUM_LC_SETS] = {{0}};  /* LC table */

static void reset_lc_table(void)
{
    int i;

    for (i = 0; i < NUM_LC_SETS; i++) {
        lc_table[i].num_vlans = 0;
        lc_table[i].type = LC_UNDEFINED;
    }
}

/* Build an internal Learning Constraints table to present te data as described
   in Std. IEEE 802.1Q-2005. It returns -1 if an allocation error occurred. */
static int cli_read_lc_table(void)
{
    oid _oid[MAX_OID_LEN];
    oid new_oid[MAX_OID_LEN];
    char *base_oid = "1.3.111.2.802.1.1.4.1.4.8.1.4";
    size_t length_oid; /* Base OID length */
    int lc_type, set_id, vid;

    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid, &length_oid))
        return 0;

    /* We initialize the OIDs with the OID of the table */
    memcpy(new_oid, _oid, MAX_OID_LEN * sizeof(oid));

    do {
        errno = 0;
        lc_type = cli_snmp_getnext_int(new_oid, &length_oid);
        if (errno != 0)
            break;
        if (cmp_oid(_oid, new_oid, 11) < 0)
            break;
        if (cmp_oid(_oid, new_oid, 11) > 0)
            break;

        vid = (int)new_oid[14];
        set_id = (int)new_oid[15];

        lc_table[set_id].type = lc_type;
        lc_table[set_id].vid[lc_table[set_id].num_vlans] = vid;
        lc_table[set_id].num_vlans++;

        memcpy(_oid, new_oid, MAX_OID_LEN * sizeof(oid));
    } while(1);
    return 0;
}

/* Creates a new entry in the VLAN Learning Constraints table */
static void set_lc(int valc, char **valv, int type)
{
    oid _oid[2][MAX_OID_LEN];
    char *base_oid = "1.3.111.2.802.1.1.4.1.4.8.1";
    size_t length_oid[2];  /* Base OID length */
    char types[2];
    char *value[2];
    int sid, vid;

    if (valc != 2) {
        printf("\tError. You have missed some command option\n");
        return;
    }

    /* Check the syntax of the set-id argument */
    if ((sid = cli_check_param(valv[0], SID_PARAM)) < 0)
        return;

    /* Check the syntax of the vid argument */
    if ((vid = cli_check_param(valv[1], VID_PARAM)) < 0)
        return;

    memset(_oid[0], 0 , MAX_OID_LEN * sizeof(oid));
    memset(_oid[1], 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid[0] = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid[0], &length_oid[0]))
        return;

    /* Build the indexes */
    _oid[0][12] = 5;                /* RowStatus column */
    _oid[0][13] = 1;                /* Component ID column */
    _oid[0][14] = vid;              /* VID column */
    _oid[0][15] = sid;              /* Set ID column */

    length_oid[0] += 4;
    memcpy(_oid[1], _oid[0], length_oid[0] * sizeof(oid));
    length_oid[1] = length_oid[0];

    /* Fill with data. Remember that prior to the creation of a new entry,
       we must set the Row Status column */
    _oid[1][12] = 4;                /* LearningConstraintsType column */
    value[0] = "4";                 /* Row status (create = 4) */
    value[1] = ((type == LC_INDEPENDENT) ? "1" : "2");    /* VLAN LC type */
    types[0] = 'i';                 /* Type integer */
    types[1] = 'i';                 /* Type integer */

    cli_snmp_set(_oid, length_oid, value, types, 2);
    return;
}

/**
 * \brief Command 'learning-constraints shared set-id <SID> vlan <vid>'.
 * This command sets a Shared VLAN Learning Constraint rule for a given VID.
 * @param cli CLI interpreter.
 * @param valc number of arguments. Only two arguments allowed.
 * @param valv Two arguments must be specified: the Set ID and the VID.
 */
void cli_cmd_lc_shared(struct cli_shell *cli, int valc, char **valv)
{
    set_lc(valc, valv, LC_SHARED);
    return;
}

/**
 * \brief Command 'learning-constraints independent set-id <SID> vlan <vid>'.
 * This command sets an Independent VLAN Learning Constraint rule for a given
 * VID.
 * @param cli CLI interpreter.
 * @param valc number of arguments. Only two arguments allowed.
 * @param valv Two arguments must be specified: the set ID and the VID.
 */
void cli_cmd_lc_independent(struct cli_shell *cli, int valc, char **valv)
{
    set_lc(valc, valv, LC_INDEPENDENT);
    return;
}

/**
 * \brief Command 'show learning-constraints'.
 * This command displays the contents in the VLAN Learning Constraints table.
 * @param cli CLI interpreter.
 * @param valc unused.
 * @param valv unused.
 */
void cli_cmd_show_lc(struct cli_shell *cli, int valc, char **valv)
{
    int i, j;

    /* Build the internal LC table */
    if (cli_read_lc_table() != 0) {
        printf("An error occurred while reading the Learning Constraints "
               "table\n");
        return;
    }

    /* Header */
    printf("\tSet ID   Type          VID\n"
           "\t------   -----------   --------------------\n");

    for (i = 0; i < NUM_LC_SETS; i++) {
        if (lc_table[i].type != LC_UNDEFINED) {
            printf("\t%-6d   %-11s   ", i,
                   (lc_table[i].type == LC_INDEPENDENT) ?
                   "Independent" : "Shared");
            for (j = 0; j < lc_table[i].num_vlans; j++) {
                printf("%d", lc_table[i].vid[j]);
                if (j < (lc_table[i].num_vlans -1)) {
                    printf(", ");
                    if ((j != 0) && (j % 3 == 0))
                        printf("\n\t                       ");
                }
            }
            printf("\n");
        }
    }

    /* Reset the internal LC table for the next reading */
    reset_lc_table();

    return;
}

/**
 * \brief Command 'show learning-constraints vlan <VID>'.
 * This command displays the VLAN Learning Constraints for a given VID.
 * @param cli CLI interpreter.
 * @param valc number of arguments. Only one argument allowed.
 * @param valv One argument must be specified: the VLAN number.
 */
void cli_cmd_show_lc_vlan(struct cli_shell *cli, int valc, char **valv)
{
    oid _oid[MAX_OID_LEN];
    oid new_oid[MAX_OID_LEN];
    char *base_oid = "1.3.111.2.802.1.1.4.1.4.8.1.4";
    size_t length_oid; /* Base OID length */
    int vid, set_id, lc_type;

    if (valc != 1) {
        printf("\tError. You have missed some argument\n");
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

    /* We initialize the OIDs with the OID of the table */
    memcpy(new_oid, _oid, MAX_OID_LEN * sizeof(oid));

    /* Header */
    printf("\tSet ID   Type       \n"
           "\t------   -----------\n");

    do {
        errno = 0;
        lc_type = cli_snmp_getnext_int(new_oid, &length_oid);
        if (errno != 0)
            break;
        if (cmp_oid(_oid, new_oid, 11) < 0)
            break;
        if (cmp_oid(_oid, new_oid, 11) > 0)
            break;

        if (new_oid[14] == vid) {
            set_id = new_oid[15];
            printf("\t%-6d   %-11s\n",
                   set_id,
                   (lc_type == LC_INDEPENDENT) ? "Independent" : "Shared");
        }

        memcpy(_oid, new_oid, MAX_OID_LEN * sizeof(oid));
    } while(1);

    return;
}

/**
 * \brief Command 'no learning-constraints set-id <SID> vlan <VID>'.
 * This command removes an entry in the VLAN Learning Constraints table.
 * @param cli CLI interpreter.
 * @param valc number of arguments. Only two arguments allowed.
 * @param valv Two arguments must be specified: the Set ID and the VID.
 */
void cli_cmd_no_lc(struct cli_shell *cli, int valc, char **valv)
{
    oid _oid[MAX_OID_LEN];
    char *base_oid = "1.3.111.2.802.1.1.4.1.4.8.1.5";
    size_t length_oid;  /* Base OID length */
    int sid, vid;

    if (valc != 2) {
        printf("\tError. You have missed some command option\n");
        return;
    }

    /* Check the syntax of the set-id argument */
    if ((sid = cli_check_param(valv[0], SID_PARAM)) < 0)
        return;

    /* Check the syntax of the vid argument */
    if ((vid = cli_check_param(valv[1], VID_PARAM)) < 0)
        return;

    memset(_oid, 0 , MAX_OID_LEN * sizeof(oid));

    /* Parse the base_oid string to an oid array type */
    length_oid = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid, _oid, &length_oid))
        return;

    /* Build the indexes */
    _oid[13] = 1;               /* Component ID column */
    _oid[14] = vid;             /* VID column */
    _oid[15] = sid;             /* Set ID column */

    length_oid += 3;

    /* Row status (delete = 6) */
    cli_snmp_set_int(_oid, length_oid, "6", 'i');
    return;
}

/* Define the 'learning-constraints' commands family */
struct cli_cmd cli_lc[NUM_LC_CMDS] = {
    /* learning-constraints */
    [CMD_LC] = {
        .parent     = NULL,
        .name       = "learning-constraints",
        .handler    = NULL,
        .desc       = "Learning Constraints configuration",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* learning-constraints shared */
    [CMD_LC_SHARED] = {
        .parent     = cli_lc + CMD_LC,
        .name       = "shared",
        .handler    = NULL,
        .desc       = "Set a Shared VLAN Learning Constraint rule",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* learning-constraints shared set-id <SID> */
    [CMD_LC_SHARED_SID] = {
        .parent     = cli_lc + CMD_LC_SHARED,
        .name       = "set-id",
        .handler    = NULL,
        .desc       = "Set a Shared VLAN Learning Constraint rule for a given"
                      " Set ID",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<SID> VLAN Learning Constraints Set Identifier"
    },
    /* learning-constraints shared set-id <SID> vlan <VID> */
    [CMD_LC_SHARED_SID_VLAN] = {
        .parent     = cli_lc + CMD_LC_SHARED_SID,
        .name       = "vlan",
        .handler    = cli_cmd_lc_shared,
        .desc       = "Set a Shared VLAN Learning Constraint rule for a given"
                      " Set Id and VLAN",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<VID> VLAN number"
    },
    /* learning-constraints independent */
    [CMD_LC_INDEPENDENT] = {
        .parent     = cli_lc + CMD_LC,
        .name       = "independent",
        .handler    = NULL,
        .desc       = "Set an Independent VLAN Learning Constraint rule",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* learning-constraints independent set-id <SID> */
    [CMD_LC_INDEPENDENT_SID] = {
        .parent     = cli_lc + CMD_LC_INDEPENDENT,
        .name       = "set-id",
        .handler    = NULL,
        .desc       = "Set an Independent VLAN Learning Constraint rule for a "
                      "given Set ID",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<SID> VLAN Learning Constraints Set Identifier"
    },
    /* learning-constraints independent set-id <SID> vlan <VID> */
    [CMD_LC_INDEPENDENT_SID_VLAN] = {
        .parent     = cli_lc + CMD_LC_INDEPENDENT_SID,
        .name       = "vlan",
        .handler    = cli_cmd_lc_independent,
        .desc       = "Set an Independent VLAN Learning Constraint rule for a "
                      "given Set ID and VLAN",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<VID> VLAN number"
    },
    /* show learning-constraints */
    [CMD_SHOW_LC] = {
        .parent     = &cli_show,
        .name       = "learning-constraints",
        .handler    = cli_cmd_show_lc,
        .desc       = "Display the contents in the VLAN Learning Constraints"
                      " table",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* show learning-constraints vlan <VID> */
    [CMD_SHOW_LC_VLAN] = {
        .parent     = cli_lc + CMD_SHOW_LC,
        .name       = "vlan",
        .handler    = cli_cmd_show_lc_vlan,
        .desc       = "Display the VLAN Learning Constraints for a given VID",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<VID> VLAN number"
    },
    /* no learning-constraints */
    [CMD_NO_LC] = {
        .parent     = &cli_no,
        .name       = "learning-constraints",
        .handler    = NULL,
        .desc       = "Remove a VLAN Learning Constraints rule",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* no learning-constraints set-id <SID> */
    [CMD_NO_LC_SID] = {
        .parent     = cli_lc + CMD_NO_LC,
        .name       = "set-id",
        .handler    = NULL,
        .desc       = "Remove a VLAN Learning Constraints rule for a given Set"
                      " ID",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<SID> VLAN Learning Constraints Set Identifier"
    },
    /* no learning-constraints set-id <SID> vlan <VID> */
    [CMD_NO_LC_SID_VLAN] = {
        .parent     = cli_lc + CMD_NO_LC_SID,
        .name       = "vlan",
        .handler    = cli_cmd_no_lc,
        .desc       = "Remove a VLAN Learning Constraint rule for a given Set"
                      " ID and VLAN",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<VID> VLAN number"
    }
};

/**
 * \brief Init function for the command family 'learning-constraints'.
 * @param cli CLI interpreter.
 */
void cmd_lc_init(struct cli_shell *cli)
{
    int i;

    for (i = 0; i < NUM_LC_CMDS; i++)
        cli_insert_command(cli, &cli_lc[i]);
}
