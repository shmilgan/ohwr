#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>

#include <hw/trace.h>

#include <net-snmp/library/snmp-tc.h>

#include "rtu_fd_proxy.h"

#include "mrp.h"
#include "mrp_attr.h"
#include "mrp_pdu.h"
#include "timer.h"

/* Applicant State Table (See 802.1ak-2007 Table 10.3)
   Note: encoding optimization not handled in this version. */
static const struct mrp_state_trans
    mrp_applicant_state_table[MRP_APPLICANT_MAX][MRP_EVENT_MAX] = {
        [MRP_APPLICANT_VO] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_VO},
            [MRP_EVENT_R_NEW]       = {.state = MRP_APPLICANT_VO},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_APPLICANT_AO},
            [MRP_EVENT_R_IN]        = {.state = MRP_APPLICANT_VO},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_APPLICANT_VO},
            [MRP_EVENT_R_MT]        = {.state = MRP_APPLICANT_VO},
            [MRP_EVENT_R_LV]        = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_R_LA]        = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_PERIODIC]    = {.state = MRP_APPLICANT_VO},
            [MRP_EVENT_TX]          = {.state = MRP_APPLICANT_VO},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_LO}
        },
        [MRP_APPLICANT_VP] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_VO},
            [MRP_EVENT_R_NEW]       = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_APPLICANT_AP},
            [MRP_EVENT_R_IN]        = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_R_MT]        = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_R_LV]        = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_R_LA]        = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_PERIODIC]    = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_TX]          = {.state = MRP_APPLICANT_AA,
                                      .action = MRP_ACTION_S_JOIN},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_AA,
                                      .action = MRP_ACTION_S},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_VP}
        },
        [MRP_APPLICANT_VN] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_LA},
            [MRP_EVENT_R_NEW]       = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_R_IN]        = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_R_MT]        = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_R_LV]        = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_R_LA]        = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_PERIODIC]    = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_TX]          = {.state = MRP_APPLICANT_AN,
                                      .action = MRP_ACTION_S_NEW},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_AN,
                                      .action = MRP_ACTION_S_NEW},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_VN}
        },
        [MRP_APPLICANT_AN] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_AN},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_AN},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_LA},
            [MRP_EVENT_R_NEW]       = {.state = MRP_APPLICANT_AN},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_APPLICANT_AN},
            [MRP_EVENT_R_IN]        = {.state = MRP_APPLICANT_AN},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_APPLICANT_AN},
            [MRP_EVENT_R_MT]        = {.state = MRP_APPLICANT_AN},
            [MRP_EVENT_R_LV]        = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_R_LA]        = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_PERIODIC]    = {.state = MRP_APPLICANT_AN},
            [MRP_EVENT_TX]          = {.state = MRP_APPLICANT_QA,
                                      .action = MRP_ACTION_S_NEW},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_QA,
                                      .action = MRP_ACTION_S_NEW},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_VN}
        },
        [MRP_APPLICANT_AA] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_AA},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_LA},
            [MRP_EVENT_R_NEW]       = {.state = MRP_APPLICANT_AA},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_APPLICANT_QA},
            [MRP_EVENT_R_IN]        = {.state = MRP_APPLICANT_QA},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_APPLICANT_AA},
            [MRP_EVENT_R_MT]        = {.state = MRP_APPLICANT_AA},
            [MRP_EVENT_R_LV]        = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_R_LA]        = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_PERIODIC]    = {.state = MRP_APPLICANT_AA},
            [MRP_EVENT_TX]          = {.state = MRP_APPLICANT_QA,
                                      .action = MRP_ACTION_S_JOIN},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_QA,
                                      .action = MRP_ACTION_S_JOIN},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_VP}
        },
        [MRP_APPLICANT_QA] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_QA},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_LA},
            [MRP_EVENT_R_NEW]       = {.state = MRP_APPLICANT_QA},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_APPLICANT_QA},
            [MRP_EVENT_R_IN]        = {.state = MRP_APPLICANT_QA},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_APPLICANT_AA},
            [MRP_EVENT_R_MT]        = {.state = MRP_APPLICANT_AA},
            [MRP_EVENT_R_LV]        = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_R_LA]        = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_PERIODIC]    = {.state = MRP_APPLICANT_AA},
            [MRP_EVENT_TX]          = {.state = MRP_APPLICANT_QA},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_QA,
                                      .action = MRP_ACTION_S_JOIN},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_VP}
        },
        [MRP_APPLICANT_LA] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_AA},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_LA},
            [MRP_EVENT_R_NEW]       = {.state = MRP_APPLICANT_LA},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_APPLICANT_LA},
            [MRP_EVENT_R_IN]        = {.state = MRP_APPLICANT_LA},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_APPLICANT_LA},
            [MRP_EVENT_R_MT]        = {.state = MRP_APPLICANT_LA},
            [MRP_EVENT_R_LV]        = {.state = MRP_APPLICANT_LA},
            [MRP_EVENT_R_LA]        = {.state = MRP_APPLICANT_LA},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_APPLICANT_LA},
            [MRP_EVENT_PERIODIC]    = {.state = MRP_APPLICANT_LA},
            [MRP_EVENT_TX]          = {.state = MRP_APPLICANT_VO,
                                      .action = MRP_ACTION_S_LV},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_LO}
        },
        [MRP_APPLICANT_AO] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_AP},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_AO},
            [MRP_EVENT_R_NEW]       = {.state = MRP_APPLICANT_AO},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_APPLICANT_QO},
            [MRP_EVENT_R_IN]        = {.state = MRP_APPLICANT_AO},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_APPLICANT_AO},
            [MRP_EVENT_R_MT]        = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_R_LV]        = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_R_LA]        = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_PERIODIC]    = {.state = MRP_APPLICANT_AO},
            [MRP_EVENT_TX]          = {.state = MRP_APPLICANT_AO},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_LO}
        },
        [MRP_APPLICANT_QO] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_QP},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_QO},
            [MRP_EVENT_R_NEW]       = {.state = MRP_APPLICANT_QO},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_APPLICANT_QO},
            [MRP_EVENT_R_IN]        = {.state = MRP_APPLICANT_QO},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_APPLICANT_AO},
            [MRP_EVENT_R_MT]        = {.state = MRP_APPLICANT_AO},
            [MRP_EVENT_R_LV]        = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_R_LA]        = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_PERIODIC]    = {.state = MRP_APPLICANT_QO},
            [MRP_EVENT_TX]          = {.state = MRP_APPLICANT_QO},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_LO}
        },
        [MRP_APPLICANT_AP] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_AP},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_AO},
            [MRP_EVENT_R_NEW]       = {.state = MRP_APPLICANT_AP},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_APPLICANT_QP},
            [MRP_EVENT_R_IN]        = {.state = MRP_APPLICANT_AP},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_APPLICANT_AP},
            [MRP_EVENT_R_MT]        = {.state = MRP_APPLICANT_AP},
            [MRP_EVENT_R_LV]        = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_R_LA]        = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_PERIODIC]    = {.state = MRP_APPLICANT_AP},
            [MRP_EVENT_TX]          = {.state = MRP_APPLICANT_QA,
                                      .action = MRP_ACTION_S_JOIN},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_QA,
                                      .action = MRP_ACTION_S_JOIN},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_VP}
        },
        [MRP_APPLICANT_QP] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_QP},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_QO},
            [MRP_EVENT_R_NEW]       = {.state = MRP_APPLICANT_QP},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_APPLICANT_QP},
            [MRP_EVENT_R_IN]        = {.state = MRP_APPLICANT_QP},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_APPLICANT_AP},
            [MRP_EVENT_R_MT]        = {.state = MRP_APPLICANT_AP},
            [MRP_EVENT_R_LV]        = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_R_LA]        = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_PERIODIC]    = {.state = MRP_APPLICANT_AP},
            [MRP_EVENT_TX]          = {.state = MRP_APPLICANT_QP},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_QA,
                                      .action = MRP_ACTION_S_JOIN},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_VP}
        },
        [MRP_APPLICANT_LO] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_R_NEW]       = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_R_IN]        = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_APPLICANT_VO},
            [MRP_EVENT_R_MT]        = {.state = MRP_APPLICANT_VO},
            [MRP_EVENT_R_LV]        = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_R_LA]        = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_PERIODIC]    = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_TX]          = {.state = MRP_APPLICANT_VO,
                                      .action = MRP_ACTION_S},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_LO},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_LO}
        }
    };

/* Registrar State Table (See 802.1ak-2007 Table 10.4) */
static const struct mrp_state_trans
    mrp_registrar_state_table[MRP_REGISTRAR_MAX][MRP_EVENT_MAX] = {
        [MRP_REGISTRAR_IN] = {
            [MRP_EVENT_R_NEW]       = {.state = MRP_REGISTRAR_IN,
                                      .action = MRP_ACTION_S_NEW},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_REGISTRAR_IN},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_REGISTRAR_IN},
            [MRP_EVENT_R_LV]        = {.state = MRP_REGISTRAR_LV,
                                      .action = MRP_ACTION_START_LEAVE_TIMER},
            [MRP_EVENT_R_LA]        = {.state = MRP_REGISTRAR_LV,
                                      .action = MRP_ACTION_START_LEAVE_TIMER},
            [MRP_EVENT_TX_LA]       = {.state = MRP_REGISTRAR_LV,
                                      .action = MRP_ACTION_START_LEAVE_TIMER},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_REGISTRAR_LV,
                                      .action = MRP_ACTION_START_LEAVE_TIMER},
            [MRP_EVENT_FLUSH]       = {.state = MRP_REGISTRAR_MT},
            [MRP_EVENT_LEAVE_TIMER] = {.state = MRP_REGISTRAR_IN}
        },
        [MRP_REGISTRAR_LV] = {
            [MRP_EVENT_R_NEW]       = {.state = MRP_REGISTRAR_IN,
                                      .action = MRP_ACTION_NEW_AND_STOP_LEAVE_TIMER},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_REGISTRAR_IN,
                                      .action = MRP_ACTION_STOP_LEAVE_TIMER},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_REGISTRAR_IN,
                                      .action = MRP_ACTION_STOP_LEAVE_TIMER},
            [MRP_EVENT_R_LV]        = {.state = MRP_REGISTRAR_LV},
            [MRP_EVENT_R_LA]        = {.state = MRP_REGISTRAR_LV},
            [MRP_EVENT_TX_LA]       = {.state = MRP_REGISTRAR_LV},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_REGISTRAR_LV},
            [MRP_EVENT_FLUSH]       = {.state = MRP_REGISTRAR_MT,
                                      .action = MRP_ACTION_LV},
            [MRP_EVENT_LEAVE_TIMER] = {.state = MRP_REGISTRAR_MT,
                                      .action = MRP_ACTION_LV}
        },
        [MRP_REGISTRAR_MT] = {
            [MRP_EVENT_R_NEW]       = {.state = MRP_REGISTRAR_IN,
                                      .action = MRP_ACTION_NEW},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_REGISTRAR_IN,
                                      .action = MRP_ACTION_JOIN},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_REGISTRAR_IN,
                                      .action = MRP_ACTION_JOIN},
            [MRP_EVENT_R_LV]        = {.state = MRP_REGISTRAR_MT},
            [MRP_EVENT_R_LA]        = {.state = MRP_REGISTRAR_MT},
            [MRP_EVENT_TX_LA]       = {.state = MRP_REGISTRAR_MT},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_REGISTRAR_MT},
            [MRP_EVENT_FLUSH]       = {.state = MRP_REGISTRAR_MT},
            [MRP_EVENT_LEAVE_TIMER] = {.state = MRP_REGISTRAR_MT}
        }
    };

/* LeaveAll State Table (See 802.1ak-2007 Table 10.5) */
static const struct mrp_state_trans
    mrp_leaveall_state_table[MRP_LEAVEALL_MAX][MRP_EVENT_MAX] = {
        [MRP_LEAVEALL_A] = {
            [MRP_EVENT_TX]             = {.state = MRP_LEAVEALL_P,
                                         .action = MRP_ACTION_S_LA},
            [MRP_EVENT_R_LA]           = {.state = MRP_LEAVEALL_P,
                                         .action = MRP_ACTION_START_LEAVE_ALL_TIMER},
            [MRP_EVENT_LEAVEALL_TIMER] = {.state = MRP_LEAVEALL_A,
                                         .action = MRP_ACTION_START_LEAVE_ALL_TIMER}
        },
        [MRP_LEAVEALL_P] = {
            [MRP_EVENT_TX]             = {.state = MRP_LEAVEALL_P},
            [MRP_EVENT_R_LA]           = {.state = MRP_LEAVEALL_P,
                                         .action = MRP_ACTION_START_LEAVE_ALL_TIMER},
            [MRP_EVENT_LEAVEALL_TIMER] = {.state = MRP_LEAVEALL_A,
                                         .action = MRP_ACTION_START_LEAVE_ALL_TIMER}
        }
    };

/* Periodic State Table (See 802.1ak-2007 Table 10.6) */
static const struct mrp_state_trans
    mrp_periodic_state_table[MRP_PERIODIC_MAX][MRP_EVENT_MAX] = {
        [MRP_PERIODIC_A] = {
            [MRP_EVENT_PERIODIC_ENABLED]  = {.state = MRP_PERIODIC_A},
            [MRP_EVENT_PERIODIC_DISABLED] = {.state = MRP_PERIODIC_P},
            [MRP_EVENT_PERIODIC_TIMER]    = {.state = MRP_LEAVEALL_A,
                                            .action = MRP_ACTION_PERIODIC_AND_START_PERIODIC_TIMER}
        },
        [MRP_PERIODIC_P] = {
            [MRP_EVENT_TX]                = {.state = MRP_LEAVEALL_P},
            [MRP_EVENT_R_LA]              = {.state = MRP_LEAVEALL_P,
                                            .action = MRP_ACTION_START_LEAVE_ALL_TIMER},
            [MRP_EVENT_LEAVEALL_TIMER]    = {.state = MRP_LEAVEALL_A,
                                            .action = MRP_ACTION_START_LEAVE_ALL_TIMER}
        }
    };

/* Default timer parameter values (centiseconds) */
static int join_time     = 20;
static int leave_time    = 60;
static int leaveall_time = 1000;
static int periodic_time = 100;

static void mrp_init_participant(int port, struct mrp_application *app)
{
    struct mrp_participant *p = &app->participants[port];

    p->app              = app;
    p->mad              = NULL;
    p->next_to_process  = NULL;
    p->leaveall         = 0;
    p->req_tx           = 0;

    /* Set port as point to point (default). Protocol works correcty even if
    port is not point to point */
    p->operPointToPointMAC = 1;

    /* Begin! */
    p->leaveall_state = MRP_LEAVEALL_P;
    p->periodic_state = MRP_PERIODIC_A;

    mrp_pdu_init(&p->pdu);

    /* Init transmission window */
    memset(p->tx_window, 0, 3 * sizeof(struct timespec));

    /* Start timers */
    timer_start(p->leaveall_timeout,
        random_val(leaveall_time,  (3 * leaveall_time) / 2));
    timer_start(p->periodic_timeout, periodic_time);
}

static void mrp_uninit_participant(int port, struct mrp_application *app)
{
    struct mrp_participant *p = &app->participants[port];

    mrp_attr_destroy_all(&p->mad);
}

/* Open packet socket to send and receive MRP PDUs for a given application,
   and makes sure that RTU delivers application packets to NIC.
   @param app contains MRP application configuration (ether_type, address)
   @return -1 if error. 0 otherwise.*/
int mrp_register_application(struct mrp_application *app)
{
    int i, fd, err;

    /* PF_PACKET:   Packet socket
       SOCK_DGRAM:  Remove link layer headers */
    fd = socket(PF_PACKET, SOCK_DGRAM, htons(app->proto.ethertype));
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    /* O_NONBLOCK: Non-blocking socket */
    fcntl(fd, F_SETFL, O_NONBLOCK);

    /* Make sure RTU delivers MRP application packets to NIC */
    errno = 0;
    err = rtu_fdb_proxy_create_static_entry(app->proto.address, WILDCARD_VID,
        0x400 /*egress*/, 0xfffffbff /*forbidden*/, ST_PERMANENT, ACTIVE);
    if (errno) {
        perror("rtu_fdb_proxy_create_static_entry");
        goto fail;
    }
    if (err) {
        fprintf(stderr, "create static entry failed err %d\n", err);
        goto fail;
    }

    app->proto.fd = fd;

    for (i = 0; i < NUM_PORTS; i++)
        mrp_init_participant(i, app);

    return 0;

fail:
    close(fd);
    return -1;
}

int mrp_unregister_application(struct mrp_application *app)
{
    int i, err;

    close(app->proto.fd);

    for (i = 0; i < NUM_PORTS; i++)
        mrp_uninit_participant(i, app);

    /* Make sure MRP app packets are forwarded on all ports (except NIC)*/
    errno = 0;
    err = rtu_fdb_proxy_create_static_entry(app->proto.address, WILDCARD_VID,
        0xfffffbff /*egress*/, 0x400 /*forbidden*/, ST_PERMANENT, ACTIVE);
    if (errno || err)
        return -1;
    return 0;
}

void mrp_participant_event(struct mrp_participant *p, enum mrp_event event)
{
    uint8_t state;
    uint8_t action;

    state = mrp_leaveall_state_table[p->leaveall_state][event].state;
    if (state == MRP_LEAVEALL_INVALID)
        return;

    action = mrp_leaveall_state_table[p->leaveall_state][event].action;
    switch (action) {
    case MRP_ACTION_START_LEAVEALL_TIMER:
        /* 'The Leave All Period Timer is set to a random value in the
           range LeaveAllTime < T < 1.5*LeaveAllTime when it is started' */
        timer_start(&p->leaveall_timeout,
            random_val(leaveall_time,  3 * leaveall_time / 2));
        break;
    case MRP_ACTION_S_LA:
        /* The LeaveAll event value specified by the transmission action is
           encoded in the NumberOfValues field of the VectorAttribute as
           specified. The sLA action also gives rise to a rLA! event against all
           instances of the Applicant state machine, or the Registrar state
           machine, associated with the MRP participant. */
        p->leaveall = 1; /* signal that the leaveall event ocurred */
        mrp_mad_event(p, MRP_EVENT_R_LA);
    }
    p->leaveall_state = state;
}

void mrp_port_event(struct mrp_port *port, enum mrp_event event)
{
    uint8_t state;
    uint8_t action;
    struct mrp_participant *p;

    state = mrp_periodic_state_table[port->periodic_state][event].state;
    if (state == MRP_PERIODIC_INVALID)
        return;

    action = mrp_periodic_state_table[port->periodic_state][event].action;
    switch (action) {
    case MRP_ACTION_PERIODIC_AND_START_PERIODIC_TIMER:
        /* 'Causes a periodic! event against all Applicant state machines
            associated with the participant(s)' attached to the port */
        for (p = port->participants; p; p = p->next)
            mrp_participant_event(p, MRP_EVENT_PERIODIC);
    case MRP_ACTION_START_PERIODIC_TIMER:
        timer_start(port->periodic_timeout);
    }
    port->periodic_state = state;
}

void mrp_attr_event(struct mrp_participant *p, struct mrp_attr *attr,
    enum mrp_event event)
{
    uint8_t state;
    uint8_t action;
    int new = 0;        //  new = 1 for new attribute declarations
    int in;             //  in = 1 when the registrar is in IN state

registrar:
    /* Update registrar state _before_ computing applicant state */
    state  = mrp_registrar_state_table[attr->reg_state][event].state;
    if (state == MRP_REGISTRAR_INVALID)
        goto applicant;

    action = mrp_registrar_state_table[attr->reg_state][event].action;
    switch (action) {
    case MRP_ACTION_NEW_AND_STOP_LEAVE_TIMER:
        attr->leave_timer_running = 0;
    case MRP_ACTION_NEW:
        new = 1;
    case MRP_ACTION_JOIN:
        p->app->mad_join_ind(attr->type, attr->len, attr->value, new);
        // TODO propagate new indication
        break;
    case MRP_ACTION_START_LEAVE_TIMER:
        timer_start(attr->leave_timeout);
        attr->leave_timer_running = 1;
        break;
    case MRP_ACTION_STOP_LEAVE_TIMER:
        attr->leave_timer_running = 0;
        break;
    }

    in = (state == MRP_REGISTRAR_IN);

    attr->reg_state = state;

applicant:

    /* applicant state machine */
    state  = mrp_applicant_state_table[attr->app_state][event].state;
    if (state == MRP_APPLICANT_INVALID)
        return;

    /* Whenever a state machine transitions to a state that requires
       transmission of a message, a transmit opportunity is requested if
       one is not already pending.*/

    /* Note: If the PDU is full, state remains unchanged. This way the
       message will be sent in the next tx opportunity (which is requested) */
    action = mrp_applicant_state_table[attr->app_state][event].action;
    switch (action) {
    case MRP_ACTION_S_JOIN:
        mrp_req_tx_opportunity(p);
        if (!mrp_pdu_append_attr(p, attr, in ? MRP_JOIN_IN:MRP_JOIN_MT))
            return;
        break;
    case MRP_ACTION_S_NEW:
        mrp_req_tx_opportunity(p);
        if (!mrp_pdu_append_attr(p, attr, MRP_NEW))
            return;
        break;
    case MRP_ACTION_S_LV:
        mrp_req_tx_opportunity(p);
        if (!mrp_pdu_append_attr(p, attr, MRP_LV))
            return;
        break;
    case MRP_ACTION_S:
        mrp_req_tx_opportunity(p);
        if (!mrp_pdu_append_attr(p, attr, in ? MRP_IN:MRP_MT))
            return;
        break;
    }
    attr->app_state = state;
}

/* Apply the given event to state machines for all Attributes of attrtype.
   @param attrtype attribute type. 0 = wildcard (all types) */
void mrp_attrtype_event(struct mrp_participant *p, int attrtype,
    enum mrp_event event)
{
    struct mrp_attr *attr = p->mad;

    if (!attr)
        return;

    do {
        if ((attrtype == 0) || (attr->type == attrtype))
            mrp_attr_event(p, attr, event);
        attr = attr->next;
    } while (attr != p->mad); /* walk through the MAD circular list */
}

void mrp_mad_event(struct mrp_participant *p, enum mrp_event event)
{
    mrp_attrtype_event(p, 0, event);
}


/* Check whether the tx rate has been exceeded (i.e. there has been more than 3
   tx opportunities in 1,5*JoinTime)
   @param p pointer to MRP participant
   @return !0 if the tx rate has been exceeded; 0 otherwise. */
static int mrp_tx_rate_exceeded(struct mrp_participant *p)
{
    struct timespec now, window;

    timespec_copy(&window, p->tx_window);
    timespec_add_cs(&window, 3 * join_time / 2);
    return timespec_compare(timer_now(&now), &window) < 0;
}

/* Register that a tx opportunity has ocurred for the given participant.
   @param p pointer to MRP participant */
static void mrp_tx_reg(struct mrp_participant *p)
{
    struct timespec now, *window = &p->tx_window[0];

    /* Shift tx window */
    timespec_copy(window, window + 1);
    timespec_copy(window + 1, window + 2);
    timespec_copy(window + 2, timer_now(&now));
}

/* Request a transmission opportunity. 'A request for a transmit opportunity
   starts a randomized join timer, and the transmit opportunity is offered
   when the timer expires. When operPointToPointMAC is TRUE, transmit
   opportunities are scheduled immediately on request' */
static void mrp_req_tx_opportunity(struct mrp_participant *p)
{
    if (!p->join_timer_running) {
        timer_start(&p->join_timeout,
            p->operPointToPointMAC ? 0:random_val(0, join_time));
        p->join_timer_running = 1;
    }
}

/* Checks whether a transmission opportunity should be offered to the
   participant (i.e. once the join timer has expired).
   If operPointToPointMAC is TRUE, a transmission opportunity is inmediately
   offered subject to no more than three transmission opportunities in any
   period of 1.5*JoinTime'
   If operPointToPointMAC is FALSE, and there is no pending request, a transmit
   opportunity is scheduled at a time value randomized between 0 and JoinTime
   @return !0 if a tx opportunity is offered. 0 otherwise */
static int mrp_tx_opportunity(struct mrp_participant *p)
{
    if (p->join_timer_running) {
        if (mrp_timer_expired(&p->join_timeout))
            return p->operPointToPointMAC ?
                !mrp_tx_rate_exceeded(p):1;
    } else {
        if (!p->operPointToPointMAC) {
            timer_start(&p->join_timeout, random_val(0, join_time));
            p->join_timer_running = 1;
        }
    }
    return 0;
}

/* Transmit an MRPDU for the given participant. */
static void mrp_tx(struct mrp_participant *p)
{
    enum mrp_event event;

    mrp_participant_event(p, even)

    /* Determine the type of transmission event */
    event = p->leaveall ?
        (mrp_pdu_full(&p->pdu) ? MRP_EVENT_TX_LAF:MRP_EVENT_TX_LA):
        MRP_EVENT_TX

    /* Trigger tx! event */
    mrp_mad_event(p, event);

    /* Send MRPDU */
    mrp_pdu_send(p);

    /* Register that tx ocurred and stop join timer. */
    mrp_tx_reg(p);
    p->join_timer_running = 0;
    /* Reset LeaveAll event */
    p->leaveall = 0;
}


/* Multiple Registration Protocol Handler */
void mrp_protocol(struct mrp_application *app)
{
    struct mrp_port *port;
    struct mrp_participant *p;

    while (1) {
        /* Process received PDUs */
        mrp_pdu_rcv(app);
        for (i = 0; i < NUM_PORTS; i++) {
            port = app->ports + i;
            /* Handle timers */
            if (timer_expired(&port->periodic_timeout))
                mrp_port_event(port, MRP_EVENT_PERIODIC_TIMER);

            for (p = port->participants; p; p = p->next) {
                if (timer_expired(&p->leaveall_timeout))
                    mrp_participant_event(p, MRP_EVENT_LEAVEALL_TIMER);

                /* Handle transimission */
                if (mrp_tx_opportunity(p))
                    mrp_tx(p);
            }
        }


        for (i = 0; i < NUM_PORTS; i++) {
            p = app->participants + i;
            /* Handle timers */
            if (timer_expired(&p->periodic_timeout)) {
                timer_start(&p->periodic_timeout, periodic_time);
            }
            if (timer_expired(&p->leaveall_timeout)) {
                timer_start(&p->leaveall_timeout,
                    random_val(leaveall_time,  3 * leaveall_time / 2));
                p->leaveall = 1;
            }
            /* Handle transimission */
            if (mrp_tx_opportunity(p))
                mrp_tx(p);
        }
        usleep(100);
    }
}

/* Creates a proxy to communicate with the RTU.
   @return -1 if error. 0 otherwise. */
int mrp_init(void)
{
    if (!rtu_fdb_proxy_create("rtu_fdb")) {
        perror("rtu_fdb_proxy_create");
        return -1;
    }

    /* Start a random seed to generate random periods for timers */
    srand(time(NULL));

    /* Take a random value for the leave_time, between 60 and 100 centisec */
    leave_time = random_val(leave_time, 100);

    return 0;
}
