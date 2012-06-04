/*
 * White Rabbit MVRP (Multiple VLAN Registration Protocol)
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *              Miguel Baizan   (miguel.baizan@integrasys.es)
 *
 * Description: MVRP daemon.
 *
 * Fixes:
 *              Alessandro Rubini
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
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <net/if.h>

#include <hw/trace.h>

#include "hal_exports.h"

#include "mrp.h"
#include "if_index.h"
#include "rtu_fd_proxy.h"
#include "mvrp_srv.h"
#include "wrsw_hal_ipc.h"
#include "mvrp.h"

/* Protocol Version as defined (802.1ak-2007) */
#define MVRP_PROTOCOL_VER   0x00

/* MVRP group MAC address */
#define MVRP_ADDRESS        { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x21 }

/* MVRP EtherType value */
#define ETH_P_MVRP          0x88F5

#define BASE_SPANNING_TREE_CONTEXT_ID 0

enum mvrp_attributes {
	MVRP_ATTR_INVALID,
	MVRP_ATTR_VID,
	MVRP_ATTR_MAX
};

/* VLAN registration entry */
struct vlan_reg_entry {
    uint16_t nvid;               /* network encoded vid */
    uint32_t egress_ports;
    uint32_t forbidden_ports;
};

/* MRP VLAN database (vfdb cache) */
static struct vlan_reg_entry vlan_db[NUM_VLANS];

/* A Single Spanning Tree instance is considered (i.e. a single context) */
static struct map_context *ctx;

/* Global state of MVRP operation */
static int mvrp_enabled;

static int switch_is_v2;

/* 'When any MVRP declaration marked as “new” is received on a given Port,
   either as a result of receiving an MVRPDU from the attached LAN
   (MAD_Join.indication), or as a result of receiving a request from MAP
   or the MVRP Application (MAD_Join.request), any entries in the filtering
   database for that Port and for the VLAN corresponding to the attribute value
   in the MAD_Join primitive are removed.' (802.1ak 11.2.5)*/
static void mvrp_new_declaration(struct mrp_participant *p, int mid)
{
    rtu_fdb_proxy_delete_dynamic_entries(p->port->port_no, mid);
}

static int mvrp_join_ind(struct mrp_participant *p, int mid, int is_new)
{
    struct mrp_port *port = p->port;

    if (rtu_vfdb_proxy_forward_dynamic(port->port_no, mid) < 0)
        return -1;

    if (is_new)
        mvrp_new_declaration(p, mid);

    return 0;
}

static void mvrp_leave_ind(struct mrp_participant *p, int mid)
{
    rtu_vfdb_proxy_filter_dynamic(p->port->port_no, mid);
}

/* Return 0 if both values are the same. Otherwise, return the difference */
static int attr_cmp(uint8_t type, uint8_t len, void *v1, void *v2)
{
    uint16_t val1;
    uint16_t val2;

    memcpy(&val1, v1, len);
    memcpy(&val2, v2, len);
    return ((type == MVRP_ATTR_VID) && (len == 2)) ?
           ntohs(val2) - ntohs(val1):
           0xffffffff;
}

static uint8_t attrtype(int mid)
{
    if ((mid < 0) || (mid >= NUM_VLANS))
        return MVRP_ATTR_INVALID;
    if (!switch_is_v2 && (mid == 0))
        return MVRP_ATTR_INVALID;
    return MVRP_ATTR_VID;
}

static int db_add_entry(uint8_t type, uint8_t len, void *firstval, int offset)
{
    uint16_t val;

    memcpy(&val, firstval, len);
    return ntohs(val) + offset;
}

static int db_read_entry(uint8_t *type, uint8_t *len, void **value, int mid)
{
    if ((mid < 0) || (mid >= NUM_VLANS))
        return -EINVAL;
    if (!switch_is_v2 && (mid == 0))
        return -EINVAL;

    *len = 2;
    *type = MVRP_ATTR_VID;
    *value = &vlan_db[mid].nvid;
    return 0;
}

static int db_find_entry(uint8_t type, uint8_t len, void *firstval, int offset)
{
    uint16_t val;
    int mid;

    memcpy(&val, firstval, len);
    mid = ntohs(val) + offset;

    if ((mid < 0) || (mid >= NUM_VLANS))
        return -EINVAL;
    if ((mid == 0) && !switch_is_v2)
        return -EINVAL;

    return mid;
}

/* Not required for this MRP application */
static void db_delete_entry(uint8_t type, uint8_t len, void *value)
{
    return;
}

/* Check whether the tcDetected timer is running or not.
   @return 1 if timer is running. 0 otherwise */
static int tc_detected(struct mrp_port *port, struct map_context *ctx)
{
    // TODO IPC with RSTPd to read the tcDetected value for the port and ST
    // instance (i.e., mrp_port(port) and ctx->id)
    return 0;
}

/* Set up participant registration controls based on existing VLAN entries */
static int mvrp_init_participant(struct mrp_participant *part)
{
    int vid, port_no = part->port->port_no;

    for (vid = 0; vid < NUM_VLANS; vid++) {
        if (is_set(vlan_db[vid].egress_ports, port_no)) {
            mrp_set_registrar_control(part, vid, MRP_FIXED_REGISTRATION);
            mrp_join_req(part, vid, 1 /* is new */);
        }
        /* TODO not clear whether forbidden registration requires join_req */
        if (is_set(vlan_db[vid].forbidden_ports, port_no))
            mrp_set_registrar_control(part, vid, MRP_FORBIDDEN_REGISTRATION);
    }
    return 0;
}

/* Remove dynamic entries for the participant*/
static int mvrp_uninit_participant(struct mrp_participant *part)
{
    int vid;

    for (vid = 0; vid < NUM_VLANS; vid++) {
        if (mad_registered_here(part, vid))
            rtu_vfdb_proxy_filter_dynamic(part->port->port_no, vid);
    }
    return 0;
}

struct mrp_application mvrp_app = {
    .maxattr         = MVRP_ATTR_MAX,
    .numattr         = NUM_VLANS,
    .mad_join_ind    = mvrp_join_ind,
    .mad_leave_ind   = mvrp_leave_ind,
    .mad_new_declaration = mvrp_new_declaration,
    .attr_cmp        = attr_cmp,
    .attrtype        = attrtype,
    .db_add_entry    = db_add_entry,
    .db_read_entry   = db_read_entry,
    .db_find_entry   = db_find_entry,
    .db_delete_entry = db_delete_entry,
    .tc_detected     = tc_detected,
    .init_participant   = mvrp_init_participant,
    .uninit_participant = mvrp_uninit_participant,
	.proto.version   = MVRP_PROTOCOL_VER,
    .proto.ethertype = ETH_P_MVRP,
    .proto.address   = MVRP_ADDRESS,
    .proto.tagged    = 0
};

/* Find port with the given port number in the application port list */
static struct mrp_port *find_port(int port_no)
{
    struct mrp_port *port;

    list_for_each_entry(port, &mvrp_app.ports, app_port)
        if (port->port_no == port_no)
            return port;
    return NULL;
}

/* Determine the forwarding state of a port within a given context.
   @return 1 if forwarding is TRUE, 0 if FALSE. */
static int is_forwarding(struct mrp_port *port)
{
    /* TODO Connect with STP instance to get the forwarding state of the port.
       For now, since there's no instance of STP, all operational ports will
       be considered to be forwarding. Note however that the implementation of
       WR creates an instance of each WR port even when there's no network
       interface attached to the port. As these ports seems to be operational,
       we need to be sure here if the port being checked is UP and RUNNING
       (i.e. if the port has a participant attached). This is a temporary
       solution until STP be implemented (the STP instance should have
       its own methods to determine if the port is forwarding) */
    if (!list_empty(&port->participants))
        return 1;
    return 0;
}

/* Add/Remove participants to/from ports based on MAC operational state.
   @return 0 on success. -1 in case of error */
static int mvrp_check_operational_state()
{
    struct mrp_port *port;
    struct mrp_participant *p;
    hexp_port_state_t port_info;
    char ifname[IF_NAMESIZE];


    list_for_each_entry(port, &mvrp_app.ports, app_port) {
        /* Get interface name */
        if (!if_indextoname(port->hw_index, ifname))
            return -1;

        /* Get the port MAC_Operational state from the WR Switch HAL */
        if (hal_get_port_info(&port_info, ifname) < 0)
            return -1;

        /* Create or delete participants according to the port state */
        if (port_info.up) {
            if (list_empty(&port->participants)) {
                /* There is a single MVRP participant per enabled port,
                irrespective of the port state in the ST context. */
                p = mrp_create_participant(port);
                if (!p)
                    return -1;

                /* Associate participant to the propagation context */
                if (map_context_add_participant(ctx, p) != 0)
                    return -1;
            }
        } else {
            if (!list_empty(&port->participants)) {
                p = list_first_entry(&port->participants,
                                     struct mrp_participant,
                                     port_participant);
                mrp_destroy_participant(p);
            }
        }
    }
    return 0;
}

/* Add/Remove ports to/from the propagation context based on forwarding state.
   @return -1 if an error occurred. 0 otherwise */
static int mvrp_check_forwarding_state()
{
    struct mrp_port *port;

    list_for_each_entry(port, &mvrp_app.ports, app_port) {
        if (is_forwarding(port)) {
            if (!list_find_port(&ctx->forwarding_ports, port))
                if (map_context_add_port(ctx, port) < 0)
                    return -1;
        } else {
            if (list_find_port(&ctx->forwarding_ports, port))
                map_context_remove_port(ctx, port);
        }
    }
    return 0;
}

/* Initialise vlan database with vfdb static entries */
static int init_vlan_table()
{
    int err;
    uint16_t vid;
    uint32_t egress_ports;
    uint32_t forbidden_ports;
    uint32_t untagged_set;

    memset(vlan_db, 0, sizeof(vlan_db));

    for (vid = 0; vid < NUM_VLANS; vid++)
        vlan_db[vid].nvid = htons(vid);

    vid = 0;
    /* Note VID = 0 is not permitted by 802.1Q (but was still used by V2 hw) */
    if (switch_is_v2) {
        errno = 0;
        err = rtu_fdb_proxy_read_static_vlan_entry(vid,
                                                   &egress_ports,
                                                   &forbidden_ports,
                                                   &untagged_set);
        if (errno)
            return -1;
        if (!err) {
            vlan_db[vid].egress_ports = egress_ports;
            vlan_db[vid].forbidden_ports = forbidden_ports;
        }
    }

    do {
        errno = 0;
        err = rtu_fdb_proxy_read_next_static_vlan_entry(&vid,
                                                        &egress_ports,
                                                        &forbidden_ports,
                                                        &untagged_set);
        if (errno)
            return -1;
        if (!err) {
            vlan_db[vid].egress_ports = egress_ports;
            vlan_db[vid].forbidden_ports = forbidden_ports;
        }
    } while (!err);

    return 0;
}

/* Initialise MVRP entity */
static int mvrp_init()
{
    fprintf(stderr, "mvrp: mrp init\n");
    if (mrp_init() != 0)
        return -1;

    /* 'A MAP Context identifier of 0 always identifies the Base Spanning Tree
       Context'. Note that even if no instance of STP is running, a context will
       still exist */
    fprintf(stderr, "mvrp: create propagation context\n");
    INIT_LIST_HEAD(&mvrp_app.contexts);
    ctx = map_context_create(BASE_SPANNING_TREE_CONTEXT_ID, &mvrp_app);
    if (!ctx)
        return -1;

    fprintf(stderr, "mvrp: init vlan table\n");
    init_vlan_table();

    fprintf(stderr, "mvrp: register application\n");
    if (mrp_register_application(&mvrp_app) != 0)
        return -1;

    mvrp_enabled = 1;

    return 0;
}

/* Helper function to daemonize the process */
static void daemonize(void)
{
    pid_t pid, sid;

    /* Already a daemon. Only allowed one daemon active at the same time */
    if ( getppid() == 1 ) return;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    /* If we got a good PID, then we can exit the parent process. */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* At this point we are executing as the child process */

    /* Change the file mode mask */
    umask(0);

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0)
        exit(EXIT_FAILURE);

    /* Change the current working directory.  This prevents the current
       directory from being locked; hence not being able to remove it. */
    if ((chdir("/")) < 0)
        exit(EXIT_FAILURE);

    /* Close standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

void sigint(int signum) {
    mrp_unregister_application(&mvrp_app);
    signal(SIGINT, SIG_DFL);
}

int main(int argc, char **argv)
{
    int op;
    char *optstring;
    int run_as_daemon = 0;
    struct minipc_ch *server;

    if (argc > 1) {
        /* Parse daemon options */
        optstring = "d:";
        while ((op = getopt(argc, argv, optstring)) != -1) {
            switch(op) {
            case 'd':
                run_as_daemon = 1;
                break;
            case 'v':
                switch_is_v2 = (atoi(optarg) == 2);
                break;
            default:
                fprintf(stderr,
                    "Usage: %s [-d] \n"
                    "\t-d   daemonize\n"
                    "\t-v<version>\n",
                    argv[0]);
                exit(1);
            }
        }
    }

    /* Daemonize */
    if(run_as_daemon)
        daemonize();

    if (mvrp_init() < 0)
        return -1;

    server = mvrp_srv_create("mvrp");

	// Register signal handler
	signal(SIGINT, sigint);

    fprintf(stderr, "mvrp: mrp protocol\n");
    while (1) {
        /* Handle user triggered actions (i.e. management) */
	    if (minipc_server_action(server, 10) < 0)
		    fprintf(stderr, "mvrp server_action(): %s\n", strerror(errno));

        if (!mvrp_enabled)
            continue;

        /* Handle any possible change in ports operational or forwarding state */
        if (mvrp_check_operational_state() < 0)
            goto failed;
        if (mvrp_check_forwarding_state() < 0)
            goto failed;

        /* Multiple Registration Protocol */
        mrp_protocol(&mvrp_app);

        // TODO handle port role change (i.e trigger flush and redeclare events)

    }

failed:
    mrp_unregister_application(&mvrp_app);
    return -1;
}

/* Management */

void mvrp_enable(void)
{
    mvrp_enabled = 1;
}

void mvrp_disable(void)
{
    mvrp_enabled = 0;
}

int mvrp_is_enabled(void)
{
    return mvrp_enabled;
}

int mvrp_enable_port(int port_no)
{
    struct mrp_port *port = find_port(port_no);
    if (!port)
        return -EINVAL;
    port->is_enabled = 1;
    return 0;
}

int mvrp_disable_port(int port_no)
{
    struct mrp_port *port = find_port(port_no);
    if (!port)
        return -EINVAL;
    port->is_enabled = 0;
    return 0;
}

int mvrp_is_enabled_port(int port_no)
{
    struct mrp_port *port = find_port(port_no);
    if (!port)
        return -EINVAL;
    return port->is_enabled;
}

int mvrp_get_failed_registrations(int port_no)
{
    struct mrp_port *port = find_port(port_no);
    if (!port)
        return -EINVAL;
    return port->reg_failures;
}

int mvrp_get_last_pdu_origin(int port_no, uint8_t (*mac)[ETH_ALEN])
{
    struct mrp_port *port = find_port(port_no);
    if (!port)
        return -EINVAL;
    mac_copy(*mac, port->last_pdu_origin);
    return 0;
}
