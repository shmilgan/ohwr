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

#ifndef __WHITERABBIT_RSTP_STMCH_H
#define __WHITERABBIT_RSTP_STMCH_H

/* State machines */
enum stmch_id {
    PIM = 0,
    PRS,
    PRT,
    PRX,
    PST,
    TCM,
    PPM,
    PTX,
    PTI,
    BDM
};

/* This is to keep track of state machines and their current states */
struct state_machine {
    struct port_data        *port;
    enum stmch_id           id;
    //enum stmch_state        state; /* TODO Define the states for each STMCH */
};


/* FUNCTIONS */
char *stmch_get_name(struct state_machine *stmch); /* For debug */

#endif /* __WHITERABBIT_RSTP_STMCH_H */
