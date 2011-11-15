/*
 * White Rabbit Switch RSTP
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizán (miguel.baizan@integrasys-sa.com)
 *
 * Description: State machines.
 *
 * Fixes:
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

#include "rstp_data.h"


/* Init State Machines data */
void init_stmchs_data(struct bridge_data *br)
{
    int i, j;

    br->stmch.bridge = br;
    br->stmch.port = NULL;

    /* Initialize the bridge's state machine */
    initialize_prs(&br->stmch);

    for (i = 0; i < MAX_NUM_PORTS ; i++) {
        for (j = 0; j < NUM_STMCH_PER_PORT; j++) {
            br->ports[i].stmch[j].bridge = br;
            br->ports[i].stmch[j].port = &br->ports[i];
        }
        /* Initialize the port's state machines */
        /* TODO
        initialize_pim(&br->ports[i].stmch[PIM]);
        initialize_prt(&br->ports[i].stmch[PRT]);
        initialize_prx(&br->ports[i].stmch[PRX]);
        initialize_pst(&br->ports[i].stmch[PST]);
        initialize_tcm(&br->ports[i].stmch[TCM]);
        initialize_ppm(&br->ports[i].stmch[PPM]);
        initialize_ptx(&br->ports[i].stmch[PTX]);
        initialize_pti(&br->ports[i].stmch[PTI]);
        initialize_bdm(&br->ports[i].stmch[BDM]);
        */
    }
}

/* Compute the transitions of all the STMCHs (called when a BPDU is received,
   or when the one second step has expired, or when a port link status has
   changed, etc) */
void stmch_compute_transitions(struct bridge_data *br)
{
    br->stmch.compute_transitions(&br->stmch);

    /* TODO compute stmchs transitions for ports that have PortEnabled = 1 */
#if 0
    int i, j;
    for (i = 0; i < MAX_NUM_PORTS ; i++) {
        for (j = 0; j < NUM_STMCH_PER_PORT; j++) {
            br->ports[i].stmch[j].compute_transitions(&br->ports[i].stmch[j]]);
        }
    }
#endif
}
