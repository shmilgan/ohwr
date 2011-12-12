/*
 * White Rabbit Switch RSTP
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizán (miguel.baizan@integrasys-sa.com)
 *
 * Description: Port Role Selection state machine. Maintained per bridge.
 *              See Std 802.1D-2004, clause 17.28.
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

#include <string.h>

#include "rstp_data.h"


/* Port Role Selection states */
enum states {
    INIT_BRIDGE = 0,
    ROLE_SELECTION
};


/* 17.21.24 */
static void updtRoleDisabledTree(struct bridge_data *br)
{
    int i;

    /* At startup all the ports are marked as DISABLED */
    for (i = 0; i < MAX_NUM_PORTS ; i++) {
        br->ports[i].selectedRole = DisabledPort;
    }

    TRACEV(TRACE_INFO, "PRS: ports role set to DISABLED");
}

/* 17.21.2 */
static void clearReselectTree(struct bridge_data *br)
{
    int i;

    for (i = 0; i < MAX_NUM_PORTS ; i++)
        br->ports[i].reselect = FALSE;

    TRACEV(TRACE_INFO, "PRS: RESELECT flag set to FALSE on every port");
}

/* 17.21.25 */
static void updtRolesTree(struct bridge_data *br)
{
    int i;
    int root_port_num = -1;
    struct st_priority_vector   rppv; /* root path priority vector */
    struct st_priority_vector   best_rppv = br->BridgePriority;
    struct rstp_times           best_times = br->BridgeTimes;
    struct port_data            *root_port;


    for (i = 0; i < MAX_NUM_PORTS ; i++) {
        /* Do not need to check the portEnabled flag; infoIs = Disabled when
           BEGIN = 1 */
        if (br->ports[i].infoIs == Received) {
            /* 17.21.25 a) */
            rppv = br->ports[i].portPriority;
            rppv.RootPathCost += br->ports[i].mng.PortPathCost;

            /* 17.21.25 b) */
            if (memcmp(&rppv.DesignatedBridgeId,
                       &br->BridgePriority.DesignatedBridgeId,
                      (ETH_ALEN + BRIDGE_PRIO_LEN) * sizeof(uint8_t)) &&
                (cmp_priority_vectors(&rppv, &best_rppv, 1) < 0)) {
                best_rppv = rppv;
                br->rootPortId = best_rppv.BridgePortId;
                root_port_num = i;
                TRACEV(TRACE_INFO, "PRS: the Bridge's root priority vector has "
                       "been updated");
            }
        }

        /* I use this in the next iteration */
        if (br->rootPortId == br->ports[i].portId)
            root_port = &br->ports[i];
    }
    br->rootPriority = best_rppv;   /* 17.21.25 b) */

    /* 17.21.25 c) */
    if (root_port_num >= 0) {
        best_times = br->ports[root_port_num].portTimes;
        best_times.message_age += ONE_SECOND;
        /* Round to nearest whole second */
        round_timer(best_times.message_age);
    }
    br->rootTimes = best_times;

    for (i = 0; i < MAX_NUM_PORTS ; i++) {
        /* 17.21.25 d) and e) */
        br->ports[i].designatedPriority = br->rootPriority;
        br->ports[i].designatedPriority.DesignatedBridgeId =
            br->mng.BridgeIdentifier;
        br->ports[i].designatedPriority.DesignatedPortId = br->ports[i].portId;
        br->ports[i].designatedPriority.BridgePortId = br->ports[i].portId;
        br->ports[i].designatedTimes = br->rootTimes;
        br->ports[i].designatedTimes.hello_time = br->BridgeTimes.hello_time;

        switch (br->ports[i].infoIs) {
        case Received:  /* 17.21.25 i), j), k), l) */
            if (root_port_num == i) {
                br->ports[i].selectedRole = RootPort;
                br->ports[i].updtInfo = FALSE;
                TRACEV(TRACE_INFO, "PRS: port %s is ROOT port",
                       br->ports[i].port_name);
                break;
            }
            if (cmp_priority_vectors(&br->ports[i].designatedPriority,
                    &br->ports[i].portPriority, 1) < 0) {
                /* Comparing the designated bridge id with the bridge id should
                   be enough to distinguish between backup and alternate ports
                   (17.3.1) */
                if (memcmp(&br->ports[i].portPriority.DesignatedBridgeId,
                           &br->mng.BridgeIdentifier,
                           (BRIDGE_PRIO_LEN + ETH_ALEN) * sizeof(uint8_t))) {
                    br->ports[i].selectedRole = AlternatePort;
                    TRACEV(TRACE_INFO, "PRS: port %s is ALTERNATE port",
                           br->ports[i].port_name);
                } else {
                    br->ports[i].selectedRole = BackupPort;
                    TRACEV(TRACE_INFO, "PRS: port %s is BACKUP port",
                           br->ports[i].port_name);
                }
                br->ports[i].updtInfo = FALSE;
                break;
            }
            br->ports[i].selectedRole = DesignatedPort;
            br->ports[i].updtInfo = TRUE;
            TRACEV(TRACE_INFO, "PRS: port %s is DESIGNATED port",
                   br->ports[i].port_name);
            break;
        case Aged:      /* 17.21.25 g) */
            br->ports[i].updtInfo = TRUE;
            br->ports[i].selectedRole = DesignatedPort;
            TRACEV(TRACE_INFO, "PRS: port %s is DESIGNATED port",
                   br->ports[i].port_name);
            break;
        case Mine:      /* 17.21.25 h) */
            if (cmp_priority_vectors(&br->ports[i].designatedPriority,
                                     &br->ports[i].portPriority, 1) ||
                cmp_times(&br->ports[i].portTimes, &root_port->portTimes)) {
                br->ports[i].updtInfo = TRUE;
            }
            br->ports[i].selectedRole = DesignatedPort;
            TRACEV(TRACE_INFO, "PRS: port %s is DESIGNATED port",
                   br->ports[i].port_name);
            break;
        case Disabled:  /* 17.21.25 f) */
            br->ports[i].selectedRole = DisabledPort;
            TRACEV(TRACE_INFO, "PRS: port %s is DISABLED port",
                   br->ports[i].port_name);
            break;
        }
    }

    TRACEV(TRACE_INFO, "PRS: port roles updated");
}

/* 17.21.16 */
static void setSelectedTree(struct bridge_data *br)
{
    int i;

    for (i = 0; i < MAX_NUM_PORTS ; i++) {
        if (br->ports[i].reselect == TRUE)
            return;
    }

    for (i = 0; i < MAX_NUM_PORTS ; i++)
        br->ports[i].selected = TRUE;

    TRACEV(TRACE_INFO, "PRS: SELECTED variable set to TRUE for all Ports");
}


/* Enter state */
static void enter_state(struct state_machine *stmch, enum states state)
{
    struct bridge_data *br = stmch->bridge;

    switch (state) {
    case INIT_BRIDGE:
        TRACEV(TRACE_INFO, "PRS: entering state INIT_BRIDGE");
        updtRoleDisabledTree(br);
        break;
    case ROLE_SELECTION:
        TRACEV(TRACE_INFO, "PRS: entering state ROLE_SELECTION");
        clearReselectTree(br);
        updtRolesTree(br);
        setSelectedTree(br);
        break;
    }

    stmch->state = state;
}

/* Compute transitions */
static void compute_transitions(struct state_machine *stmch)
{
    int i;
    struct bridge_data *br = stmch->bridge;

    TRACEV(TRACE_INFO, "Computing transitions for the Port Role Selection"
           " state machine");

    /* Test if the BEGIN flag is true */
    if (br->begin) {
        enter_state(stmch, INIT_BRIDGE);
        return;
    }

    switch (stmch->state) {
    case ROLE_SELECTION:
        for (i = 0; i < MAX_NUM_PORTS ; i++) {
            if (br->ports[i].reselect == TRUE) {
                enter_state(stmch, ROLE_SELECTION);
                break;
            }
        }
        break;
    case INIT_BRIDGE:
        enter_state(stmch, ROLE_SELECTION);
        break;
    }
}


/* Initialize the PRS state machines data */
void initialize_prs(struct state_machine *stmch)
{
    stmch->id = PRS;
    stmch->compute_transitions = compute_transitions;
}
