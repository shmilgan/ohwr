/*
 * White Rabbit Switch RSTP
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizán (miguel.baizan@integrasys-sa.com)
 *
 * Description: Port Information state machine. Maintained per port.
 *              See Std 802.1D-2004, clause 17.27.
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
    DISABLED = 0,
    AGED,
    UPDATE,
    CURRENT,
    RECEIVE,
    SUPERIOR_DESIGNATED,
    REPEATED_DESIGNATED,
    INFERIOR_DESIGNATED,
    NOT_DESIGNATED,
    OTHER
};


/* 17.21.1 */
static int betterorsameInfo(struct port_data *port)
{
    if ((port->infoIs == Received &&
         cmp_priority_vectors(&port->msgPriority,
                              &port->portPriority, 1) <= 0) ||
        (port->infoIs == Mine &&
         cmp_priority_vectors(&port->designatedPriority,
                              &port->portPriority, 1) <= 0)) {
        return 1;
    }
    return 0;
}

/* 17.21.8 */
static enum received_info rcvInfo(struct port_data *port)
{
    int cmp_pv, bpdu_port_role;

    /* TODO: Decode BPDU. See also 9.3.4. */

    /* Check the port role encoded in the BPDU */
    bpdu_port_role = (port->bpdu.configuration_bpdu.flags & PORT_ROLE_FLAG) >>
                     PORT_ROLE_FLAG_OFFS;

    cmp_pv = cmp_priority_vectors(&port->msgPriority, &port->portPriority, 1);

    if (port->bpdu.configuration_bpdu.bpdu_type == BPDU_TYPE_CONFIG ||
        (port->bpdu.configuration_bpdu.bpdu_type == BPDU_TYPE_RSTP &&
         bpdu_port_role == PORT_ROLE_DESIGNATED)) {
        if (cmp_pv < 0) {   /* 17.21.8 a.1)*/
            return SuperiorDesignatedInfo;
        }
        if (cmp_pv == 0) {
            if (!cmp_times(&port->msgTimes, &port->portTimes))
                return RepeatedDesignatedInfo;  /* 17.21.8 b) */
            return SuperiorDesignatedInfo;  /* 17.21.8 a.2) */
        }
        if (cmp_pv > 0) {   /* 17.21.8 c) */
            return InferiorDesignatedInfo;
        }
    }

    if (port->bpdu.configuration_bpdu.bpdu_type == BPDU_TYPE_RSTP &&
        bpdu_port_role != PORT_ROLE_UNKNOWN) {
        if (cmp_pv >= 0)    /* 17.21.8 d) */
            return InferiorRootAlternateInfo;
    }

    return OtherInfo;
}

/* 17.21.11 */
static void recordProposal(struct port_data *port)
{
    int bpdu_port_role =
        (port->bpdu.configuration_bpdu.flags & PORT_ROLE_FLAG) >>
        PORT_ROLE_FLAG_OFFS;

    if (port->bpdu.configuration_bpdu.bpdu_type == BPDU_TYPE_CONFIG ||
        (port->bpdu.configuration_bpdu.bpdu_type == BPDU_TYPE_RSTP &&
         bpdu_port_role == PORT_ROLE_DESIGNATED)) {
        if ((port->bpdu.configuration_bpdu.flags & PROPOSAL_FLAG))
            set_port_flag(port->rstp_flags, proposed);
    }
}

/* 17.21.17 */
static void setTcFlags(struct port_data *port)
{
    switch (port->bpdu.configuration_bpdu.bpdu_type) {
    case BPDU_TYPE_CONFIG:
    case BPDU_TYPE_RSTP:
        if ((port->bpdu.configuration_bpdu.flags & TOPOLOGY_CHANGE_FLAG))
            set_port_flag(port->rstp_flags, rcvdTc);
        if ((port->bpdu.configuration_bpdu.flags & TOPOLOGY_CHANGE_ACK_FLAG))
            set_port_flag(port->rstp_flags, rcvdTcAck);
        break;
    case BPDU_TYPE_TCN:
        set_port_flag(port->rstp_flags, rcvdTcn);
        break;
    }
}

/* 17.21.12 */
static void recordPriority(struct port_data *port)
{
    port->portPriority = port->msgPriority;
}

/* 17.21.13 */
static void recordTimes(struct port_data *port)
{
    port->portTimes = port->msgTimes;

    if (port->msgTimes.hello_time <= MIN_BRIDGE_HELLO_TIME)
        port->portTimes.hello_time = MIN_BRIDGE_HELLO_TIME;
}

/* 17.21.23 */
static void updtRcvdInfoWhile(struct port_data *port)
{
    uint16_t temp_time;

    temp_time = port->portTimes.message_age + ONE_SECOND;
    round_timer(temp_time);

    port->rcvdInfoWhile = (temp_time < port->portTimes.max_age) ?
                          (3 * port->portTimes.hello_time) :
                          0;
}

/* 17.21.10 */
static void recordDispute(struct port_data *port)
{
    if (port->bpdu.configuration_bpdu.bpdu_type == BPDU_TYPE_RSTP &&
        (port->bpdu.configuration_bpdu.flags & LEARNING_FLAG)) {
        set_port_flag(port->rstp_flags, agreed);
        remove_port_flag(port->rstp_flags, proposing);
    }
}

/* 17.21.9 */
static void recordAgreement(struct port_data *port)
{
    if ((port->bpdu.configuration_bpdu.protocol_version_identifier ==
         RST_PROTOCOL_VERSION_IDENTIFIER) &&
        (port->operPointToPointMAC == TRUE) &&
        (port->bpdu.configuration_bpdu.flags & AGREEMENT_FLAG)) {
        set_port_flag(port->rstp_flags, agreed);
        remove_port_flag(port->rstp_flags, proposing);
        return;
    }
    remove_port_flag(port->rstp_flags, agreed);
}


/* Enter state */
static void enter_state(struct state_machine *stmch, enum states state)
{
    struct port_data *port = stmch->port;

    switch (state) {
    case CURRENT:
        break;
    case DISABLED:
        remove_port_flag(port->rstp_flags, rcvdMsg);
        remove_port_flag(port->rstp_flags, proposing);
        remove_port_flag(port->rstp_flags, proposed);
        remove_port_flag(port->rstp_flags, agree);
        remove_port_flag(port->rstp_flags, agreed);
        port->rcvdInfoWhile = 0;
        port->infoIs = Disabled;
        set_port_flag(port->rstp_flags, reselect);
        remove_port_flag(port->rstp_flags, selected);
        break;
    case AGED:
        port->infoIs = Aged;
        set_port_flag(port->rstp_flags, reselect);
        remove_port_flag(port->rstp_flags, selected);
        break;
    case UPDATE:
        remove_port_flag(port->rstp_flags, proposing);
        remove_port_flag(port->rstp_flags, proposed);
        (get_port_flag(port->rstp_flags, agreed) &&
            betterorsameInfo(port)) ?
            set_port_flag(port->rstp_flags, agreed) :
            remove_port_flag(port->rstp_flags, agreed);
        (get_port_flag(port->rstp_flags, synced) &&
            get_port_flag(port->rstp_flags, agreed)) ?
            set_port_flag(port->rstp_flags, synced) :
            remove_port_flag(port->rstp_flags, synced);
        port->portPriority = port->designatedPriority;
        port->portTimes = port->designatedTimes;
        remove_port_flag(port->rstp_flags, updtInfo);
        port->infoIs = Mine;
        set_port_flag(port->rstp_flags, newInfo);
        break;
    case SUPERIOR_DESIGNATED:
        remove_port_flag(port->rstp_flags, agreed);
        remove_port_flag(port->rstp_flags, proposing);
        recordProposal(port);
        setTcFlags(port);
        (get_port_flag(port->rstp_flags, agree) &&
            betterorsameInfo(port)) ?
            set_port_flag(port->rstp_flags, agree) :
            remove_port_flag(port->rstp_flags, agree);
        recordPriority(port);
        recordTimes(port);
        updtRcvdInfoWhile(port);
        port->infoIs = Received;
        set_port_flag(port->rstp_flags, reselect);
        remove_port_flag(port->rstp_flags, selected);
        remove_port_flag(port->rstp_flags, rcvdMsg);
        break;
    case REPEATED_DESIGNATED:
        recordProposal(port);
        setTcFlags(port);
        updtRcvdInfoWhile(port);
        remove_port_flag(port->rstp_flags, rcvdMsg);
        break;
    case INFERIOR_DESIGNATED:
        recordDispute(port);
        remove_port_flag(port->rstp_flags, rcvdMsg);
        break;
    case NOT_DESIGNATED:
        recordAgreement(port);
        setTcFlags(port);
        remove_port_flag(port->rstp_flags, rcvdMsg);
        break;
    case OTHER:
        remove_port_flag(port->rstp_flags, rcvdMsg);
        break;
    case RECEIVE:
        port->rcvdInfo = rcvInfo(port);
        break;
    }

    stmch->state = state;
}

/* Compute transitions */
static void compute_transitions(struct state_machine *stmch)
{
    struct bridge_data *br = stmch->bridge;
    struct port_data *port = stmch->port;
    static char *state_name [] = { /* For debug */
        [DISABLED] = "DISABLED",
        [AGED] = "AGED",
        [UPDATE] = "UPDATE",
        [CURRENT] = "CURRENT",
        [RECEIVE] = "RECEIVE",
        [SUPERIOR_DESIGNATED] = "SUPERIOR_DESIGNATED",
        [REPEATED_DESIGNATED] = "REPEATED_DESIGNATED",
        [INFERIOR_DESIGNATED] = "INFERIOR_DESIGNATED",
        [NOT_DESIGNATED] = "NOT_DESIGNATED",
        [OTHER] = "OTHER"
    };

    TRACEV(TRACE_INFO, "Computing transitions for the Port Information"
           " state machine");

    /* Test if the BEGIN flag is true */
    if (br->begin || (!get_port_flag(port->rstp_flags, portEnabled) &&
                      (port->infoIs != Disabled))) {
        enter_state(stmch, DISABLED);
        TRACEV(TRACE_INFO, "PIM: BEGIN is TRUE: current state: DISABLED");
        return;
    }

    TRACEV(TRACE_INFO, "PIM: current state: %s", state_name[stmch->state]);

    switch (stmch->state) {
    case DISABLED:
        if (get_port_flag(port->rstp_flags, portEnabled))
            enter_state(stmch, AGED);
        else if (get_port_flag(port->rstp_flags, rcvdMsg))
            enter_state(stmch, DISABLED);
        break;
    case AGED:
        if (get_port_flag(port->rstp_flags, selected) &&
            get_port_flag(port->rstp_flags, updtInfo)) {
            enter_state(stmch, UPDATE);
        }
        break;
    case UPDATE:
    case SUPERIOR_DESIGNATED:
    case REPEATED_DESIGNATED:
    case INFERIOR_DESIGNATED:
    case NOT_DESIGNATED:
    case OTHER:
        enter_state(stmch, CURRENT);
        break;
    case CURRENT:
        if (get_port_flag(port->rstp_flags, selected) &&
            get_port_flag(port->rstp_flags, updtInfo)) {
            enter_state(stmch, UPDATE);
        } else if ((port->infoIs == Received) &&
                   (port->rcvdInfoWhile == 0) &&
                   !get_port_flag(port->rstp_flags, updtInfo) &&
                   !get_port_flag(port->rstp_flags, rcvdMsg)) {
            enter_state(stmch, AGED);
        } else if (get_port_flag(port->rstp_flags, rcvdMsg) &&
                   !get_port_flag(port->rstp_flags, updtInfo)) {
            enter_state(stmch, RECEIVE);
        }
        break;
    case RECEIVE:
        switch (port->rcvdInfo) {
        case SuperiorDesignatedInfo:
            enter_state(stmch, SUPERIOR_DESIGNATED);
            break;
        case RepeatedDesignatedInfo:
            enter_state(stmch, REPEATED_DESIGNATED);
            break;
        case InferiorDesignatedInfo:
            enter_state(stmch, INFERIOR_DESIGNATED);
            break;
        case InferiorRootAlternateInfo:
            enter_state(stmch, NOT_DESIGNATED);
            break;
        case OtherInfo:
            enter_state(stmch, OTHER);
            break;
        }
        break;
    }

    TRACEV(TRACE_INFO, "PIM: entered state: %s", state_name[stmch->state]);
}


/* Initialize the PRS state machines data */
void initialize_pim(struct state_machine *stmch)
{
    stmch->id = PIM;
    stmch->compute_transitions = compute_transitions;
}
