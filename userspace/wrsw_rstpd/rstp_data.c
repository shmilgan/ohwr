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

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <hw/trace.h>

#include "rstp_data.h"
#include "rstp_stmch.h"
#include "rstp_if.h"


/* Structure to hold the bridge information. With this structure we can also
   access the ports information */
struct bridge_data br;

/* For debug */
static void print_init_data(void);

/* Initialize bridge and port structures to keep the RSTP parameters and
   states */
int init_data(void)
{
    int                         i;
    int                         ret = 0;
    uint8_t                     *bridge_addr;


    /*
     * 1) FILL IN STRUCTURES WITH NON-RSTP DATA
    */
    ret = rstp_if_init_port_data();
    if (ret) {
        TRACE(TRACE_INFO, "Error while filling port structures with data");
        return -1;
    }

    /*
     * 2) INIT MANAGEABLE RSTP DATA FOR BOTH BRIDGES AND PORTS
    */

    /* TODO: For now we set the values of the writable management data to
       defaults defined by the standard. In a future, they'll have to be read
       from some kind of persistent configuration file */
    br.mng.BridgeForwardDelay = DEFAULT_BRIDGE_FORWARD_DELAY;
    br.mng.BridgeHelloTime = DEFAULT_BRIDGE_HELLO_TIME;
    br.mng.BridgeMaxAge = DEFAULT_BRIDGE_MAX_AGE;
    br.mng.MigrateTime = DEFAULT_MIGRATE_TIME;
    br.mng.forceVersion = RSTP_NORMAL_OPERATION;
    br.mng.BridgeIdentifier.prio[0] = 0; /* Prio defined in steps of 16 */
    br.mng.BridgeIdentifier.prio[1] = ((DEFAULT_BRIDGE_PRIORITY >> 4) & 0x0F);
    bridge_addr = rstp_if_get_bridge_addr();
    memcpy(&br.mng.BridgeIdentifier.addr, &bridge_addr, ETH_ALEN);
    br.mng.TxHoldCount = DEFAULT_TRANSMIT_HOLD_COUNT;

    for (i = 0; i < MAX_NUM_PORTS ; i++) {
        br.ports[i].mng.PortEnabled = 1; /* All ports participate in the ST at
                                            startup */
        br.ports[i].mng.AdminEdgePort = 0;
        br.ports[i].mng.PortPriority = DEFAULT_PORT_PRIORITY;
        br.ports[i].mng.PortPathCost = DEFAULT_PORT_PATH_COST_1_GBPS;
    }

    /*
     * 3) INIT BRIDGE RSTP DATA
    */

    br.begin = 0;

    /* At startup, during 'Root War', all the bridges consider itself as Root */
    /* Set priority vectors. See 17.18.3, 17.18.6 and 17.6 */
    br.BridgePriority.RootBridgeId = br.mng.BridgeIdentifier;
    br.BridgePriority.TxBridgeId = br.mng.BridgeIdentifier;
    br.BridgePriority.RootPathCost =
    br.BridgePriority.TxPortId =
    br.BridgePriority.RxPortId = 0;
    br.rootPriority = br.BridgePriority;

    /* Set the Port Identifier of the Root Port. See 17.18.5 */
    br.rootPortId = br.rootPriority.RxPortId;

    /* Set the Bridge and Root times. See 17.18.4 and 17.18.7 */
    br.BridgeTimes.message_age = 0;
    br.BridgeTimes.max_age = br.mng.BridgeMaxAge;
    br.BridgeTimes.hello_time = br.mng.BridgeHelloTime;
    br.BridgeTimes.forward_delay = br.mng.BridgeForwardDelay;
    br.rootTimes = br.BridgeTimes;

    /*
     * 4) INIT PORT RSTP DATA
    */

    for (i = 0; i < MAX_NUM_PORTS ; i++) {
        /* Set the Port ID. See 9.2.7 */
        br.ports[i].portId =
            ((br.ports[i].mng.PortPriority << 8) | (br.ports[i].port_number));

        /* Set some flags */
        br.ports[i].rstp_flags = 0x0;
        if (br.ports[i].link_status && br.ports[i].mng.PortEnabled) { /* 17.19.18 */
            SET_PORT_FLAG(br.ports[i].rstp_flags, PORTENABLED);
        } else { /* Link status down or administrative port state disabled */
            REMOVE_PORT_FLAG(br.ports[i].rstp_flags, PORTENABLED);
        }

        /* Set priority vectors */
        br.ports[i].designatedPriority = br.rootPriority;
        br.ports[i].designatedPriority.TxPortId =
        br.ports[i].designatedPriority.RxPortId = br.ports[i].portId;

        memset(&br.ports[i].msgPriority, 0x00,
               sizeof(struct st_priority_vector));
        br.ports[i].portPriority = br.rootPriority;
        br.ports[i].portPriority.RxPortId = br.ports[i].portId;

        /* Set timers */
        br.ports[i].designatedTimes = br.rootTimes;  /* 17.19.5 and 17.21.25 */
        br.ports[i].designatedTimes.hello_time = br.BridgeTimes.hello_time;
        memset(&br.ports[i].msgTimes, 0x00, sizeof(struct rstp_times));
        br.ports[i].portTimes = br.ports[i].designatedTimes;
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
void one_second(void)
{
    /* TODO Function to be implemented */

    /* Each time this function is instatiated we should test the port's link
       status in order to update the rstp info accordingly. This polling is
       a temporary solution, while we implement a method for notifications */

    /* Update the TICK flag */
    int i = 0;
    for (i = 0; i < MAX_NUM_PORTS ; i++) {
        SET_PORT_FLAG(br.ports[i].rstp_flags, TICK);
    }
}



static void print_init_data(void)
{
    char *up = "up";
    char *down = "down";
    int i = 0;

    TRACE(TRACE_INFO, "Printing initialisation data for ports...");

    for (i = 0; i < MAX_NUM_PORTS ; i++) {
        TRACE(TRACE_INFO, "port id %d, named %s, is %s",
              br.ports[i].portId, br.ports[i].port_name,
              (br.ports[i].link_status ? up : down));
    }
}
