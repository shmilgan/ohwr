/*
 * White Rabbit Switch RSTP
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizán (miguel.baizan@integrasys-sa.com)
 *
 * Description: Manages the bridge and port RSTP information.
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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "rstp_data.h"
#include "rstp_stmch.h"
#include "rstp_if.h"


/* Structure to hold the bridge information. With this structure we can also
   access the ports information */
static struct bridge_data bridge;

/* For debug */
static void print_init_data(void);
static char *mac_to_string(uint8_t mac[ETH_ALEN], char str[3 * ETH_ALEN]);

/* Initialize bridge and port structures to keep the RSTP parameters and
   states */
int init_data(void)
{
    int i;
    char __attribute__((unused)) mac[3 * ETH_ALEN];


    /*
     * 1) FILL IN STRUCTURES WITH NON-RSTP DATA
    */
    if (rstp_if_init_port_data(&bridge)) {
        TRACE(TRACE_FATAL, "Error while filling port structures with data");
        return -1;
    }

    /*
     * 2) INIT MANAGEABLE RSTP DATA FOR BOTH BRIDGES AND PORTS
    */

    /* TODO: For now we set the values of the writable management data to
       defaults defined by the standard. In a future, they'll have to be read
       from some kind of persistent configuration file */
    bridge.mng.BridgeForwardDelay = DEFAULT_BRIDGE_FORWARD_DELAY;
    bridge.mng.BridgeHelloTime = DEFAULT_BRIDGE_HELLO_TIME;
    bridge.mng.BridgeMaxAge = DEFAULT_BRIDGE_MAX_AGE;
    bridge.mng.MigrateTime = DEFAULT_MIGRATE_TIME;
    bridge.mng.forceVersion = RSTP_NORMAL_OPERATION;
    bridge.mng.BridgeIdentifier.prio[0] = 0; /* Prio defined in steps of 16 */
    bridge.mng.BridgeIdentifier.prio[1] =
        ((DEFAULT_BRIDGE_PRIORITY >> 4) & 0x0F);

    if (rstp_if_get_bridge_addr(&bridge)) {
        TRACE(TRACE_FATAL, "Error while getting the bridge address");
        return -1;
    }
    TRACEV(TRACE_INFO, "The bridge address is %s",
           mac_to_string(bridge.mng.BridgeIdentifier.addr, mac));

    bridge.mng.TxHoldCount = DEFAULT_TRANSMIT_HOLD_COUNT;

    for (i = 0; i < MAX_NUM_PORTS ; i++) {
        bridge.ports[i].mng.PortEnabled = 1; /* All ports participate in the ST
                                                at startup */
        bridge.ports[i].mng.AdminEdgePort = 0;
        bridge.ports[i].mng.PortPriority = DEFAULT_PORT_PRIORITY;
        bridge.ports[i].mng.PortPathCost = DEFAULT_PORT_PATH_COST_1_GBPS;
    }

    /*
     * 3) INIT BRIDGE RSTP DATA
    */

    bridge.begin = 0;

    /* At startup, during 'Root War', all the bridges consider itself as Root */
    /* Set priority vectors. See 17.18.3, 17.18.6 and 17.6 */
    bridge.BridgePriority.RootBridgeId = bridge.mng.BridgeIdentifier;
    bridge.BridgePriority.TxBridgeId = bridge.mng.BridgeIdentifier;
    bridge.BridgePriority.RootPathCost =
    bridge.BridgePriority.TxPortId =
    bridge.BridgePriority.RxPortId = 0;
    bridge.rootPriority = bridge.BridgePriority;

    /* Set the Port Identifier of the Root Port. See 17.18.5 */
    bridge.rootPortId = bridge.rootPriority.RxPortId;

    /* Set the Bridge and Root times. See 17.18.4 and 17.18.7 */
    bridge.BridgeTimes.message_age = 0;
    bridge.BridgeTimes.max_age = bridge.mng.BridgeMaxAge;
    bridge.BridgeTimes.hello_time = bridge.mng.BridgeHelloTime;
    bridge.BridgeTimes.forward_delay = bridge.mng.BridgeForwardDelay;
    bridge.rootTimes = bridge.BridgeTimes;

    /*
     * 4) INIT PORT RSTP DATA
    */

    for (i = 0; i < MAX_NUM_PORTS ; i++) {
        /* Set the Port ID. See 9.2.7 */
        bridge.ports[i].portId =
            ((bridge.ports[i].mng.PortPriority << 8) |
             (bridge.ports[i].port_number));

        /* Set some flags */
        bridge.ports[i].rstp_flags = 0x0;
        /* 17.19.18 */
        if (bridge.ports[i].link_status && bridge.ports[i].mng.PortEnabled) {
            set_port_flag(bridge.ports[i].rstp_flags, PORTENABLED);
        } else { /* Link status down or administrative port state disabled */
            remove_port_flag(bridge.ports[i].rstp_flags, PORTENABLED);
        }

        /* Set priority vectors */
        bridge.ports[i].designatedPriority = bridge.rootPriority;
        bridge.ports[i].designatedPriority.TxPortId =
        bridge.ports[i].designatedPriority.RxPortId = bridge.ports[i].portId;

        memset(&bridge.ports[i].msgPriority, 0x00,
               sizeof(struct st_priority_vector));
        bridge.ports[i].portPriority = bridge.rootPriority;
        bridge.ports[i].portPriority.RxPortId = bridge.ports[i].portId;

        /* Set timers */
        bridge.ports[i].designatedTimes = bridge.rootTimes;  /* 17.19.5 and
                                                                17.21.25 */
        bridge.ports[i].designatedTimes.hello_time =
            bridge.BridgeTimes.hello_time;
        memset(&bridge.ports[i].msgTimes, 0x00, sizeof(struct rstp_times));
        bridge.ports[i].portTimes = bridge.ports[i].designatedTimes;
    }

    /*
     * 5) INIT STMCHs STATES
    */
    /* TODO init port and bridge states; BEGIN = TRUE to initialize the states
       of the STMCHs */


    print_init_data(); /* For debug */

    return 0;
}


/* Re-compute the state of the STMCHs when one second has expired. We we'll
   iterate through the list of STMCHs on each port */
void recompute_stmchs(void)
{
    /* TODO Function to be implemented */

    /* Each time this function is instatiated we should test the port's link
       status in order to update the rstp info accordingly. This polling is
       a temporary solution, while we implement a method for notifications */

    /* Update the TICK flag */
    int i = 0;
    for (i = 0; i < MAX_NUM_PORTS ; i++) {
        set_port_flag(bridge.ports[i].rstp_flags, TICK);
    }
}



static void print_init_data(void)
{
    char __attribute__((unused)) *up = "up";
    char __attribute__((unused)) *down = "down";
    int i = 0;

    TRACEV(TRACE_INFO, "Printing initialisation data for ports...");

    for (i = 0; i < MAX_NUM_PORTS ; i++) {
        TRACEV(TRACE_INFO, "port id %d, named %s, is %s",
               bridge.ports[i].portId, bridge.ports[i].port_name,
               (bridge.ports[i].link_status ? up : down));
    }
}

static char *mac_to_string(uint8_t mac[ETH_ALEN], char str[3 * ETH_ALEN])
{
    snprintf(str, 3 * ETH_ALEN, "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return str;
}
