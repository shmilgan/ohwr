/*
 * White Rabbit MRP (Multiple Registration Protocol)
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *              Miguel Baizan   (miguel.baizan@integrasys.es)
 *
 * Description: Main MRP functions.
 *              State machines definition.
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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include <hal_exports.h>

#include <hw/trace.h>

#include <net-snmp/library/snmp-tc.h>

#include "rtu_fd_proxy.h"

#include "mrp.h"
#include "timer.h"
#include "if_index.h"
#include "wrsw_hal_ipc.h"

/* Full-Participant Applicant State Table (See 802.1ak-2007 Table 10.3)
   Note: encoding optimization not handled in this version.
   Note: Whenever a state machine transitions to a state that requires
   transmission of a message, a transmit opportunity is requested (if one is not
   already pending.) */
static const struct mrp_state_trans
    mrp_applicant_state_table[MRP_APPLICANT_MAX][MRP_EVENT_MAX] = {
        [MRP_APPLICANT_VO] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN,
                                      .action = MRP_ACTION_REQ_TX },
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_VP,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_VO},
            [MRP_EVENT_R_NEW]       = {.state = MRP_APPLICANT_VO},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_APPLICANT_AO},
            [MRP_EVENT_R_IN]        = {.state = MRP_APPLICANT_VO},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_APPLICANT_VO},
            [MRP_EVENT_R_MT]        = {.state = MRP_APPLICANT_VO},
            [MRP_EVENT_R_LV]        = {.state = MRP_APPLICANT_LO,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_R_LA]        = {.state = MRP_APPLICANT_LO,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_APPLICANT_LO,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_PERIODIC]    = {.state = MRP_APPLICANT_VO},
            [MRP_EVENT_TX]          = {.state = MRP_APPLICANT_VO},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_LO,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_LO,
                                      .action = MRP_ACTION_REQ_TX}
        },
        [MRP_APPLICANT_VP] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN,
                                      .action = MRP_ACTION_REQ_TX },
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_VO},
            [MRP_EVENT_R_NEW]       = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_APPLICANT_AP,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_R_IN]        = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_R_MT]        = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_R_LV]        = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_R_LA]        = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_PERIODIC]    = {.state = MRP_APPLICANT_VP},
            [MRP_EVENT_TX]          = {.state = MRP_APPLICANT_AA,
                                      .action = MRP_ACTION_S_JOIN |
                                                MRP_ACTION_REQ_TX},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_AA,
                                      .action = MRP_ACTION_S |
                                                MRP_ACTION_REQ_TX},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_VP,
                                      .action = MRP_ACTION_REQ_TX}
        },
        [MRP_APPLICANT_VN] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_VN},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_LA,
                                      .action = MRP_ACTION_REQ_TX},
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
                                      .action = MRP_ACTION_S_NEW |
                                                MRP_ACTION_REQ_TX},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_AN,
                                      .action = MRP_ACTION_S_NEW |
                                                MRP_ACTION_REQ_TX},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_VN,
                                      .action = MRP_ACTION_REQ_TX}
        },
        [MRP_APPLICANT_AN] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_AN},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_AN},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_LA,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_R_NEW]       = {.state = MRP_APPLICANT_AN},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_APPLICANT_AN},
            [MRP_EVENT_R_IN]        = {.state = MRP_APPLICANT_AN},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_APPLICANT_AN},
            [MRP_EVENT_R_MT]        = {.state = MRP_APPLICANT_AN},
            [MRP_EVENT_R_LV]        = {.state = MRP_APPLICANT_VN,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_R_LA]        = {.state = MRP_APPLICANT_VN,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_APPLICANT_VN,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_PERIODIC]    = {.state = MRP_APPLICANT_AN},
            [MRP_EVENT_TX]          = {.state = MRP_APPLICANT_QA,
                                      .action = MRP_ACTION_S_NEW},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_QA,
                                      .action = MRP_ACTION_S_NEW},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_VN,
                                      .action = MRP_ACTION_REQ_TX}
        },
        [MRP_APPLICANT_AA] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_AA},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_LA,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_R_NEW]       = {.state = MRP_APPLICANT_AA},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_APPLICANT_QA},
            [MRP_EVENT_R_IN]        = {.state = MRP_APPLICANT_QA},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_APPLICANT_AA},
            [MRP_EVENT_R_MT]        = {.state = MRP_APPLICANT_AA},
            [MRP_EVENT_R_LV]        = {.state = MRP_APPLICANT_VP,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_R_LA]        = {.state = MRP_APPLICANT_VP,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_APPLICANT_VP,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_PERIODIC]    = {.state = MRP_APPLICANT_AA},
            [MRP_EVENT_TX]          = {.state = MRP_APPLICANT_QA,
                                      .action = MRP_ACTION_S_JOIN},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_QA,
                                      .action = MRP_ACTION_S_JOIN},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_VP,
                                      .action = MRP_ACTION_REQ_TX}
        },
        [MRP_APPLICANT_QA] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_QA},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_LA,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_R_NEW]       = {.state = MRP_APPLICANT_QA},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_APPLICANT_QA},
            [MRP_EVENT_R_IN]        = {.state = MRP_APPLICANT_QA},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_APPLICANT_AA,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_R_MT]        = {.state = MRP_APPLICANT_AA,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_R_LV]        = {.state = MRP_APPLICANT_VP,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_R_LA]        = {.state = MRP_APPLICANT_VP,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_APPLICANT_VP,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_PERIODIC]    = {.state = MRP_APPLICANT_AA,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_TX]          = {.state = MRP_APPLICANT_QA},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_QA,
                                      .action = MRP_ACTION_S_JOIN},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_VP,
                                      .action = MRP_ACTION_REQ_TX}
        },
        [MRP_APPLICANT_LA] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_AA,
                                      .action = MRP_ACTION_REQ_TX},
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
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_LO,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_LO,
                                      .action = MRP_ACTION_REQ_TX}
        },
        [MRP_APPLICANT_AO] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_AP,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_AO},
            [MRP_EVENT_R_NEW]       = {.state = MRP_APPLICANT_AO},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_APPLICANT_QO},
            [MRP_EVENT_R_IN]        = {.state = MRP_APPLICANT_AO},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_APPLICANT_AO},
            [MRP_EVENT_R_MT]        = {.state = MRP_APPLICANT_LO,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_R_LV]        = {.state = MRP_APPLICANT_LO,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_R_LA]        = {.state = MRP_APPLICANT_LO,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_APPLICANT_LO,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_PERIODIC]    = {.state = MRP_APPLICANT_AO},
            [MRP_EVENT_TX]          = {.state = MRP_APPLICANT_AO},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_LO,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_LO,
                                      .action = MRP_ACTION_REQ_TX}
        },
        [MRP_APPLICANT_QO] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_QP},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_QO},
            [MRP_EVENT_R_NEW]       = {.state = MRP_APPLICANT_QO},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_APPLICANT_QO},
            [MRP_EVENT_R_IN]        = {.state = MRP_APPLICANT_QO},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_APPLICANT_AO},
            [MRP_EVENT_R_MT]        = {.state = MRP_APPLICANT_AO},
            [MRP_EVENT_R_LV]        = {.state = MRP_APPLICANT_LO,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_R_LA]        = {.state = MRP_APPLICANT_LO,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_APPLICANT_LO,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_PERIODIC]    = {.state = MRP_APPLICANT_QO},
            [MRP_EVENT_TX]          = {.state = MRP_APPLICANT_QO},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_LO,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_LO,
                                      .action = MRP_ACTION_REQ_TX}
        },
        [MRP_APPLICANT_AP] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_AP},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_AO},
            [MRP_EVENT_R_NEW]       = {.state = MRP_APPLICANT_AP},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_APPLICANT_QP},
            [MRP_EVENT_R_IN]        = {.state = MRP_APPLICANT_AP},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_APPLICANT_AP},
            [MRP_EVENT_R_MT]        = {.state = MRP_APPLICANT_AP},
            [MRP_EVENT_R_LV]        = {.state = MRP_APPLICANT_VP,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_R_LA]        = {.state = MRP_APPLICANT_VP,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_APPLICANT_VP,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_PERIODIC]    = {.state = MRP_APPLICANT_AP},
            [MRP_EVENT_TX]          = {.state = MRP_APPLICANT_QA,
                                      .action = MRP_ACTION_S_JOIN},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_QA,
                                      .action = MRP_ACTION_S_JOIN},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_VP,
                                      .action = MRP_ACTION_REQ_TX}
        },
        [MRP_APPLICANT_QP] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_QP},
            [MRP_EVENT_LV]          = {.state = MRP_APPLICANT_QO},
            [MRP_EVENT_R_NEW]       = {.state = MRP_APPLICANT_QP},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_APPLICANT_QP},
            [MRP_EVENT_R_IN]        = {.state = MRP_APPLICANT_QP},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_APPLICANT_AP,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_R_MT]        = {.state = MRP_APPLICANT_AP,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_R_LV]        = {.state = MRP_APPLICANT_VP,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_R_LA]        = {.state = MRP_APPLICANT_VP,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_APPLICANT_VP,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_PERIODIC]    = {.state = MRP_APPLICANT_AP,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_TX]          = {.state = MRP_APPLICANT_QP},
            [MRP_EVENT_TX_LA]       = {.state = MRP_APPLICANT_QA,
                                      .action = MRP_ACTION_S_JOIN},
            [MRP_EVENT_TX_LAF]      = {.state = MRP_APPLICANT_VP,
                                      .action = MRP_ACTION_REQ_TX}
        },
        [MRP_APPLICANT_LO] = {
            [MRP_EVENT_NEW]         = {.state = MRP_APPLICANT_VN,
                                      .action = MRP_ACTION_REQ_TX},
            [MRP_EVENT_JOIN]        = {.state = MRP_APPLICANT_VP,
                                      .action = MRP_ACTION_REQ_TX},
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
                                      .action = MRP_ACTION_NEW},
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
                                      .action = MRP_ACTION_NEW |
                                                MRP_ACTION_STOP_LEAVE_TIMER},
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
            [MRP_EVENT_LEAVE_TIMER] = {.state = MRP_REGISTRAR_L3,
                                      .action = MRP_ACTION_START_LEAVE_TIMER}
        },
        [MRP_REGISTRAR_L3] = {
            [MRP_EVENT_R_NEW]       = {.state = MRP_REGISTRAR_IN,
                                      .action = MRP_ACTION_NEW |
                                                MRP_ACTION_STOP_LEAVE_TIMER},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_REGISTRAR_IN,
                                      .action = MRP_ACTION_STOP_LEAVE_TIMER},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_REGISTRAR_IN,
                                      .action = MRP_ACTION_STOP_LEAVE_TIMER},
            [MRP_EVENT_R_LV]        = {.state = MRP_REGISTRAR_L3},
            [MRP_EVENT_R_LA]        = {.state = MRP_REGISTRAR_L3},
            [MRP_EVENT_TX_LA]       = {.state = MRP_REGISTRAR_L3},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_REGISTRAR_L3},
            [MRP_EVENT_FLUSH]       = {.state = MRP_REGISTRAR_MT,
                                      .action = MRP_ACTION_LV},
            [MRP_EVENT_LEAVE_TIMER] = {.state = MRP_REGISTRAR_L2,
                                      .action = MRP_ACTION_START_LEAVE_TIMER}
        },
        [MRP_REGISTRAR_L2] = {
            [MRP_EVENT_R_NEW]       = {.state = MRP_REGISTRAR_IN,
                                      .action = MRP_ACTION_NEW |
                                                MRP_ACTION_STOP_LEAVE_TIMER},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_REGISTRAR_IN,
                                      .action = MRP_ACTION_STOP_LEAVE_TIMER},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_REGISTRAR_IN,
                                      .action = MRP_ACTION_STOP_LEAVE_TIMER},
            [MRP_EVENT_R_LV]        = {.state = MRP_REGISTRAR_L2},
            [MRP_EVENT_R_LA]        = {.state = MRP_REGISTRAR_L2},
            [MRP_EVENT_TX_LA]       = {.state = MRP_REGISTRAR_L2},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_REGISTRAR_L2},
            [MRP_EVENT_FLUSH]       = {.state = MRP_REGISTRAR_MT,
                                      .action = MRP_ACTION_LV},
            [MRP_EVENT_LEAVE_TIMER] = {.state = MRP_REGISTRAR_L1,
                                      .action = MRP_ACTION_START_LEAVE_TIMER}
        },
        [MRP_REGISTRAR_L1] = {
            [MRP_EVENT_R_NEW]       = {.state = MRP_REGISTRAR_IN,
                                      .action = MRP_ACTION_NEW |
                                                MRP_ACTION_STOP_LEAVE_TIMER},
            [MRP_EVENT_R_JOIN_IN]   = {.state = MRP_REGISTRAR_IN,
                                      .action = MRP_ACTION_STOP_LEAVE_TIMER},
            [MRP_EVENT_R_JOIN_MT]   = {.state = MRP_REGISTRAR_IN,
                                      .action = MRP_ACTION_STOP_LEAVE_TIMER},
            [MRP_EVENT_R_LV]        = {.state = MRP_REGISTRAR_L1},
            [MRP_EVENT_R_LA]        = {.state = MRP_REGISTRAR_L1},
            [MRP_EVENT_TX_LA]       = {.state = MRP_REGISTRAR_L1},
            [MRP_EVENT_REDECLARE]   = {.state = MRP_REGISTRAR_L1},
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

/* LeaveAll State Table (See 802.1ak-2007 Table 10.5)
   Note: the leaveAllTimer causes a transmit opportunity to be requested when
   it expires */
static const struct mrp_state_trans
    mrp_leaveall_state_table[MRP_LEAVEALL_MAX][MRP_EVENT_MAX] = {
        [MRP_LEAVEALL_A] = {
            [MRP_EVENT_TX]             = {.state = MRP_LEAVEALL_P,
                                         .action = MRP_ACTION_S_LA},
            [MRP_EVENT_R_LA]           = {.state = MRP_LEAVEALL_P,
                                         .action = MRP_ACTION_START_LEAVEALL_TIMER},
            [MRP_EVENT_LEAVEALL_TIMER] = {.state = MRP_LEAVEALL_A,
                                         .action = MRP_ACTION_START_LEAVEALL_TIMER |
                                                   MRP_ACTION_REQ_TX}
        },
        [MRP_LEAVEALL_P] = {
            [MRP_EVENT_TX]             = {.state = MRP_LEAVEALL_P},
            [MRP_EVENT_R_LA]           = {.state = MRP_LEAVEALL_P,
                                         .action = MRP_ACTION_START_LEAVEALL_TIMER},
            [MRP_EVENT_LEAVEALL_TIMER] = {.state = MRP_LEAVEALL_A,
                                         .action = MRP_ACTION_START_LEAVEALL_TIMER |
                                                   MRP_ACTION_REQ_TX}
        }
    };

/* Periodic State Table (See 802.1ak-2007 Table 10.6) */
static const struct mrp_state_trans
    mrp_periodic_state_table[MRP_PERIODIC_MAX][MRP_EVENT_MAX] = {
        [MRP_PERIODIC_A] = {
            [MRP_EVENT_PERIODIC_ENABLED]  = {.state = MRP_PERIODIC_A},
            [MRP_EVENT_PERIODIC_DISABLED] = {.state = MRP_PERIODIC_P},
            [MRP_EVENT_PERIODIC_TIMER]    = {.state = MRP_LEAVEALL_A,
                                            .action = MRP_ACTION_PERIODIC |
                                                      MRP_ACTION_START_PERIODIC_TIMER}
        },
        [MRP_PERIODIC_P] = {
            [MRP_EVENT_PERIODIC_ENABLED]  = {.state = MRP_PERIODIC_A,
                                            .action = MRP_ACTION_START_PERIODIC_TIMER},
            [MRP_EVENT_PERIODIC_DISABLED] = {.state = MRP_PERIODIC_P},
            [MRP_EVENT_PERIODIC_TIMER]    = {.state = MRP_PERIODIC_P}
        }
    };

/* Default timer parameter values (centiseconds) */
static int join_time     = 20;
static int leaveall_time = 1000;
static int periodic_time = 100;
/* leave time takes random value between 60 and 100 centisec at mrp_init */
static int leave_time;

inline static void mad_init_machine(struct mad_machine *m)
{
    m->app_state = MRP_APPLICANT_VO;
    m->reg_state = MRP_REGISTRAR_MT;

    m->app_mgt = MRP_NORMAL_PARTICIPANT;
    m->reg_mgt = MRP_NORMAL_REGISTRATION;
}

/* Request a transmission opportunity. 'A request for a transmit opportunity
   starts a randomized join timer, and the transmit opportunity is offered
   when the timer expires. When operPointToPointMAC is TRUE, transmit
   opportunities are scheduled immediately on request' */
static void mrp_req_tx_opportunity(struct mrp_participant *p)
{
    if (!p->join_timer_running) {
        timer_start(&p->join_timeout,
            p->port->point_to_point ? 0:random_val(0, join_time));
        p->join_timer_running = 1;
    }
}

/* Return true in case the given attribute is registered in another context
   participant but is not declared in this participant. See 802.1ak 10.3.d */
static int map_propagates_to(struct mrp_participant *p,
                             struct map_context *c,
                             int mid)
{
    return (c->members[mid] > 0) && !mad_declared_here(p, mid);
}

/* Find the context with the given cid identifier
   @return pointer to context, or NULL if none found */
struct map_context *mrp_find_context(struct mrp_application *app, int cid)
{
    struct map_context *c;

	list_for_each_entry(c, &app->contexts, app_context)
        if (c->cid == cid)
            return c;
    return NULL;
}

/* Find the participant that is attached to the given port and context
   @return pointer to participant, or NULL if none found */
struct mrp_participant *mrp_find_participant(struct mrp_port *port,
                                             struct map_context *ctx)
{
    struct mrp_participant *p;
    struct map_context_list *node;

	list_for_each_entry(p, &port->participants, port_participant) {
	    list_for_each_entry(node, &p->contexts, node) {
	        if (node->context == ctx)
	            return p;
	    }
	}
    return NULL;
}

/* Find the port with the given hardware index
   @return pointer to port, or NULL if none found */
struct mrp_port *mrp_find_port(struct mrp_application *app, int hw_index)
{
    struct mrp_port *port;

    list_for_each_entry(port, &app->ports, app_port)
        if (port->hw_index == hw_index)
            return port;
    return NULL;
}

/* MAD_Join.request(attribute_type, attribute_value, new) */
static void mad_join_req(struct mrp_participant *p, int mid, int is_new)
{
    mad_attr_event(p, mid, is_new ? MRP_EVENT_NEW:MRP_EVENT_JOIN);
    if (is_new)
        p->port->app->mad_new_declaration(p, mid);
}

/* MAD_Leave.request(attribute_type, attribute_value) */
static void mad_leave_req(struct mrp_participant *p, int mid)
{
    mad_attr_event(p, mid, MRP_EVENT_LV);
}

/* Propagates a join in all the participants for the given context.
   The join needs to be propagated if either (a) this is the first participant
   in the context to register membership, or (b) there is another participant
   in the context registering membership, but no further port that would cause
   a join request to that port.
   The is_new parameter signals that the attribute value is being newly
   declared, or is being redeclared following a change in the underlying topology.
   If the value of tcDetected (RSTP/MSTP timer) for the port and context is
   non-zero, then the value of the new parameter in propagated MAD_Join.request
   is set to TRUE, regardless of the value of this parameter in the indication
   or request that is being propagated. */
static void map_propagate_join_in_context(struct mrp_participant *this_part,
                                          struct map_context *ctx,
                                          int mid)
{
    struct mrp_participant_list *node;

    int joining_members;
    struct mrp_participant *to_part;
    struct mrp_application *app = this_part->port->app;

    /* Propagation only occurs if originating port is in forwarding state */
    if (!list_find_port(&ctx->forwarding_ports, this_part->port))
        return;

    /* Avoid increasing joining_members in case case of New re-declarations */
    joining_members = mad_registered_here(this_part, mid) ?
                      ctx->members[mid]:
                      ++ctx->members[mid];

    if (joining_members > 2)
        return;

    list_for_each_entry(node, &ctx->participants, node) {
	    to_part = node->participant;
        if (to_part == this_part)
            continue;
        /* Propagation only occurs if destination port is in forwarding state */
        if (!list_find_port(&ctx->forwarding_ports, to_part->port))
            continue;
        if ((joining_members == 1) || mad_registered_here(to_part, mid))
            mad_join_req(to_part, mid, app->tc_detected(to_part->port, ctx));
    }
}

static void map_propagate_leave_in_context(struct mrp_participant *this_part,
                                           struct map_context *ctx,
                                           int mid)
{
    struct mrp_participant_list *node;
    struct mrp_participant *to_part;
    int remaining_members;

    /* Propagation only occurs if originating port is in forwarding state */
    if (!list_find_port(&ctx->forwarding_ports, this_part->port))
        return;

    remaining_members = --(ctx->members[mid]);
    if (remaining_members > 1)
        return;

    list_for_each_entry(node, &ctx->participants, node) {
	    to_part = node->participant;
        if (to_part == this_part)
            continue;
        /* Propagation only occurs if destination port is in forwarding state */
        if (!list_find_port(&ctx->forwarding_ports, to_part->port))
            continue;
        if ((remaining_members == 0) || mad_registered_here(to_part, mid))
            mad_leave_req(to_part, mid);
    }
}

/* Propagates a join in all the contexts associated to the participant.
    @param this_part pointer to the participant that initiated the propagation
   @param mid attribute index */
void map_propagate_join(struct mrp_participant *this_part, int mid)
{
    struct map_context_list *node;
    list_for_each_entry(node, &this_part->contexts, node)
        map_propagate_join_in_context(this_part, node->context, mid);
}


/* Propagates a leave in all the contexts associated to the participant.
   The leave needs to be propagated if either (a) this is the last participant
   in the propagation context to register membership, or (b) there is one other
   participant in the context registering membership, in which case the leave
   request needs to be sent to that port alone.
   @param this_part pointer to the participant that initiated the propagation
   @param mid attribute index */
void map_propagate_leave(struct mrp_participant *this_part, int mid)
{
    struct map_context_list *node;

    list_for_each_entry(node, &this_part->contexts, node)
        map_propagate_leave_in_context(this_part, node->context, mid);
}

int mad_attr_event(struct mrp_participant *p,
                    int mid,
                    enum mrp_event event)
{
    uint8_t state, action;
    int is_new = 0;     //  is_new = 1 for new attribute declarations
    int in;             //  in = 1 when the registrar is in IN state
    struct mrp_port *port = p->port;
    struct mrp_application *app = port->app;
    struct mad_machine *machine = &p->machines[mid];

//registrar:

    /* Fixed    : Registrar ignores all MRP messages, and remains IN
       Forbidden: Registrar ignores all MRP messages, and remains MT */
    if ((machine->reg_mgt == MRP_FIXED_REGISTRATION) ||
        (machine->reg_mgt == MRP_FORBIDDEN_REGISTRATION))
        goto applicant;

    /* Update registrar state _before_ computing applicant state */
    state  = mrp_registrar_state_table[machine->reg_state][event].state;
    if (state == MRP_REGISTRAR_INVALID)
        goto applicant;

    action = mrp_registrar_state_table[machine->reg_state][event].action;
    switch (action & MRP_ACTION_MASK) {
    case MRP_ACTION_NEW:
        is_new = 1;
        /* fall through */
    case MRP_ACTION_JOIN:
        if (app->mad_join_ind(p, mid, is_new) < 0) {
            port->reg_failures++;
            break;
        }
        map_propagate_join(p, mid);
        break;
    case MRP_ACTION_LV:
        app->mad_leave_ind(p, mid);
        map_propagate_leave(p, mid);
        break;
    }

    switch (action & MRP_TIMER_ACTION_MASK) {
    case MRP_ACTION_START_LEAVE_TIMER:
        if (!p->leave_timer_running) {
            timer_start(&p->leave_timeout_4, leave_time / 4);
            p->leave_timer_running = 1;
        }
        break;
    case MRP_ACTION_STOP_LEAVE_TIMER:;
        // leave timer handled at participant level
        break;
    }

    machine->reg_state = state;

applicant:

    in = (machine->reg_state == MRP_REGISTRAR_IN);

    /* applicant state machine */
    state  = mrp_applicant_state_table[machine->app_state][event].state;
    if (state == MRP_APPLICANT_INVALID)
        return 0;

    action = mrp_applicant_state_table[machine->app_state][event].action;

    /* some state transitions depend on additional conditions (as defined in
       notes attached to the applicant state table in 802.1ak) */
    switch (machine->app_state) {
    case MRP_APPLICANT_VO:
    case MRP_APPLICANT_VP:
        /* Ignore transitions to states AO and AP when operPointToPointMAC
           is TRUE (802.1ak 2007 Table 10.3 Note 3) */
        if ((event == MRP_EVENT_R_JOIN_IN) && (port->point_to_point))
            return 0;
        break;
    case MRP_APPLICANT_AA:
        /* Transition from AA to QA due to rIn! is Ignored (no transition) if
           operPointToPointMAC is FALSE (802.1ak 2007 Table 10.3 Note 5)*/
        if ((event == MRP_EVENT_R_IN) && (!port->point_to_point))
            return 0;
        break;
    case MRP_APPLICANT_AN:
        /* Upon tx! event, transition from AN to QA if the Registrar
           is IN, and AA otherwise (802.1ak 2007 Table 10.3 Note 8) */
        if ((event == MRP_EVENT_TX) && !in) {
            state = MRP_APPLICANT_AA;
            action |= MRP_ACTION_REQ_TX;
        }
        break;
    default:;
    }

    /* The Applicant Administrative Control, determines whether or not the
       Applicant state machine participates in MRP protocol exchanges.*/
    if (machine->app_mgt == MRP_NON_PARTICIPANT)
        goto non_participant;

//normal_participant:

    /* Note 7: If the PDU is full, state remains unchanged. This way the
       message will be sent in the next tx opportunity (which is requested) */

    /* If Registration Fixed or Forbidden, In and JoinIn messages are
       sent rather than Empty or JoinEmpty messages */
    in |= (machine->reg_mgt != MRP_NORMAL_REGISTRATION);
    switch (action & MRP_ACTION_MASK) {
    case MRP_ACTION_S_JOIN:
        if (mrp_pdu_append_attr(p, mid, in ? MRP_JOIN_IN:MRP_JOIN_MT) < 0) {
            mrp_req_tx_opportunity(p);
            return -1;
        }
        break;
    case MRP_ACTION_S_NEW:
        if (mrp_pdu_append_attr(p, mid, MRP_NEW) < 0) {
            mrp_req_tx_opportunity(p);
            return -1;
        }
        break;
    case MRP_ACTION_S_LV:
        if (mrp_pdu_append_attr(p, mid, MRP_LV) < 0) {
            mrp_req_tx_opportunity(p);
            return -1;
        }
        break;
    case MRP_ACTION_S:
        if (mrp_pdu_append_attr(p, mid, in ? MRP_IN:MRP_MT) < 0) {
            mrp_req_tx_opportunity(p);
            return -1;
        }
        break;
    }

    /* 802.1ak 2007 Table 10.3 Note 6.- Request opportunity to transmit on entry
       to VN, AN, AA, LA, VP, AP, and LO states. */
    if (action & MRP_REQ_TX_MASK)
        mrp_req_tx_opportunity(p);

non_participant:
    machine->app_state = state;
    return 0;
}

/* Apply the given event to state machines for all Attributes of attrtype.
   @param attrtype attribute type */
void mad_attrtype_event(struct mrp_participant *p,
                        uint8_t attrtype,
                        enum mrp_event event)
{
    struct mrp_application *app = p->port->app;
    int size = app->numattr;
    int begin = p->next_to_process;
    int i, mid;

    for (i = 0; i < size; i++) {
        mid = (begin + i) % size;
        if (mad_machine_active(p, mid) && (app->attrtype(mid) == attrtype)) {
            if (mad_attr_event(p, mid, event) < 0) {
                /* PDU is full */
                p->next_to_process = mid;
                return;
            }
        }
    }
}

/* Apply the given event to state machines for all Attributes (...that are
   of interest for the participant, i.e. have been declared or registered) */
static void mad_event(struct mrp_participant *p, enum mrp_event event)
{
    int i, mid;
    int begin = p->next_to_process;
    int size = p->port->app->numattr;

    /* Handle attr event considering a circular state machine list */
    for (i = 0; i < size; i++) {
        mid = (begin + i) % size;
        if (mad_machine_active(p, mid)) {
            if (mad_attr_event(p, mid, event) < 0) {
                /* PDU is full */
                p->next_to_process = mid;
                return;
            }
        }
    }
}

void mad_participant_event(struct mrp_participant *p, enum mrp_event event)
{
    uint8_t state;
    uint8_t action;

    state = mrp_leaveall_state_table[p->leaveall_state][event].state;
    if (state == MRP_LEAVEALL_INVALID)
        return;

    action = mrp_leaveall_state_table[p->leaveall_state][event].action;
    switch (action & MRP_ACTION_MASK) {
    case MRP_ACTION_S_LA:
        /* The LeaveAll event value specified by the transmission action is
           encoded in the NumberOfValues field of the VectorAttribute as
           specified. The sLA action also gives rise to a rLA! event against all
           instances of the Applicant state machine, or the Registrar state
           machine, associated with the MRP participant. */
        p->leaveall = 1; /* signal that the leaveall event ocurred */
        mad_event(p, MRP_EVENT_R_LA);
    }
    switch (action & MRP_TIMER_ACTION_MASK) {
    case MRP_ACTION_START_LEAVEALL_TIMER:
        /* 'The Leave All Period Timer is set to a random value in the
           range LeaveAllTime < T < 1.5*LeaveAllTime when it is started' */
        timer_start(&p->leaveall_timeout,
            random_val(leaveall_time,  3 * leaveall_time / 2));
    }

    if (action & MRP_REQ_TX_MASK)
        mrp_req_tx_opportunity(p);


    p->leaveall_state = state;
}

void mad_port_event(struct mrp_port *port, enum mrp_event event)
{
    uint8_t state, action;
    struct mrp_participant *p;

    state = mrp_periodic_state_table[port->periodic_state][event].state;
    if (state == MRP_PERIODIC_INVALID)
        return;

    action = mrp_periodic_state_table[port->periodic_state][event].action;
    switch (action & MRP_ACTION_MASK) {
    case MRP_ACTION_PERIODIC:
        /* 'Causes a periodic! event against all Applicant state machines
            associated with the participant(s)' attached to the port */
	    list_for_each_entry(p, &port->participants, port_participant)
            mad_event(p, MRP_EVENT_PERIODIC);
    }
    switch (action & MRP_TIMER_ACTION_MASK) {
    case MRP_ACTION_START_PERIODIC_TIMER:
        timer_start(&port->periodic_timeout, periodic_time);
    }

    port->periodic_state = state;
}

/* Adds a port to the set of ports in forwarding state for the given context
   @return 0 if succeeded. -1 otherwise */
int map_context_add_port(struct map_context *ctx, struct mrp_port *port)
{
    struct mrp_participant *p;
    struct mrp_application *app = port->app;
    int mid, max = app->numattr;

    struct mrp_port_list *port_node;

    port_node = malloc(sizeof(*port_node));
    if (!port_node)
        return -1;
    
    port_node->port = port;
    INIT_LIST_HEAD(&port_node->node);
    
    list_add(&port_node->node, &ctx->forwarding_ports);

    p = mrp_find_participant(port, ctx);
    if (!p)
        return -1;

    for (mid = 0; mid < max; mid++) {
        /* 'If a Port is added to the set, and that Port has registered an
        attribute then MAD_Join.requests are propagated to the MAD instances for
        each of the other Ports in the set.'*/
        if (mad_registered_here(p, mid))
            map_propagate_join_in_context(p, ctx, mid);

        /* 'If a Port is added to the set, but that Port has not declared an
        attribute that other Ports in the set have registered, then
        MAD_Join.requests are propagated by those other Ports to the MAD
        instance for that Port.'*/
        if (map_propagates_to(p, ctx, mid))
            mad_join_req(p, mid, app->tc_detected(port, ctx));
    }
    return 0;
}

/* Removes a port to the set of ports in forwarding state for the given context
   @return 0 if succeeded. -1 otherwise */
void map_context_remove_port(struct map_context *c, struct mrp_port *port)
{
    struct mrp_participant *p;
    int mid, max = port->app->numattr;
    struct mrp_port_list *port_node;


    p = mrp_find_participant(port, c);
    if (p) {
        /* If a Port is removed from the set, and that Port has registered an
        attribute and no other Port has, then MAD_Leave.requests are propagated
        to the MAD instances for each of the other Ports in the set.*/
        /* If a Port is removed from the set, and that Port has registered an
        attribute that another Port has also registered, then a
        MAD_Leave.request is propagated to the MAD instance for that other
        Port. */
        for (mid = 0; mid < max; mid++)
            if (mad_registered_here(p, mid))
                map_propagate_leave_in_context(p, c, mid);

        /* If a Port is removed from the set, and that Port has declared one or more
        attributes, then this Port transmits a Leave message for every
        attribute that it has declared. (affects only to Active applicants) */
        for (mid = 0; mid < max; mid++) {
            if (mad_declared_here(p, mid))
                mad_attr_event(p, mid, MRP_EVENT_LV);
        }
    }

    port_node = list_find_port(&c->forwarding_ports, port);
    if (port_node)
        list_del(&port_node->node);
}


/* Add a participant to a propagation context.
   @return 0 if succeeded. -1 if failed. */
int map_context_add_participant(struct map_context *c,
                                struct mrp_participant *p)
{
    struct map_context_list *cnode;
    struct mrp_participant_list *pnode;

    cnode = malloc(sizeof(cnode));
    if (!cnode)
        return -1;
    cnode->context = c;

    pnode = malloc(sizeof(*pnode));
    if (!pnode)
        goto nomem;
    pnode->participant = p;

    INIT_LIST_HEAD(&cnode->node);
    INIT_LIST_HEAD(&pnode->node);

    /* Add context to participant */
    list_add(&cnode->node, &p->contexts);
    /* Add participant to context */
    list_add(&pnode->node, &c->participants);

    return 0;

nomem:
    free(cnode);
    return -1;
}

/* Remove a participant from a propagation context */
void map_context_remove_participant(struct map_context *c,
                                    struct mrp_participant *p)
{
    struct mrp_participant_list *pnode;
    struct map_context_list *cnode;

    /* Delete participant from context */
    pnode = list_find_participant(&c->participants, p);
    if (pnode)
        list_del(&pnode->node);

    /* Delete context from participant */
    cnode = list_find_context(&p->contexts, c);
    if (cnode)
        list_del(&cnode->node);
}

/* Create a participant for the given MRP application.
   @return pointer to participant or NULL if failed */
struct mrp_participant *mrp_create_participant(struct mrp_port *port)
{
    struct mrp_application *app = port->app;
    struct mrp_participant *p;
    int i;

    p = (struct mrp_participant*)malloc(sizeof(struct mrp_participant));
    if (!p)
        return p;

    p->machines = (struct mad_machine*)
        malloc(sizeof(struct mad_machine) * app->numattr);
    if (!p->machines)
        goto nomem;

    for (i = 0; i < app->numattr; i++)
        mad_init_machine(&p->machines[i]);

    INIT_LIST_HEAD(&p->contexts);
    INIT_LIST_HEAD(&p->port_participant);

    p->port             = port;
    p->leaveall         = 0;
    p->next_to_process  = 0;

    p->join_timer_running   = 0;
    p->leave_timer_running  = 0;

    /* Init pdu and transmission window */
    mrp_pdu_init(&p->pdu, app->proto.tagged);
    memset(p->tx_window, 0, 3 * sizeof(struct timespec));

    /* leaveall state machine and timer */
    p->leaveall_state = MRP_LEAVEALL_P;
    timer_start(&p->leaveall_timeout,
        random_val(leaveall_time,  (3 * leaveall_time) / 2));

    /* Add participant to the port */
    list_add(&p->port_participant, &port->participants);

    if (app->init_participant(p) < 0)
        goto err;

    return p;

err:
    free(p->machines);
nomem:
    free(p);
    return NULL;
}

/* Delete participant and remove it from its port and any propagation context */
void mrp_destroy_participant(struct mrp_participant *p)
{
    struct map_context_list *cnode;

    if (!p)
        return;

    p->port->app->uninit_participant(p);

    /* Remove participant from propagation contexts */
    while (!list_empty(&p->contexts)) {
        cnode = list_first_entry(&p->contexts, struct map_context_list, node);
        map_context_remove_participant(cnode->context, p);
    }

    /* Remove participant from port */
    list_del(&p->port_participant);

    free(p->machines);
    free(p);
}

/* Delete port and remove it from the application */
void mrp_destroy_port(struct mrp_port *port)
{
    struct mrp_participant *p;

    if (!port)
        return;

    /* Delete all port participants */
	list_for_each_entry(p, &port->participants, port_participant)
        mrp_destroy_participant(p);

    /* Remove port from application */
    list_del(&port->app_port);

    free(port);
}

/* Create a context with the given identifier. Add propagation context to app.
   @return pointer to context or NULL if failed. */
struct map_context *map_context_create(int cid, struct mrp_application *app)
{
    struct map_context *ctx;

    ctx = (struct map_context*)malloc(sizeof(struct map_context));
    if (!ctx)
        return NULL;

    ctx->cid = cid;

    INIT_LIST_HEAD(&ctx->participants);
    INIT_LIST_HEAD(&ctx->forwarding_ports);
    INIT_LIST_HEAD(&ctx->app_context);

    ctx->members = malloc(sizeof(int) * app->numattr);
    if (!ctx->members)
        goto nomem;

    memset(ctx->members, 0, sizeof(int) * app->numattr);
    list_add(&ctx->app_context, &app->contexts);
    return ctx;

nomem:
    free(ctx);
    return NULL;
}

/* Delete context and remove it from the application */
void map_context_destroy(struct map_context *ctx, struct mrp_application *app)
{
    struct mrp_participant_list *part_node;
    struct mrp_port_list *port_node;

    if (!ctx)
        return;

    /* Remove all participants from context */
    list_for_each_entry(part_node, &ctx->participants, node)
        map_context_remove_participant(ctx, part_node->participant);

    /* Remove all ports from forwarding port set */
    list_for_each_entry(port_node, &ctx->forwarding_ports, node)
        list_del(&port_node->node);

    /* Remove context from application */
    list_del(&ctx->app_context);

    free(ctx->members);
    free(ctx);
}

static struct mrp_port *mrp_create_port(struct mrp_application *app,
                                        int hw_index,
                                        int port_no)
{
    struct mrp_port *port;
    struct ifreq ifr;

    port = (struct mrp_port *)malloc(sizeof(*port));
    if (!port)
        return NULL;

    port->app = app;
    /* Set port as point to point (default).
       Protocol works correcty even if port is not point to point */
    port->point_to_point = 1;

    /* Begin! periodic state machine and timer */
    port->periodic_state = MRP_PERIODIC_A;
    timer_start(&port->periodic_timeout, periodic_time);

    INIT_LIST_HEAD(&port->participants);
    INIT_LIST_HEAD(&port->app_port);

    port->hw_index = hw_index;
    port->port_no = port_no;
    port->reg_failures = 0;

    /* Get port address. First we need to get the name of the interface and then
       we can get its HW address ('Normally, the user specifies which device to
       affect by setting ifr_name... and given the ifr_ifindex, the SIOCGIFNAME
       is the only ioctl which returns its results in ifr_name') */
    ifr.ifr_ifindex = hw_index;
    if ((ioctl(app->proto.fd, SIOCGIFNAME,   &ifr) == -1) ||
	    (ioctl(app->proto.fd, SIOCGIFHWADDR, &ifr) == -1)) {
        fprintf(stderr, "mrp: unable to get HW address; IOCTL error.");
        free(port);
        return NULL;
    }
    memcpy(port->hwaddr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

    /* Add port to application port list */
    list_add(&port->app_port, &app->ports);

    port->is_enabled = 1;

    return port;
}

/* Open packet socket to send and receive MRP PDUs for a given application,
   and makes sure that RTU delivers application packets to NIC.
   Important: Application ports and contexts should have been already set up.
   @param app contains MRP application configuration (ether_type, address)
   @return -1 if error. 0 otherwise.*/
int mrp_register_application(struct mrp_application *app)
{
    hexp_port_list_t port_list;
    int err, i, hw_index, port_no;

    /* Open socket */
    if (mrp_open_socket(app) < 0)
        return -1;

    /* Get port list from HAL */

    if (hal_get_port_list(&port_list) < 0)
        goto fail;

    INIT_LIST_HEAD(&app->ports);

    /* For each existing interface, create port with appropriate index and number */
    for (i = 0; i < HAL_MAX_PORTS; i++) {
        if (!port_list.port_names[i])
            continue;

        hw_index = if_nametoindex(port_list.port_names[i]);
        if (hw_index == 0)
            continue;

        port_no = wr_nametoport(port_list.port_names[i]);
        if (port_no < 0) {
            fprintf(stderr, "mrp: read interface port number failed\n");
            goto fail;
        }

        if (!mrp_create_port(app, hw_index, port_no)) {
            fprintf(stderr, "mrp: not enough memory to create new port\n");
            goto fail;
        }
    }

    /* Make sure RTU delivers MRP application packets to NIC */
    errno = 0;
    err = rtu_fdb_proxy_create_static_entry(
            app->proto.address,
            WILDCARD_VID,
            0x400,              /*egress*/
            0xfffffbff,         /*forbidden*/
            ST_VOLATILE,
            ACTIVE,
            !app->proto.tagged  /* is bpdu */);
    if (errno) {
        perror("rtu_fdb_proxy_create_static_entry");
        goto fail;
    }
    if (err) {
        fprintf(stderr, "create static entry failed err %d\n", err);
        goto fail;
    }

    return 0;

fail:
    while (!list_empty(&app->ports))
        list_del(app->ports.next);
    mrp_close_socket(app);
    return -1;
}

int mrp_unregister_application(struct mrp_application *app)
{
    int err;
    struct mrp_port *port;
    struct map_context *context;

    close(app->proto.fd);

    /* Delete all ports (will remove participants) */
    list_for_each_entry(port, &app->ports, app_port)
        mrp_destroy_port(port);

    /* Delete all contexts */
    list_for_each_entry(context, &app->contexts, app_context)
        map_context_destroy(context, app);

    /* Make sure MRP app packets are forwarded again on all ports */
    errno = 0;
    err = rtu_fdb_proxy_delete_static_entry(app->proto.address, WILDCARD_VID);
    if (errno || err)
        return -1;

    return 0;
}

/* MAD_Join.request(attribute_type, attribute_value, new) issued by app */
void mrp_join_req(struct mrp_participant *p, int mid, int new)
{
    mad_join_req(p, mid, new);
    map_propagate_join(p, mid);
}

/* MAD_Leave.request(attribute_type, attribute_value) primitive from app */
void mrp_leave_req(struct mrp_participant *p, int mid)
{
    mad_leave_req(p, mid);
    map_propagate_leave(p, mid);
}

/* Used by the application to notify that a topology change has been detected,
   and there is a need to rapidly deregister information on the port associated
   with the registrar state machine (in participant p) */
void mrp_flush(struct mrp_participant *p)
{
    mad_event(p, MRP_EVENT_FLUSH);
    mad_participant_event(p, MRP_EVENT_LEAVEALL_TIMER);
}

/* Used by the application to notify that a topology change has been detected,
   and there is a need to rapidly redeclare registered information on the port
   associated with the applicant and registrar state machines (in participant p) */
void mrp_redeclare(struct mrp_participant *p)
{
    mad_event(p, MRP_EVENT_REDECLARE);
}

/* Check whether the tx rate has been exceeded (i.e. there has been more than 3
   tx opportunities in 1,5*JoinTime)
   @param p pointer to MRP participant
   @return !0 if the tx rate has been exceeded; 0 otherwise. */
static int mrp_tx_rate_exceeded(struct mrp_participant *p)
{
    struct timespec now, window;

    window = p->tx_window[0];
    timespec_add_cs(&window, 3 * join_time / 2);
    return timespec_compare(timer_now(&now), &window) < 0;
}

/* Register that a tx has ocurred for the given participant.
   @param p pointer to MRP participant */
static void mrp_tx_reg(struct mrp_participant *p)
{
    /* Shift tx window */
    p->tx_window[0] = p->tx_window[1];
    p->tx_window[1] = p->tx_window[2];
    timer_now(p->tx_window + 2);
    /* Reset LeaveAll event */
    p->leaveall = 0;

    mrp_pdu_init(&p->pdu, p->port->app->proto.tagged);
}

/* Checks whether a transmission opportunity should be offered to the
   participant (i.e. once the join timer has expired).
   If operPointToPointMAC is TRUE, a transmission opportunity is inmediately
   offered subject to no more than three transmission opportunities in any
   period of 1.5*JoinTime'
   If operPointToPointMAC is FALSE, and there is no pending request, a transmit
   opportunity is scheduled at a time value randomized between 0 and JoinTime
   @return !0 if a tx opportunity is offered. 0 otherwise */
static int mrp_tx_opportunity(struct mrp_participant *p, struct timespec *now)
{
    if (p->join_timer_running) {
        if (timer_expired_now(&p->join_timeout, now)) {
            p->join_timer_running = 0;
            return p->port->point_to_point ? !mrp_tx_rate_exceeded(p):1;
        }
    } else {
        if (!p->port->point_to_point) {
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

    /* Trigger per-participant tx! event */
    mad_participant_event(p, MRP_EVENT_TX);

    /* Determine the type of transmission event */
    event = p->leaveall ?
        (mrp_pdu_full(&p->pdu) ? MRP_EVENT_TX_LAF:MRP_EVENT_TX_LA):
        MRP_EVENT_TX;

    /* Trigger per-attribute tx! event */
    mad_event(p, event);

    if (!mrp_pdu_empty(&p->pdu)) {
        /* Send MRPDU */
        mrp_pdu_send(p);

        /* Register that tx ocurred for the participant */
        mrp_tx_reg(p);
    }
}

/* Management related */

/*static void mrp_enable_periodic_tx(struct mrp_port *port)*/
/*{*/
/*    mad_port_event(port, MRP_EVENT_PERIODIC_ENABLED);*/
/*}*/

/*static void mrp_disable_periodic_tx(struct mrp_port *port)*/
/*{*/
/*    mad_port_event(port, MRP_EVENT_PERIODIC_DISABLED);*/
/*}*/

/* Multiple Registration Protocol Handler */
void mrp_protocol(struct mrp_application *app)
{
    struct mrp_port *port;
    struct mrp_participant *p;
    struct timespec now;

    timer_now(&now);

    /* Process received PDUs */
    mrp_pdu_rcv(app);
    list_for_each_entry(port, &app->ports, app_port) {
        if (!port->is_enabled)
            continue;

        if (list_empty(&port->participants))
            continue;

        /* Handle timers */
        if (timer_expired_now(&port->periodic_timeout, &now))
            mad_port_event(port, MRP_EVENT_PERIODIC_TIMER);

	    list_for_each_entry(p, &port->participants, port_participant) {
            if (timer_expired_now(&p->leaveall_timeout, &now))
                mad_participant_event(p, MRP_EVENT_LEAVEALL_TIMER);
            /* 'The accuracy required for the leavetimer is sufficiently
               coarse to permit the use of a single operating system timer
               per Participant with 2 bits of state for each Registrar' */
            if (p->leave_timer_running &&
                timer_expired_now(&p->leave_timeout_4, &now)) {
                p->leave_timer_running = 0;
                mad_event(p, MRP_EVENT_LEAVE_TIMER);
            }
            /* Handle transimission */
            if (mrp_tx_opportunity(p, &now))
                mrp_tx(p);
        }
    }
}


void mrp_set_registrar_control(struct mrp_participant *p,
                               int mid,
                               enum mrp_registrar_mgt control)
{
    struct mad_machine *machine = &p->machines[mid];

    switch (control) {
    case MRP_FIXED_REGISTRATION:
        machine->reg_state = MRP_REGISTRAR_IN;
        break;
    case MRP_FORBIDDEN_REGISTRATION:
        machine->reg_state = MRP_REGISTRAR_MT;
    default:;
    }
    machine->reg_mgt = control;
}

void mrp_set_applicant_control(struct mrp_participant *p,
                               int mid,
                               enum mrp_applicant_mgt control)
{
    p->machines[mid].app_mgt = control;
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
    leave_time = random_val(60, 100);

    return 0;
}
