/*
 * White Rabbit RTU (Routing Table Unit)
 * Copyright (C) 2010, CERN.
 *
 * Version:     wrsw_rtud v1.0
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *              Miguel Baizan   (miguel.baizan@integrasys.es)
 *
 * Description: RTU daemon.
 *              Handles the learning and aging processes.
 *              Manages the filtering and VLAN databases.
 *
 * Fixes:
 *              Alessandro Rubini
 *              Tomasz Wlostowski
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

#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

#include <hw/switch_hw.h>
#include <hal_client.h>

#include <net-snmp/library/snmp-tc.h>

#include "rtu.h"
#include "mac.h"
#include "rtu_fd.h"
#include "rtu_drv.h"
#include "rtu_hash.h"
#include "rtu_fd_srv.h"
#include "endpoint_hw.h"
#include "utils.h"

static pthread_t aging_process;
static pthread_t fdb_srv_process;

/**
 * Creates static entries for reserved MAC addresses in the filtering
 * database. Should be called on FDB (re)initialisation. These entries are
 * permanent and can not be modified.
 * @return error code
 */
static int create_permanent_entries()
{
    uint8_t bcast_mac[]         = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    uint8_t slow_proto_mac[]    = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x01};

    hexp_port_list_t plist;
    hexp_port_state_t pstate;
    int i, err;

    // VLAN-aware Bridge reserved addresses (802.1Q-2005 Table 8.1)
    // NIC egress port = 11 -> 0x400 (any other port is forbidden)
    TRACE(TRACE_INFO,"adding static routes for slow protocols...");
    for(i = 0; i < NUM_RESERVED_ADDR; i++) {
        slow_proto_mac[5] = i;
        err = rtu_fdb_create_static_entry(slow_proto_mac, WILDCARD_VID,
            0x400, 0xfffffbff, ST_READONLY, ACTIVE, !IS_BPDU);
        if(err)
            return err;
    }

    // packets addressed to WR card interfaces are forwarded to NIC virtual port
    halexp_query_ports(&plist);
    for(i = 0; i < plist.num_ports; i++) {
        halexp_get_port_state(&pstate, plist.port_names[i]);
        TRACE(
            TRACE_INFO,
            "adding static route for port %s index %d [mac %s]",
            plist.port_names[i],
            pstate.hw_index,
            mac_to_string(pstate.hw_addr)
        );
		err = rtu_fdb_create_static_entry(pstate.hw_addr, WILDCARD_VID,
		    0x400, 0xfffffbff, ST_READONLY, ACTIVE, !IS_BPDU);
        if(err)
            return err;
    }

    // Broadcast MAC
    TRACE(TRACE_INFO,"adding static route for broadcast MAC...");
    err = rtu_fdb_create_static_entry(bcast_mac, WILDCARD_VID,
        0xffffffff, 0x00000000, ST_READONLY, ACTIVE, !IS_BPDU);
    if(err)
        return err;

    return 0;
}

/**
 * \brief Periodically removes the filtering database old entries.
 *
 */
static void *rtu_daemon_aging_process(void *arg)
{
    unsigned long aging_res = (unsigned long)arg;

    while(1){
        rtu_fdb_age_dynamic_entries();
        sleep(aging_res);
    }
    return NULL;
}

/**
 * \brief Handles the learning process.
 * @return error code
 */
static int rtu_daemon_learning_process()
{
    int err;
    struct rtu_request req;             // Request read from learning queue
    uint32_t port_map;                  // Destination port map
    uint16_t vid;                       // VLAN identifier

    while(1){
        // Serve pending unrecognised request
        err = rtu_hw_read_learning_queue(&req);
        if (err) {
            TRACE(TRACE_INFO,"read learning queue: err %d\n", err);
        } else {
//            TRACE_DBG(TRACE_INFO, "mac=%u.%u.%u.%u.%u.%u",
//                (int)req.src[0], (int)req.src[1], (int)req.src[2],
//                (int)req.src[3], (int)req.src[4], (int)req.src[5]);
            TRACE_DBG(
                TRACE_INFO,
                "ureq: port %d src %s VID %d priority %d",
                req.port_id,
                mac_to_string(req.src),
                req.has_vid  ? req.vid:DEFAULT_VID,
                req.has_prio ? req.prio:0
            );
            // If req has no VID, use default for untagged packets
            vid      = req.has_vid ? req.vid:DEFAULT_VID;
            port_map = (1 << req.port_id);
            // 802.1Q checking list:
            // 1. Check port is in learning state. Done at HW
            // 2. Check MAC address is unicast.
            if (mac_multicast(req.src)) // would prefer doing it at HW...
                continue;
            // 3. Check FDB is not full. Done at FDB
            // 4. Check VLAN member set is not empty. Done at FDB.
            // create or update entry at filtering database
            err = rtu_fdb_create_dynamic_entry(req.src, vid, port_map);
            if (err == -ENOMEM) {
                // TODO remove oldest entries (802.1D says you MAY do it)
                TRACE(TRACE_INFO, "filtering database full\n");
            } else if (err) {
                TRACE(TRACE_INFO, "create entry: err %d\n", err);
                break;
            }
        }
    }
    return err;
}

static void *rtu_daemon_fdb_srv_process(void *arg)
{
    struct minipc_ch *server;

    server = rtu_fdb_srv_create("rtu_fdb");
	while (1) {
	    if (minipc_server_action(server, 1000) < 0)
		    fprintf(stderr, "rtu_fdb server_action(): %s\n", strerror(errno));
    }
    return NULL;
}


/**
 * \brief RTU set up.
 * Initialises routing table cache and RTU at hardware.
 * @param poly hash polinomial.
 * @param aging_time Aging time in seconds.
 * @return error code.
 */
static int rtu_daemon_init(uint16_t poly, unsigned long aging_time)
{
    int i, err;

    // init RTU HW
    TRACE(TRACE_INFO, "init rtu hardware.");
    err = rtu_hw_init();
    if(err)
        return err;

    // disable RTU
    TRACE(TRACE_INFO, "disable rtu.");
    rtu_hw_disable();


    // init configuration for ports
    TRACE(TRACE_INFO, "init port config.");
    for(i = MIN_PORT; i <= MAX_PORT; i++) {
        // MIN_PORT <= port <= MAX_PORT, thus no err returned
        err = rtu_hw_learn_enable_on_port(i,1);
        err = rtu_hw_pass_all_on_port(i,1);
        err = rtu_hw_pass_bpdu_on_port(i,0);
        err = rtu_hw_set_fixed_prio_on_port(i,0);
        err = rtu_hw_set_unrecognised_behaviour_on_port(i,1);
    }

    // init filtering database
    TRACE(TRACE_INFO, "init fd.");
    err = rtu_fdb_init(poly, aging_time);
    if (err)
        return err;

    // create permanent entries for reserved MAC addresses
    err = create_permanent_entries();
    if (err)
        return err;

    // Fix default PVID
    TRACE(TRACE_INFO, "set default port VID.");
    err = ep_hw_init();
    if (err)
        return err;

    for(i = MIN_PORT; i <= MAX_PORT; i++) {
        err = ep_hw_set_pvid(i, DEFAULT_VID);
        if (err)
           TRACE_DBG(TRACE_INFO, "could not set default PVID on port %d.", i);
    }
    // turn on RTU
    TRACE(TRACE_INFO, "enable rtu.");
    rtu_hw_enable();

    return err;
}

/**
 * \brief RTU shutdown.
 */
static void rtu_daemon_destroy()
{
    // Threads stuff
    pthread_cancel(aging_process);

    // Turn off RTU
    rtu_hw_disable();
    rtu_hw_exit();
}

void sigint(int signum) {
    rtu_daemon_destroy();
    signal(SIGINT, SIG_DFL);
}

/**
 * \brief Starts up the learning and aging processes.
 */
int main(int argc, char **argv)
{
	int op, err;
    char *s, *name, *optstring;
    int run_as_daemon        = 0;
    uint16_t poly            = HW_POLYNOMIAL_CCITT;  // Hash polinomial
    unsigned long aging_res  = DEFAULT_AGING_RES;    // Aging resolution [sec.]
    unsigned long aging_time = DEFAULT_AGING_TIME;   // Aging time       [sec.]

    trace_log_stderr();

    if (argc > 1) {
        // Strip out path from argv[0] if exists, and extract command name
        for (name = s = argv[0]; s[0]; s++) {
            if (s[0] == '/' && s[1]) {
                name = &s[1];
            }
        }
        // Parse daemon options
        optstring = "dhp:r:t:";
        while ((op = getopt(argc, argv, optstring)) != -1) {
            switch(op) {
            case 'd':
                run_as_daemon = 1;
                break;
            case 'h':
                usage(name);
            case 'p':
                if (strcmp(optarg, "CCITT") == 0) {
                    poly = HW_POLYNOMIAL_CCITT;
                } else if (strcmp(optarg, "IBM") == 0) {
                    poly = HW_POLYNOMIAL_IBM;
                } else if (strcmp(optarg, "DECT") == 0) {
                    poly = HW_POLYNOMIAL_DECT;
                } else {
                    fprintf(stderr, "Invalid polynomial\n");
                    usage(name);
                }
                break;
            case 'r':
                if ((aging_res = atol(optarg)) <= 0) {
                    fprintf(stderr, "Invalid aging resolution\n");
                    usage(name);
                }
                break;
            case 't':
                aging_time = atol(optarg);
                if ((aging_time < MIN_AGING_TIME) ||
                    (aging_time > MAX_AGING_TIME)) {
                    fprintf(stderr, "Invalid aging time\n");
                    usage(name);
                }
                break;
            default:
                usage(name);
            }
        }
    }

    // Initialise RTU.
    if((err = rtu_daemon_init(poly, aging_time))) {
        rtu_daemon_destroy();
        return err;
    }

	// Register signal handler
	signal(SIGINT, sigint);

    // daemonize _before_ creating threads
    if(run_as_daemon)
        daemonize();

    // Start up aging process
    if ((err = pthread_create(&aging_process, NULL, rtu_daemon_aging_process, (void *) aging_res))) {
        rtu_daemon_destroy();
        return err;
    }

    if ((err = pthread_create(&fdb_srv_process, NULL, rtu_daemon_fdb_srv_process, (void *)NULL))) {
        rtu_daemon_destroy();
        return err;
    }

    // Start up learning process.
    err = rtu_daemon_learning_process();
    // On error, release RTU resources
    rtu_daemon_destroy();
	return err;
}
