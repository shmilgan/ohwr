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


#include "rstp_stmch.h"


/* This is for debug */
char *stmch_get_name(struct state_machine *stmch)
{
    char *stmch_names[] = {"Port Information", "Port Role Selection",
                           "Port Role Transitions", "Port Receive",
                           "Port State Transitions", "Topology Change",
                           "Port Protocol Migration", "Port Transmit",
                           "Port Timers", "Bridge Detection"};

    return stmch_names[stmch->id];
}
