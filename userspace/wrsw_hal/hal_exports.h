#ifndef __HAL_EXPORTS_H
#define __HAL_EXPORTS_H

#include <stdint.h>

#define HAL_MAX_PORTS 64

/* Where do we listen for wripc requests */
#define WRSW_HAL_SERVER_ADDR "wrsw_hal"

/* Calibration call commands, handled by halexp_calibration_cmd() */

/* checks if the calibration unit is idle */
#define HEXP_CAL_CMD_CHECK_IDLE 1

/* enables/disables transmission of calibration patter*/
#define HEXP_CAL_CMD_TX_PATTERN 2

/* requests the measurement of DeltaTX */
#define HEXP_CAL_CMD_TX_MEASURE 4

/*requests a measurement of RX delta */
#define HEXP_CAL_CMD_RX_MEASURE 5

/* get the raw delta_rx/tx (totally unprocessed - expressed as raw DMTD phase) */
#define HEXP_CAL_CMD_GET_RAW_DELTA_RX 6
#define HEXP_CAL_CMD_GET_RAW_DELTA_TX 7


/* Calibration call responses */

/* Calibrator is busy */
#define HEXP_CAL_RESP_BUSY 1
#define HEXP_CAL_RESP_OK 0
#define HEXP_CAL_RESP_ERROR -1

/* Locking call commands/responses */
#define HEXP_LOCK_CMD_START 1
#define HEXP_LOCK_CMD_CHECK 2

#define HEXP_LOCK_STATUS_LOCKED 0
#define HEXP_LOCK_STATUS_BUSY 1
#define HEXP_LOCK_STATUS_NONE 2

/* PPS Generator commands */
#define HEXP_PPSG_CMD_GET 0
#define HEXP_PPSG_CMD_ADJUST_PHASE 1
#define HEXP_PPSG_CMD_ADJUST_UTC 2
#define HEXP_PPSG_CMD_ADJUST_NSEC 3
#define HEXP_PPSG_CMD_POLL 4

/* Foreseen for halexp_pll_cmd() */
#define HEXP_ON 1
#define HEXP_OFF 0

#define HEXP_HPLL 0
#define HEXP_DMPLL 1

#define HEXP_FREQ 0
#define HEXP_PHASE 1

/* Port modes (hexp_port_state_t.mode) */
#define HEXP_PORT_MODE_WR_M_AND_S 4
#define HEXP_PORT_MODE_WR_MASTER 1
#define HEXP_PORT_MODE_WR_SLAVE 2
#define HEXP_PORT_MODE_NON_WR 3

/* External clock source control commands (temporary) */
#define HEXP_EXTSRC_CMD_CHECK 0
#define HEXP_EXTSRC_STATUS_LOCKED 0 
#define HEXP_EXTSRC_STATUS_NOSRC  2



/* Number of the fractional bits in the fiber alpha coefficient - used by the PTPd servo to
   avoid floating point calculations. */
#define FIX_ALPHA_FRACBITS 40

/* PPS adjustment call parameter */
typedef struct {

/* Port to be adjusted (only relevant for phase adjustment - i.e. HEXP_PPSG_CMD_ADJUST_PHASE */
  char port_name[16];

/* Current value of the phase shift on port (port_name) */
  uint32_t current_phase_shift;

/* Phase shift adjustment in picoseconds. Positive = future, negative = past */
  int32_t adjust_phase_shift;

/* UTC/Cycle counter adjustment (seconds/nanoseconds) */
  int64_t adjust_utc;
  int32_t adjust_nsec; 

/* Current time obtained by PPSG_GET call */
  uint64_t current_utc;
  uint32_t current_nsec;
} hexp_pps_params_t;


/* Generic port state structure, filled in by halexp_get_port_state() */
typedef struct {
  /* When non-zero: port state is valid */
  int valid;

  /* WR-PTP role of the port (Master, Slave, non-WR, etc.) */
  int mode;

  /* TX and RX delays in picoseconds (combined, big Deltas from the link model in the spec). */
  uint32_t delta_tx;
  uint32_t delta_rx;

  /* DDMTD raw phase value in picoseconds */
  uint32_t phase_val;

  /* When non-zero: phase_val contains a valid phase readout */
  int phase_val_valid;

  /* When non-zero: link is up */
  int up;

  /* When non-zero: TX path is calibrated (delta_tx contains valid value) */
  int tx_calibrated;

  /* When non-zero: RX path is calibrated (delta_rx contains valid value) */
  int rx_calibrated;

  int tx_tstamp_counter;
  int rx_tstamp_counter;

  /* When non-zero: port is locked (i.e. the PLL is currently using it as a reference source) */
  int is_locked;

  /* Lock priority of the port. (0 = highest, 1 = sligtly smaller, etc). 
     Negative = the PLL will not use the port for clock recovery. */
  int lock_priority;


  /* Current PLL phase setpoint (picoseconds). If lock_priority < 0, it's irrelevant. */
  uint32_t phase_setpoint; 

  /* Reference clock period in picoseconds (8000 for WR). */
  uint32_t clock_period; 

  /* Approximate DMTD phase value (on a slave port) at which RX timestamp (T2) counter 
     transistion occurs (picoseconds). Used to merge the timestamp counter value with the phase value
     to obtain a picosecond-level t2 timestamp. See linearize_rx_timestamp() function in libptpnetif and
     Tom's M.Sc for the details of the algorithm. */
  uint32_t t2_phase_transition; 

  /* The same, but for a master port. In V3 these will be identical, as there will be no physical differences
     between the uplink and downlink ports */
  uint32_t t4_phase_transition; 

  /* MAC address of the port */
  uint8_t hw_addr[6];

  /* Hardware index of the port */
  int hw_index;

  /* Fixed-point fiber alpha parameter, used by the PTPd to calculate the asymmetry of the fiber. 
     Relevant only for ports working in Slave mode. 

	 WARNING! This is not the alpha as defined in the WR protocol specification, but a fixed point version
	 (which is NOT EXACTLY a floating point alpha from the spec directly converted to a fixed format by 
     shifting and rounding). The actual equation for obtaining fix_alpha from alpha is in hal_ports.c */
  int32_t fiber_fix_alpha;
} hexp_port_state_t;

/* Port list structure. Filled in by halexp_get_port_state() */
typedef struct {
/* number of ports in port_names[][] */
  int num_ports; 
/* array of port names (Linux network I/Fs) */
  char port_names[HAL_MAX_PORTS][16];
} hexp_port_list_t;

/* PLL command structure for halexp_pll_cmd(). To be upgraded with more parameters in the V3 */
typedef struct {
	int ki, kp;
	int pll;
	int branch;
} hexp_pll_cmd_t;

int halexp_check_running();
int halexp_reset_port(const char *port_name);
int halexp_calibration_cmd(const char *port_name, int command, int on_off);
int halexp_lock_cmd(const char *port_name, int command, int priority);
int halexp_query_ports(hexp_port_list_t *list);
int halexp_get_port_state(hexp_port_state_t *state, const char *port_name);
int halexp_pps_cmd(int cmd, hexp_pps_params_t *params);
int halexp_pll_set_gain(int pll, int branch, int kp, int ki);
int halexp_extsrc_cmd(int command); //added by ML


#endif

