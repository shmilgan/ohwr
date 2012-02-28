/*
 * White Rabbit MRP (Multiple Registration Protocol)
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *              Miguel Baizan   (miguel.baizan@integrasys.es)
 *
 * Description: Main MRP data and function prototypes (it includes the
 *              functions prototypes from mrp.c, mrp_pdu.c and mrp_socket.c)
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
#ifndef __WHITERABBIT_MRP_H
#define __WHITERABBIT_MRP_H

#include <stdint.h>
#include <linux/if_ether.h>

#include "llist.h"
#include "rtu_fd.h"

#define MRP_END_MARK                0x0000

#define MRP_END_MARK_LEN            2   /* octets (IEEE Corrigendum 1 to MRP) */
#define MRP_PROTOCOL_VERSION_LEN    1   /* octets */
#define MRP_MSG_HDR_LEN             2   /* octets */
#define MRP_ATTR_HDR_LEN            2   /* octets */

#define MAX_VECTOR_ATTR_LEN         ((ETH_DATA_LEN) - \
                                    ((MRP_PROTOCOL_VERSION_LEN) + (MRP_MSG_HDR_LEN)))

#define MAX_THREE_PACKED_EVENT_VAL  215 /* ((5 * 6) + 5) * 6) + 5 */

/* Minimum length for the Ethernet payload */
#define MIN_ETH_DATA_LEN            ((ETH_ZLEN) - (ETH_HLEN))

#define VLAN_TAG_LEN                4  /* octets */

/* PDU payload lenght to guarantee compatibility with interfaces that do not
   handle 802.1Q tags */
#define VLAN_ETH_DATA_LEN           ((ETH_DATA_LEN) - (VLAN_TAG_LEN))

const static uint16_t endmark = MRP_END_MARK;

enum mrp_applicant_state {
    MRP_APPLICANT_INVALID,
    MRP_APPLICANT_VO,           // Very anxious observer
    MRP_APPLICANT_VP,           // Very anxious passive
    MRP_APPLICANT_VN,           // Very anxious new
    MRP_APPLICANT_AN,           // Anxious      new
    MRP_APPLICANT_AA,           // Anxious      active
    MRP_APPLICANT_QA,           // Quiet        active
    MRP_APPLICANT_LA,           // Leaving      active
    MRP_APPLICANT_AO,           // Anxious      observer
    MRP_APPLICANT_QO,           // Quiet        observer
    MRP_APPLICANT_AP,           // Anxious      passive
    MRP_APPLICANT_QP,           // Quiet        passive
    MRP_APPLICANT_LO,           // Leaving      observer
    MRP_APPLICANT_MAX
};

enum mrp_registrar_state {
    MRP_REGISTRAR_INVALID,
    MRP_REGISTRAR_IN,           // In
    MRP_REGISTRAR_LV,           //'The accuracy required for the leavetimer is
    MRP_REGISTRAR_L3,           // sufficiently coarse as to permit the use of
    MRP_REGISTRAR_L2,           // a single operating system timer per Participant
    MRP_REGISTRAR_L1,           // with 2 bits of state for each Registrar'
    MRP_REGISTRAR_MT,           // Empty
    MRP_REGISTRAR_MAX
};

enum mrp_leaveall_state {
    MRP_LEAVEALL_INVALID,
    MRP_LEAVEALL_A,             // Active
    MRP_LEAVEALL_P,             // Passive
    MRP_LEAVEALL_MAX
};

enum mrp_periodic_state {
    MRP_PERIODIC_INVALID,
    MRP_PERIODIC_A,             // Active
    MRP_PERIODIC_P,             // Passive
    MRP_PERIODIC_MAX
};

enum mrp_attr_event {
    MRP_NEW         = 0,        // New
    MRP_JOIN_IN     = 1,        // JoinIn
    MRP_IN          = 2,        // In
    MRP_JOIN_MT     = 3,        // JoinEmpty
    MRP_MT          = 4,        // Empty
    MRP_LV          = 5         // Leave
};

enum mrp_event {
    MRP_EVENT_UNKOWN,

    MRP_EVENT_NEW,              // New attribute declaration
    MRP_EVENT_JOIN,             // Attribute declaration (no new registration)
    MRP_EVENT_LV,               // Withdraw attribute declaration

    MRP_EVENT_R_NEW,            // Receive New message
    MRP_EVENT_R_JOIN_IN,        // Receive JoinIn message
    MRP_EVENT_R_IN,             // Receive In message
    MRP_EVENT_R_JOIN_MT,        // Receive JoinEmpty message
    MRP_EVENT_R_MT,             // Receive Empty message
    MRP_EVENT_R_LV,             // Receive Leave message
    MRP_EVENT_R_LA,             // Receive LeaveAll message


    MRP_EVENT_TX,               // Tx opportunity without LeaveAll
    MRP_EVENT_TX_LA,            // Tx opportunity with a LeaveAll.
    MRP_EVENT_TX_LAF,           // Tx opportunity with a LeaveAll. (PDU full)

    MRP_EVENT_FLUSH,            // Root or Alternate port changed to Designated
    MRP_EVENT_REDECLARE,        // Designated port changed to Root or Alternate
    MRP_EVENT_PERIODIC,         // Periodic transmission event
    MRP_EVENT_PERIODIC_TIMER,   // Periodic timer expired
    MRP_EVENT_PERIODIC_ENABLED, // Periodic transmission enabled by management
    MRP_EVENT_PERIODIC_DISABLED,// Periodic transmission disabled by management

    MRP_EVENT_LEAVE_TIMER,      // Leave timer expired
    MRP_EVENT_LEAVEALL_TIMER,   // Leave All timer expired
    MRP_EVENT_MAX
};

enum mrp_applicant_mgt {
    MRP_NORMAL_PARTICIPANT,
    MRP_NON_PARTICIPANT
};

enum mrp_registrar_mgt {
    MRP_NORMAL_REGISTRATION,
    MRP_FIXED_REGISTRATION,
    MRP_FORBIDDEN_REGISTRATION
};

#define MRP_ACTION_MASK         0x0f
#define MRP_TIMER_ACTION_MASK   0x70
#define MRP_REQ_TX_MASK         0x80

/* Multiple actions can be combined considering the following syntax:
   bit 0-3: user and tx actions
   bit 4-6: timer actions
   bit 7:   flag for requesting tx opportunity */
enum mrp_action {
    MRP_ACTION_NONE  = 0x00,
    MRP_ACTION_NEW   = 0x01,         // Send New indication to MAP and MRP app.
    MRP_ACTION_JOIN  = 0x02,         // Send Join indication to MAP and MRP app.
    MRP_ACTION_LV    = 0x03,         // Send Leave indication to MAP and MRP app.

    MRP_ACTION_S_NEW  = 0x04,        // Send a New message
    MRP_ACTION_S_JOIN = 0x05,        // Send a JoinIn or JoinEmpty message
    MRP_ACTION_S_LV   = 0x06,        // Send a Leave message
    MRP_ACTION_S      = 0x07,        // Send an In or Empty message
    MRP_ACTION_S_LA   = 0x08,        // Send a LeaveAll message

    MRP_ACTION_PERIODIC = 0x09,      // Causes periodic event

    MRP_ACTION_START_LEAVE_TIMER     = 0x10,    // Start leave timer
    MRP_ACTION_STOP_LEAVE_TIMER      = 0x20,    // Stop leave timer
    MRP_ACTION_START_LEAVEALL_TIMER  = 0x30,    // Start leave all timer
    MRP_ACTION_START_PERIODIC_TIMER  = 0x40,    // Start periodic timer

    MRP_ACTION_REQ_TX = 0x80        // Request transmission opportunity
};

/* MRP State Transition */
struct mrp_state_trans {
    uint8_t state;
    uint8_t action;
};

/* Holds per-attribute applicant and registrar state machines.*/
struct mad_machine {
    enum mrp_applicant_state app_state : 4;
    enum mrp_registrar_state reg_state : 3;

    enum mrp_applicant_mgt app_mgt     : 1;
    enum mrp_registrar_mgt reg_mgt     : 2;
};

/* MRP Protocol Data Unit */
struct mrpdu {
    uint8_t buf[ETH_DATA_LEN];

    int maxlen;         // max buffer length
    int len;            // buffer length
    int pos;            // decoding position

    int mhdr;           // offset of message being encoded
    int vhdr;           // offset of vector header for attribute being encoded
};

struct mrp_application;

/* MRP port */
struct mrp_port {
    int hw_index;                       // Interface index of the port
    int port_no;                        // Port number
    int point_to_point;                 // 1 = point to point
                                        // 0 = shared medium
    struct timespec periodic_timeout;   // Periodic timer expiration time
    enum mrp_periodic_state periodic_state;

    struct mrp_application *app;        // mrp application component
    NODE *participants;                 // list of participants attached
                                        // to this port

    int is_enabled;
    int reg_failures;                   // number of registration failures
    uint8_t last_pdu_origin[ETH_ALEN];  // MAC addr of originator of the last
                                        // MRPDU that caused a change in the
                                        // Registrar state machine
};

/* MRP propagation context */
struct map_context {
    int cid;                            // context identifier
    int *members;                       // per-attribute membership
                                        // (participants registering an attr)
    NODE *participants;                 // List of participants
    NODE *forwarding_ports;             // List of ports in forwarding state
};

/* MRP participant */
struct mrp_participant {
    struct mrp_port *port;              // mrp port

    struct mad_machine *machines;       // per-attribute state machines
    enum mrp_leaveall_state leaveall_state; // per-part leaveall state machine

    struct timespec join_timeout;       // Join timer expiration time
    struct timespec leave_timeout_4;    // Leave timer expiration time
    struct timespec leaveall_timeout;   // LeaveAll timer expiration time

    int join_timer_running;             // 1 = running, 0 = not running
    int leave_timer_running;            // 1 = running, 0 = not running
    int leaveall;                       // 1 = leaveall timer expired

    struct timespec tx_window[3];       // Tx rate control

    struct mrpdu pdu;                   // used to encode MRP PDU

    int next_to_process;                // next attribute to start processing
                                        // with, in the following tx opportunity
    NODE *contexts;                     // List of propagation contexts
};

/* MRP protocol configuration */
struct mrp_proto {
    uint8_t version;                    // MRP Application Protocol Version
    uint16_t ethertype;
    uint8_t address[ETH_ALEN];
    int fd;                             // (raw) socket file descriptor
    int tagged;                         // 1 = vlan aware protocol
                                        // 0 = not vlan aware
};

/* MRP application configuration */
struct mrp_application {
    uint8_t  maxattr;                   // maximum attribute type
    uint32_t numattr;                   // supported number of attributes

    struct mrp_proto proto;             // protocol configuration
    NODE *ports;                        // List of (all) ports
    NODE *contexts;                     // List of (all) contexts

    /* Service Primitives */
	int (*mad_join_ind)(struct mrp_participant *p, int mid, int is_new);
	void (*mad_leave_ind)(struct mrp_participant *p, int mid);

    void (*mad_new_declaration)(struct mrp_participant *p, int mid);

    /* Has a topology change been detected? */
	int (*tc_detected)(struct mrp_port *port, struct map_context *c);

    /* Attribute database */
	int  (*db_add_entry)(uint8_t type, uint8_t len, void *firstval, int offset);
	int  (*db_read_entry)(uint8_t *type, uint8_t *len, void **val, int mid);
	int  (*db_find_entry)(uint8_t type, uint8_t len, void *firstval, int offset);
	void (*db_delete_entry)(uint8_t type, uint8_t len, void *val);
    uint8_t (*attrtype)(int mid);

    int (*init_participant)(struct mrp_participant *p);
    int (*uninit_participant)(struct mrp_participant *p);

    /* Attribute comparison */
	int (*attr_cmp)(uint8_t type, uint8_t len, void *firstval, void *secondval);
};

/* Service primitives */
void mrp_join_req(struct mrp_participant *p, int mid, int is_new);
void mrp_leave_req(struct mrp_participant *p, int mid);

void mrp_flush(struct mrp_participant *p);
void mrp_redeclare(struct mrp_participant *p);

/* MRP Configuration */
int mrp_register_application(struct mrp_application *app);
int mrp_unregister_application(struct mrp_application *app);

void mrp_init_port(struct mrp_port *port);
void mrp_uninit_port(struct mrp_port *port);

struct mrp_participant *mrp_create_participant(struct mrp_port *port);

void mrp_destroy_participant(struct mrp_participant *p);

struct map_context *map_context_create(int cid, struct mrp_application *app);
void map_context_destroy(struct map_context *c, struct mrp_application *app);

struct mrp_port *mrp_find_port(struct mrp_application *app, int hw_index);
struct map_context *mrp_find_context(struct mrp_application *app, int cid);
struct mrp_participant *mrp_find_participant(struct mrp_port *port,
                                             struct map_context *c);

int map_context_add_port(struct map_context *c, struct mrp_port *port);
void map_context_remove_port(struct map_context *c, struct mrp_port *port);

int map_context_add_participant(struct map_context *c,
                                struct mrp_participant *p);

void map_context_remove_participant(struct map_context *c,
                                    struct mrp_participant *p);

/* MRP Protocol */
int mrp_init(void);
void mrp_protocol(struct mrp_application *app);

int mad_attr_event(struct mrp_participant *p,
                    int mid,
                    enum mrp_event event);
void mad_attrtype_event(struct mrp_participant *p,
                        uint8_t attrtype,
                        enum mrp_event event);
void mad_participant_event(struct mrp_participant *p, enum mrp_event event);

/* MRP Management */
void mrp_set_registrar_control(struct mrp_participant *p,
                               int mid,
                               enum mrp_registrar_mgt control);

void mrp_set_applicant_control(struct mrp_participant *p,
                               int mid,
                               enum mrp_applicant_mgt control);

/* MRP PDU */

void mrp_pdu_init(struct mrpdu *pdu, int tagged);
int mrp_pdu_rcv(struct mrp_application *app);
void mrp_pdu_send(struct mrp_participant *part);
int mrp_pdu_full(struct mrpdu *pdu);
int mrp_pdu_empty(struct mrpdu *pdu);

int mrp_pdu_append_attr(struct mrp_participant *part,
                        int mid,
                        enum mrp_attr_event event);

int mrp_pdu_append_endmark(struct mrpdu *pdu);

/* MRP socket */
int mrp_open_socket(struct mrp_application *app);
void mrp_socket_send(struct mrp_participant *p);
struct mrp_participant *mrp_socket_rcv(struct mrp_application *app,
                                       struct mrpdu *pdu);


inline static int mad_declared_here(struct mrp_participant *p, int mid)
{
    return p->machines[mid].app_state != MRP_APPLICANT_VO;
}

inline static int mad_registered_here(struct mrp_participant *p, int mid)
{
    return p->machines[mid].reg_mgt != MRP_NORMAL_REGISTRATION ||
        p->machines[mid].reg_state != MRP_REGISTRAR_MT;
}

inline static int mad_machine_active(struct mrp_participant *p, int mid)
{
    return mad_declared_here(p, mid) || mad_registered_here(p, mid);
}

#endif /*__WHITERABBIT_MRP_H*/
