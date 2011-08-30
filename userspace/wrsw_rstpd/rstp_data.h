/*
 * White Rabbit Switch RSTP
 * Copyright (C) 2011, CERN.
 *
 * Authors:     Miguel Baizán (miguel.baizan@integrasys-sa.com)
 *
 * Description: Basic RSTP structures and parameters.
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

#ifndef __WHITERABBIT_RSTP_DATA_H
#define __WHITERABBIT_RSTP_DATA_H

#include <stdint.h>
#include <linux/if_ether.h>
#include <time.h>

#include "rstp_stmch.h"


#define MAX_NUM_PORTS 10
#define NUM_STMCH_PER_PORT 9

/* Bridge ID length */
#define BRIDGE_PRIO_LEN 2 /* Number of octets */
#define BRIDGE_ID_LEN   (BRIDGE_PRIO_LEN + ETH_ALEN)

/* Types of BPDU supported */
#define BPDU_TYPE_CONFIG    0x00
#define BPDU_TYPE_TCN       0x80
#define BPDU_TYPE_RSTP      0x02

/* STP is version 0 and RSTP is version 2 */
#define RST_PROTOCOL_VERSION_IDENTIFIER 0x02

/* Masks for the flags parameter in the BPDUs */
#define TOPOLOGY_CHANGE_FLAG        0x01
#define PROPOSAL_FLAG               (0x01 << 1)
#define PORT_ROLE_FLAG              (0x03 << 2)
#define LEARNING_FLAG               (0x01 << 4)
#define FORWARDING_FLAG             (0x01 << 5)
#define AGREEMENT_FLAG              (0x01 << 6)
#define TOPOLOGY_CHANGE_ACK_FLAG    (0x01 << 7)

/* Port roles as defined in the BPDU */
#define PORT_ROLE_UNKNOWN           0
#define PORT_ROLE_ALTERNATE_BACKUP  1
#define PORT_ROLE_ROOT              2
#define PORT_ROLE_DESIGNATED        3

/* Default values for the RSTP performance parameters. See 802.1D-2004, clause
   17.14 and 9.2.8. Time values in BPDUs are encoded in units of time of
   1/256 of a second */
#define DEFAULT_MIGRATE_TIME            0x0300      /* 3 seconds */
#define DEFAULT_BRIDGE_HELLO_TIME       0x0200      /* 2 seconds */
#define DEFAULT_BRIDGE_MAX_AGE          0x1400      /* 20 seconds */
#define DEFAULT_BRIDGE_FORWARD_DELAY    0x0F00      /* 15 seconds */
#define DEFAULT_TRANSMIT_HOLD_COUNT     0x06        /* 6 */
#define DEFAULT_BRIDGE_PRIORITY         0x8000      /* 32768 */
#define DEFAULT_PORT_PRIORITY           0x80        /* 128 */

/* Ranges for the RSTP performance parameters */
#define MIN_BRIDGE_HELLO_TIME           0x0100      /* 1 second */
#define MAX_BRIDGE_HELLO_TIME           0x0200      /* 2 seconds */
#define MIN_BRIDGE_MAX_AGE              0x0600      /* 6 seconds */
#define MAX_BRIDGE_MAX_AGE              0x2800      /* 40 seconds */
#define MIN_BRIDGE_FORWARD_DELAY        0x0400      /* 4 seconds */
#define MAX_BRIDGE_FORWARD_DELAY        0x1E00      /* 30 seconds */
#define MIN_TRANSMIT_HOLD_COUNT         0x0100      /* 1 second */
#define MAX_TRANSMIT_HOLD_COUNT         0x0A00      /* 10 seconds */
#define MIN_BRIDGE_PRIORITY             0
#define MAX_BRIDGE_PRIORITY             0xF000      /* 61440 */
#define MIN_PORT_PRIORITY               0
#define MAX_PORT_PRIORITY               0xF0        /* 240 */
#define BRIDGE_PRIORITY_STEP            0x1000      /* 4096 */
#define PORT_PRIORITY_STEP              0x10        /* 16 */

/* Default Port Path Cost values. See  802.1D-2004, clause 17.14 */
#define DEFAULT_PORT_PATH_COST_100_KBPS     0xBEBC200   /* 200 000 000 */
#define DEFAULT_PORT_PATH_COST_1_MBPS       0x1312D00   /* 20 000 000 */
#define DEFAULT_PORT_PATH_COST_10_MBPS      0x01E8480   /* 2 000 000 */
#define DEFAULT_PORT_PATH_COST_100_MBPS     0x0030D40   /* 200 000 */
#define DEFAULT_PORT_PATH_COST_1_GBPS       0x0004E20   /* 20 000 */
#define DEFAULT_PORT_PATH_COST_10_GBPS      0x00007D0   /* 2 000 */
#define DEFAULT_PORT_PATH_COST_100_GBPS     0x00000C8   /* 200 */
#define DEFAULT_PORT_PATH_COST_1_TBPS       0x0000014   /* 20 */
#define DEFAULT_PORT_PATH_COST_10_TBPS      0x0000002   /* 2 */
#define MAX_PORT_PATH_COST                  0xBEBC200   /* 200000000 */

/* RSTP port flags (rstp_flags). See  802.1D-2004, clause 17.19  */
#define AGREE           0   /* 17.19.2 */
#define AGREED          1   /* 17.19.3 */
#define DISPUTED        2   /* 17.19.6 */
#define FDBFLUSH        3   /* 17.19.7 */
#define FORWARD         4   /* 17.19.8 */
#define FORWARDING      5   /* 17.19.9 */
#define LEARN           6   /* 17.19.11 */
#define LEARNING        7   /* 17.19.12 */
#define MCHECK          8   /* 17.19.13 */
#define NEWINFO         9   /* 17.19.16 */
#define OPEREDGE        10  /* 17.19.17 */
#define PORTENABLED     11  /* 17.19.18 */
#define PROPOSED        12  /* 17.19.23 */
#define PROPOSING       13  /* 17.19.24 */
#define RCVDBPDU        14  /* 17.19.25 */
#define RCVDMSG         15  /* 17.19.27 */
#define RCVDRSTP        16  /* 17.19.28 */
#define RCVDSTP         17  /* 17.19.29 */
#define RCVDTc          18  /* 17.19.30 */
#define RCVDTCACK       19  /* 17.19.31 */
#define RCVDTCN         20  /* 17.19.32 */
#define REROOT          21  /* 17.19.33 */
#define RESELECT        22  /* 17.19.34 */
#define SELECTED        23  /* 17.19.36 */
#define SENDRSTP        24  /* 17.19.38 */
#define SYNC            25  /* 17.19.39 */
#define SYNCED          26  /* 17.19.40 */
#define TCACK           27  /* 17.19.41 */
#define TCPROP          28  /* 17.19.42 */
#define TICK            29  /* 17.19.43 */
#define UPDTINFO        30  /* 17.19.45 */

/* Operations on RSTP port flags */
#define GET_PORT_FLAG(bitfield, flag)       ((bitfield >> flag) & 0x01)
#define REMOVE_PORT_FLAG(bitfield, flag)    (bitfield &= (~(0x01 << flag)))
#define SET_PORT_FLAG(bitfield, flag)       (bitfield |= (0x01 << flag))



/* Bridge ID is 8 octets long. See 802.1D, clause 9.2.5, for encoding */
struct bridge_id {
    uint8_t prio[BRIDGE_PRIO_LEN];  /* More significant */
    uint8_t addr[ETH_ALEN];         /* Less significant */
};


/* RSTP times for the bridge and port operation. See 802.1D, clause 17.14 */
struct rstp_times {
    uint16_t   message_age;
    uint16_t   max_age;
    uint16_t   hello_time;
    uint16_t   forward_delay;
};


/* Configuration BPDU. See 802.1D, clause 9.3.1. Note that the Port
   ID is 2 octets long. See 802.1D-2004, clause 9.2.7, for encoding
   (the less significant 12 bits are used to encode the Port Number, while
   the 4 more significant bits are used to encode the Port Priority) */
struct cfg_bpdu {
    uint16_t                protocol_identifier;
    uint8_t                 protocol_version_identifier;
    uint8_t                 bpdu_type;
    uint8_t                 flags;

    struct bridge_id        root_identifier;
    uint32_t                root_path_cost;
    struct bridge_id        bridge_identifier;
    uint16_t                port_identifier;

    struct rstp_times       times;
}; /* TODO: should we use: __attribute__((packed))?. Decide when BPDUs
      processing functions be written */;

/* Topology Change Notification BPDU. See 802.1D, clause 9.3.2 */
struct tcn_bpdu {
    uint16_t    protocol_identifier;
    uint8_t     protocol_version_identifier;
    uint8_t     bpdu_type;
};

/* Rapid Spanning Tree BPDU. See 802.1D, clause 9.3.3. This BPDU can act as
   a Configuration BPDU and as a TCN BPDU */
struct rstp_bpdu {
    struct cfg_bpdu configuration_bpdu;
    uint8_t         version1_length;
};


/* Spanning Tree priority vector. See 802.1D, clause 17.5 */
struct st_priority_vector {
    struct bridge_id    RootBridgeId;
    uint32_t            RootPathCost;
    struct bridge_id    TxBridgeId;
    uint16_t            TxPortId;
    uint16_t            RxPortId;
};

/* Indicate the origin/state of the Port's ST information. See 802.1D,
   clause 17.19.10*/
enum port_info {
    RECEIVED,
    MINE,
    AGED,
    DISABLED
};

enum received_info {
    SUPERIOR_DESIGNATED_INFO,
    REPEATED_DESIGNATED_INFO,
    INFERIOR_DESIGNATED_INFO,
    INFERIOR_ROOT_ALTERNATE_INFO,
    OTHER_INFO
};

enum port_role {
    ROOT_PORT,
    DESIGNATED_PORT,
    ALTERNATE_PORT,
    BACKUP_PORT,
    DISABLED_PORT
};

/* Values that the Force Protocol Version management parameter can take */
enum protocol_version {
    STP_COMPATIBLE = 0,
    RSTP_NORMAL_OPERATION = 2
};


/* RSTP performance parameters per port. These parameters are not modified by
the operation of RSTP, but are treated as constants. They may be modified by
management. See 802.1D, clause 17.13 */
struct rstp_port_mng_data {
    int PortEnabled;        /* The Administrative status of the Port. If True(1)
                               the ST is enabled for this port */
    int AdminEdgePort;      /* 17.13.1 */ /* There's another related parameter
                                             called AutoEdgePort. It's optional
                                             and provided only for bridges that
                                             support the identification of edge
                                             ports. Not implemented for now. */
    uint8_t     PortPriority;   /* 17.13.10 */
    uint32_t    PortPathCost;   /* 17.13.11 and 17.19.20 */

};

/* Per-Port variables. See 802.1D, clause 17.17 and 17.19 */
struct port_data {
    uint8_t                     port_number;
    char                        port_name[16];
    unsigned int                link_status;    /* When non-zero: link is up */

    struct state_machine        stmch[NUM_STMCH_PER_PORT];

    struct rstp_port_mng_data   mng;

    /* RSTP data.
     * Per-Port RSTP variables. The state machines will use the information
     * stored in these variables to compute the state of the port. See 802.1D,
     * clause 17.19 and 17.17
    */
    /* TODO The ageingTime is mantained by the RTU and it's the same for all the
       ports. In a future we should probably have an independent instance for
       each port */
    /*unsigned long               ageingTime;*/     /* 17.19.1 */

    struct st_priority_vector   designatedPriority; /* 17.19.4 */
    struct rstp_times           designatedTimes;    /* 17.19.5 */
    struct st_priority_vector   msgPriority;        /* 17.19.14 */
    struct rstp_times           msgTimes;           /* 17.19.15 */
    struct st_priority_vector   portPriority;       /* 17.19.21 */
    struct rstp_times           portTimes;          /* 17.19.22 */

    uint8_t                     txCount;        /* 17.19.44 */
    uint16_t                    portId;         /* 17.19.19 */
    enum port_info              infoIs;         /* 17.19.10 */
    enum received_info          rcvdInfo;       /* 17.19.26 */
    enum port_role              role;           /* 17.19.35 */
    enum port_role              selectedRole;   /* 17.19.37 */

    /* RSTP port flags */
    uint32_t                    rstp_flags;     /* Flags defined above. Use the
                                                   macros to get/set/remove
                                                   each flag */
    /* Timers */
    uint16_t edgeDelayWhile;    /* 17.17.1 */
    uint16_t fdWhile;           /* 17.17.2 */
    uint16_t helloWhen;         /* 17.17.3 */
    uint16_t mdelayWhile;       /* 17.17.4 */
    uint16_t rbWhile;           /* 17.17.5 */
    uint16_t rcvdInfoWhile;     /* 17.17.6 */
    uint16_t rrWhile;           /* 17.17.7 */
    uint16_t tcWhile;           /* 17.17.8 */
};


/* RSTP performance parameters per bridge. These parameters are not modified by
the operation of RSTP, but are treated as constants. They may be modified by
management. See 802.1D, clause 17.13 */
struct rstp_bridge_mng_data {
    struct bridge_id        BridgeIdentifier;       /* 17.18.2 and 17.13.7 */

    uint16_t                BridgeForwardDelay;     /* 17.13.5 */
    uint16_t                BridgeHelloTime;        /* 17.13.6 */
    uint16_t                BridgeMaxAge;           /* 17.13.8 */
    uint16_t                MigrateTime;            /* 17.13.9 */
    enum protocol_version   forceVersion;           /* 17.13.4 */
    uint8_t                 TxHoldCount;            /* 17.13.12 */
};

/* Per-Bridge variables. See 802.1D, clause 17.18 */
struct bridge_data {
    struct port_data            ports[MAX_NUM_PORTS];
    struct state_machine        stmch;      /* STMCH per bridge */

    struct rstp_bridge_mng_data mng;

    int                         begin;              /* 17.18.1 */
    struct st_priority_vector   BridgePriority;     /* 17.18.3 */
    struct rstp_times           BridgeTimes;        /* 17.18.4 */
    uint16_t                    rootPortId;         /* 17.18.5 */
    struct st_priority_vector   rootPriority;       /* 17.18.6 */
    struct rstp_times           rootTimes;          /* 17.18.7 */
};



/* Declare bridge data structure */
extern struct bridge_data br;


/*** FUNCTIONS ***/
int init_data(void);
void one_second(void);


#endif /* __WHITERABBIT_RSTP_DATA_H */
