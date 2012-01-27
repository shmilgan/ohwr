#ifndef __WHITERABBIT_MRP_H
#define __WHITERABBIT_MRP_H

#include <stdint.h>
#include <linux/if_ether.h>

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
    MRP_REGISTRAR_LV,           // Leaving
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
    MRP_LV          = 5,        // Leave
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

    MRP_EVENT_LEAVE_TIMER,        // Leave timer expired
    MRP_EVENT_LEAVEALL_TIMER,     // Leave All timer expired
    MRP_EVENT_MAX
};

enum mrp_action {
    MRP_ACTION_NONE,
    MRP_ACTION_NEW,             // Send New indication to MAP and MRP app.
    MRP_ACTION_JOIN,            // Send Join indication to MAP and MRP app.
    MRP_ACTION_LV,              // Send Leave indication to MAP and MRP app.

    MRP_ACTION_S_NEW,           // Send a New message
    MRP_ACTION_S_JOIN,          // Send a JoinIn or JoinEmpty message
    MRP_ACTION_S_LV,            // Send a Leave message
    MRP_ACTION_S,               // Send an In or Empty message
    MRP_ACTION_S_LA,            // Send a LeaveAll message

    MRP_ACTION_PERIODIC,        // Causes periodic event

    MRP_ACTION_START_LEAVE_TIMER,       // Start leave timer
    MRP_ACTION_STOP_LEAVE_TIMER,        // Stop leave timer
    MRP_ACTION_START_LEAVE_ALL_TIMER,   // Start leave all timer
    MRP_ACTION_START_PERIODIC_TIMER,    // Start periodic timer

    // multiple event definition - not very elegant but keeps it simple
    MRP_ACTION_NEW_AND_STOP_LEAVE_TIMER,
    MRP_ACTION_PERIODIC_AND_START_PERIODIC_TIMER,

};

/* MRP State Transition */
struct mrp_state_trans {
    uint8_t state;
    uint8_t action;
};

/* MRP protocol configuration */
struct mrp_proto {
    uint8_t version;
    uint16_t ethertype;
    uint8_t address[ETH_ALEN];
    int fd;
};

/* MRP attribute.
   Also holds associated applicant and registrar state machines.*/
struct mrp_attr {
    enum mrp_applicant_state app_state;
    enum mrp_registrar_state reg_state;

    int map_count;                      // propagation count

    struct timeval leave_timeout;
    uint8_t leave_timer_running;

    struct mrp_attr *next;
    struct mrp_attr *prev;

    uint8_t type;
    uint8_t len;
    unsigned char value[];
};

/* MRP Protocol Data Unit */
struct mrpdu {
    uint8_t buf[ETH_DATA_LEN];          // Octet array buffer
    int len;                            // Number of octets in sdu buffer
    int pos;                            // Keeps track of decoding position

    struct mrp_attr *cur_attr;          // attribute being encoded
    int vhdr;                           // position of vector header within pdu,
                                        // for attribute being encoded
};

struct mrp_application;

/* MRP participant */
struct mrp_participant {
    int reg_failures;                   // number of registration failures

    enum mrp_leaveall_state leaveall_state;

    struct timespec join_timeout;       // Join timer expiration time
    struct timespec leaveall_timeout;   // LeaveAll timer expiration time

    struct mrp_attr *mad;               // mrp attribute declaration
    struct mrp_port *port;              // mrp port
    struct mrp_attr *next_to_process;   // next mad attribute to process
    int leaveall;                       // 1 = leaveall timer expired
    int join_timer_running;             // 1 = running, 0 = not running

    struct timespec tx_window[3];       // Tx rate control

    struct mrpdu pdu;                   // used to encode MRP PDU

    struct mrp_participant *next;
};

/* MRP port */
struct mrp_port {
    struct mrp_application *app;        // mrp application component

    int operPointToPointMAC;            // 1 = port operates point to point

    enum mrp_periodic_state periodic_state;

    struct timespec periodic_timeout;   // Periodic timer expiration time

    struct mrp_participant *participants;
}

/* MRP application configuration */
struct mrp_application {
    struct mrp_port ports[NUM_PORTS];

//    struct mrp_participant participants[NUM_PORTS];

    unsigned int maxattr;
    struct mrp_proto proto;
	void (*mad_join_ind)(int type, int len, void *value, int is_new);
	void (*mad_leave_ind)(int type, int len, void *value);

	int (*mad_index)(int type, int len, void *value);
	void (*mad_attr)(int *type, int *len, void *value, int mad_idx);

	void (*nextval)(int type, int len, void *value);
};

inline static int mrp_port(struct mrp_port *part)
{
    return part->port - &part->app->ports[0];
}

int mrp_init(void);
int mrp_register_application(struct mrp_application *appl);
int mrp_unregister_application(struct mrp_application *appl);
void mrp_protocol(struct mrp_application *appl);
void mrp_rcv(int port, struct mrpdu *pdu);
int mrp_send(int port, struct mrpdu *pdu);

void mrp_attr_event(struct mrp_attr *attr, enum mrp_event event);
void mrp_attrtype_event(struct mrp_participant *part, int attrtype,
    enum mrp_event event);
void mrp_mad_event(struct mrp_participant *part, enum mrp_event event);

#endif /*__WHITERABBIT_MRP_H*/
