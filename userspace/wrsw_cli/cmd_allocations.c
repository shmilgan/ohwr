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

#include <rtu_fd_proxy.h>

#include "cli_commands.h"
#include "cli_commands_utils.h"

enum allocations_cmds {
    CMD_ALLOCATIONS = 0,
    CMD_ALLOCATIONS_VLAN,
    CMD_ALLOCATIONS_VLAN_FID,
    CMD_SHOW_ALLOCATIONS,
    CMD_SHOW_ALLOCATIONS_VID,
    CMD_SHOW_ALLOCATIONS_FID,
    CMD_NO_ALLOCATIONS,
    CMD_NO_ALLOCATIONS_VID,
    NUM_ALLOCATIONS_CMDS
};

enum allocation_type {
    CMD_ALLOCATIONS_UNDEFINED   = 0,
    CMD_ALLOCATIONS_FIXED       = 1,
    CMD_ALLOCATIONS_DYNAMIC     = 2
};

/**
 * \brief Command 'allocations vlan <VID> fid <FID>'.
 * This command establishes a Fixed allocation of a VID to an FID.
 * @param cli CLI interpreter.
 * @param argc number of arguments. Only two arguments allowed.
 * @param agv Two arguments must be specified: the VID and the FID.
 */
void cli_cmd_allocation(struct cli_shell *cli, int argc, char **argv)
{
    int vid, fid;

    if (argc != 2) {
        printf("\tError. You have missed some command option\n");
        return;
    }

    /* Check the syntax of the vlan argument */
    if (is_vid(argv[0]) < 0)
        return;

    /* Check the syntax of the fid argument */
    if (is_fid(argv[1]) < 0)
        return;

    vid = atoi(argv[0]);
    fid = atoi(argv[1]);

    if (rtu_fdb_proxy_set_fid((uint16_t)vid, (uint8_t)fid) < 0)
        printf("\tOperation rejected.\n");
}

/**
 * \brief Command 'show allocations'.
 * This command displays the contents of the VID to FID allocation table.
 * @param cli CLI interpreter.
 * @param argc unused.
 * @param agv unused.
 */
void cli_cmd_show_allocation(struct cli_shell *cli, int argc, char **argv)
{
    uint16_t    vid = 0;
    uint8_t     fid;
    int         fid_fixed;
    int         ret = 0;

    printf("\tVID    Type        FID\n"
           "\t----   ---------   ---\n");

    /* Search starts from VID = 0 */
    rtu_fdb_proxy_read_fid(vid, &fid, &fid_fixed);

    /* Remember that for an implementation choice the FID = 0 is used for the
       type of allocation "Undefined", which will not be shown in the output */
    while (ret >= 0) {
        if (fid != 0)
            printf("\t%-4d   %-9s   %-3d\n",
                   vid,
                   (fid_fixed == CMD_ALLOCATIONS_FIXED) ? "Fixed" : "Dynamic",
                   fid);
        ret = rtu_fdb_proxy_read_next_fid(&vid, &fid, &fid_fixed);
    }
}

/**
 * \brief Command 'show allocations vlan <VID>'.
 * This command displays the FID to which a specified VID is currently
 * allocated.
 * @param cli CLI interpreter.
 * @param argc number of arguments. Only one argument allowed.
 * @param agv One argument must be specified: the VID.
 */
void cli_cmd_show_allocation_vlan(struct cli_shell *cli, int argc, char **argv)
{
    uint16_t    vid;
    uint8_t     fid;
    int         fid_fixed;

    if (argc != 1) {
        printf("\tError. You have missed some command option\n");
        return;
    }

    /* Check the syntax of the vlan argument */
    if (is_vid(argv[0]) < 0)
        return;

    vid = (uint16_t)atoi(argv[0]);

    rtu_fdb_proxy_read_fid(vid, &fid, &fid_fixed);

    printf("\tVID    Type        FID\n"
           "\t----   ---------   ---\n");
    printf("\t%-4d   %-9s   %-3d\n",
           vid,
           (fid == 0) ?
                "Undefined" :
                ((fid_fixed == CMD_ALLOCATIONS_FIXED) ? "Fixed" : "Dynamic"),
           fid);
}

/**
 * \brief Command 'show allocations fid <FID>'.
 * This command displays all VIDs currently allocated to a given FID.
 * @param cli CLI interpreter.
 * @param argc number of arguments. Only one argument allowed.
 * @param agv One argument must be specified: the FID.
 */
void cli_cmd_show_allocation_fid(struct cli_shell *cli, int argc, char **argv)
{
    uint16_t    vid = 0;
    uint8_t     fid;
    uint8_t     current_fid;
    int         fid_fixed;
    int         ret = 0;

    if (argc != 1) {
        printf("\tError. You have missed some command option\n");
        return;
    }

    /* Check the syntax of the fid argument */
    if (is_fid(argv[0]) < 0)
        return;

    fid = (uint8_t)atoi(argv[0]);

    printf("\tFID   Type        VID\n"
           "\t---   ---------   ----\n");

    /* Search starts from VID = 0 */
    rtu_fdb_proxy_read_fid(vid, &current_fid, &fid_fixed);

    while (ret >= 0) {
        if ((current_fid != 0) && (current_fid == fid))
            printf("\t%-3d   %-9s   %-4d\n",
                   fid,
                   (fid_fixed == CMD_ALLOCATIONS_FIXED) ? "Fixed" : "Dynamic",
                   vid);
        ret = rtu_fdb_proxy_read_next_fid(&vid, &current_fid, &fid_fixed);
    }
}

/**
 * \brief Command 'no allocations vlan <VID>'.
 * This command removes a fixed VID to FID allocation from the VID to FID
 * allocation table.
 * @param cli CLI interpreter.
 * @param argc number of arguments. Only one argument allowed.
 * @param agv One argument must be specified: the VID.
 */
void cli_cmd_no_allocation(struct cli_shell *cli, int argc, char **argv)
{
    uint16_t vid;

    if (argc != 1) {
        printf("\tError. You have missed some command option\n");
        return;
    }

    /* Check the syntax of the vlan argument */
    if (is_vid(argv[0]) < 0)
        return;

    vid = (uint16_t)atoi(argv[0]);

    if (rtu_fdb_proxy_delete_fid(vid) < 0)
        printf("\tOperation rejected.\n");
}

/* Define the 'allocations' commands family */
struct cli_cmd cli_allocations[NUM_ALLOCATIONS_CMDS] = {
    /* allocations */
    [CMD_ALLOCATIONS] = {
        .parent     = NULL,
        .name       = "allocations",
        .handler    = NULL,
        .desc       = "VID to FID table configuration",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* allocations vlan <VID> */
    [CMD_ALLOCATIONS_VLAN] = {
        .parent     = cli_allocations + CMD_ALLOCATIONS,
        .name       = "vlan",
        .handler    = NULL,
        .desc       = "Establish a fixed allocation of a VID to an FID",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<VID> VLAN number"
    },
    /* allocations vlan <VID> fid <FID> */
    [CMD_ALLOCATIONS_VLAN_FID] = {
        .parent     = cli_allocations + CMD_ALLOCATIONS_VLAN,
        .name       = "fid",
        .handler    = cli_cmd_allocation,
        .desc       = "Establish a Fixed allocation of a VID to an FID",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<FID> Filtering Database Identifier"
    },
    /* show allocations */
    [CMD_SHOW_ALLOCATIONS] = {
        .parent     = &cli_show,
        .name       = "allocations",
        .handler    = cli_cmd_show_allocation,
        .desc       = "Display the contents of the VID to FID allocation table",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* show allocations vlan <VID> */
    [CMD_SHOW_ALLOCATIONS_VID] = {
        .parent     = cli_allocations + CMD_SHOW_ALLOCATIONS,
        .name       = "vlan",
        .handler    = cli_cmd_show_allocation_vlan,
        .desc       = "Display the FID to which a specified VID is currently"
                      " allocated",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<VID> VLAN number"
    },
    /* show allocations fid <FID> */
    [CMD_SHOW_ALLOCATIONS_FID] = {
        .parent     = cli_allocations + CMD_SHOW_ALLOCATIONS,
        .name       = "fid",
        .handler    = cli_cmd_show_allocation_fid,
        .desc       = "Display all VIDs currently allocated to a given FID",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<FID> Filtering Database Identifier"
    },
    /* no allocations */
    [CMD_NO_ALLOCATIONS] = {
        .parent     = &cli_no,
        .name       = "allocations",
        .handler    = NULL,
        .desc       = "Remove a fixed VID to FID allocation from the VID to FID"
                      " allocation table",
        .opt        = CMD_NO_ARG,
        .opt_desc   = NULL
    },
    /* no allocations vlan <VID> */
    [CMD_NO_ALLOCATIONS_VID] = {
        .parent     = cli_allocations + CMD_NO_ALLOCATIONS,
        .name       = "vlan",
        .handler    = cli_cmd_no_allocation,
        .desc       = "Remove a fixed VID to FID allocation from the VID to FID"
                      " allocation table",
        .opt        = CMD_ARG_MANDATORY,
        .opt_desc   = "<VID> VLAN number"
    }
};

/**
 * \brief Init function for the command family 'allocations'.
 * @param cli CLI interpreter.
 */
void cmd_allocations_init(struct cli_shell *cli)
{
    int i;

    for (i = 0; i < NUM_ALLOCATIONS_CMDS; i++)
        cli_insert_command(cli, &cli_allocations[i]);
}
