#ifndef __RT_IPC_H
#define __RT_IPC_H

#include <stdint.h>

#define RTS_PLL_CHANNELS 18

/* Individual channel flags */
/* Reference input frequency valid */
#define CHAN_REF_VALID (1<<0)
/* Frequency out of range */
#define CHAN_FREQ_OUT_OF_RANGE (1<<1)
/* Phase is drifting too fast */
#define CHAN_DRIFTING (1<<2)
/* Channel phase measurement is ready */
#define CHAN_PMEAS_READY (1<<3)
/* Channel not available/disabled */
#define CHAN_DISABLED (1<<4)
/* Channel is busy adjusting phase */
#define CHAN_SHIFTING (1<<5)
/* Channel is busy adjusting phase */
#define CHAN_PTRACKER_ENABLED (1<<6)

/* DMTD clock is present */
#define RTS_DMTD_LOCKED (1<<0)

/* 125 MHz reference locked */
#define RTS_REF_LOCKED (1<<1)

/* External 10 MHz reference present */
#define RTS_EXT_10M_VALID (1<<2)

/* External 1-PPS present */
#define RTS_EXT_PPS_VALID (1<<3)

/* External 10 MHz frequency out-of-range */
#define RTS_EXT_10M_OUT_OF_RANGE (1<<4)

/* External 1-PPS frequency out-of-range */
#define RTS_EXT_PPS_OUT_OF_RANGE (1<<5)

/* Holdover mode active */
#define RTS_HOLDOVER_ACTIVE (1<<6)

/* Grandmaster mode active (uses 10 MHz / 1-PPS reference) */
#define RTS_MODE_GM_EXTERNAL 1

/* Free-running grandmaster (uses local TCXO) */
#define RTS_MODE_GM_FREERUNNING 2

/* Boundary clock mode active (uses network reference) */
#define RTS_MODE_BC 3

/* PLL disabled */
#define RTS_MODE_DISABLED 4

/* backup slave*/
#define RTS_MODE_BACKUP_SLAVE 5

/* pseudo-lock the backup channel */
#define RTS_BACKUP_CH_LOCK     1

/* switchover from failed active channel to the backup one */
#define RTS_BACKUP_CH_ACTIVATE 2

/* the backup channel was lost (link went down) */
#define RTS_BACKUP_CH_DOWN     3

/* null reference input */
#define REF_NONE 255

/* RT Subsystem debug commands, handled via rts_debug_command() */

/* Serdes reference clock enable/disable */
#define RTS_DEBUG_ENABLE_SERDES_CLOCKS 1

struct rts_pll_state {

/* State of an individual input channel (i.e. switch port) */
	struct channel {
		/* Switchover priority: 0 = highest, 1 - 254 = high..low, 255 = channel disabled (a master port) */
		uint32_t priority;
		/* channel phase setpoint in picoseconds. Used only when channel is a slave. */
		int32_t phase_setpoint;
		/* current phase shift in picoseconds. Used only when channel is a slave. */
		int32_t phase_current;
		/* TX-RX Loopback phase measurement in picoseconds. */
		int32_t phase_loopback;
		/* TX-Rx looback stable measurement of the phase*/
		int32_t phase_good_val;
		/* flags (per channel - see CHAN_xxx defines) */
		uint32_t flags;
	} channels[RTS_PLL_CHANNELS];

	/* flags (global - RTS_xxx defines) */
	uint32_t flags;

	/* duration of current holdover period in 10us units */
	int32_t holdover_duration;

	/* current reference source - or REF_NONE if free-running or grandmaster */
	uint32_t current_ref;

	/* backup reference source - or REF_NONE if no backup */
	uint32_t backup_ref;

	/* mode of operation (RTS_MODE_xxx) */
	uint32_t mode;

	uint32_t delock_count;

	uint32_t ipc_count;
	
	uint32_t debug_data[8];
	
	uint32_t switchover_ocured;
};

/* API */

/* Connects to the RT CPU */
int rts_connect();

/* Queries the RT CPU PLL state */
int rts_get_state(struct rts_pll_state *state);

/* Sets the phase setpoint on a given channel */
int rts_adjust_phase(int channel, int32_t phase_setpoint);

/* Sets the RT subsystem mode (Boundary Clock or Grandmaster) */
int rts_set_mode(int mode);

/* Reference channel configuration (BC mode only) */
int rts_lock_channel(int channel, int priority);

/* Manage backup channel (BC mode only) */
int rts_backup_channel(int channel, int cmd);

/* Enabled/disables phase tracking on a particular port */
int rts_enable_ptracker(int channel, int enable);

/* Enabled/disables phase tracking on a particular port */
int rts_debug_command(int param, int value);

#ifdef RTIPC_EXPORT_STRUCTURES

static struct minipc_pd rtipc_rts_get_state_struct = {
	.name = "aaaa",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_STRUCT, struct rts_pll_state),
	.args = {
		MINIPC_ARG_END
	},
};

static struct minipc_pd rtipc_rts_set_mode_struct = {
	.name = "bbbb",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
	.args = {
	    MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int ),
	    MINIPC_ARG_END
	},
};

static struct minipc_pd rtipc_rts_lock_channel_struct = {
	.name = "cccc",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
	.args = {
	    MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int ),
	    MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int ),
	    MINIPC_ARG_END
	},
};

static struct minipc_pd rtipc_rts_adjust_phase_struct = {
	.name = "dddd",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
	.args = {
	    MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int ),
	    MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int ),
	    MINIPC_ARG_END
	},
};

static struct minipc_pd rtipc_rts_enable_ptracker_struct = {
	.name = "eeee",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
	.args = {
	    MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int ),
	    MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int ),
	    MINIPC_ARG_END
	},
};

static struct minipc_pd rtipc_rts_debug_command_struct = {
	.name = "ffff",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
	.args = {
	    MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int ),
	    MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int ),
	    MINIPC_ARG_END
	},
};

static struct minipc_pd rtipc_rts_backup_channel_struct = {
	.name = "xxxx",
	.retval = MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int),
	.args = {
	    MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int ),
	    MINIPC_ARG_ENCODE(MINIPC_ATYPE_INT, int ),
	    MINIPC_ARG_END
	},
};

#endif

#endif
