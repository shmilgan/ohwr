/* HAL public API available via WR-IPC */

#include <stdio.h>
#include <stdlib.h>

#include <hw/trace.h>
#include <hw/dmpll.h> /* for direct access to DMPLL */

#include "wrsw_hal.h"
#include "hal_exports.h" /* for exported structs/function protos */

#include <wr_ipc.h>

static wripc_handle_t hal_ipc;

/* Dummy WRIPC export, used by the clients to check if the HAL is responding */
int halexp_check_running()
{
	return 1;
}

/* External synchronization source (i.e. GPS 10 MHz input) control. */
int halexp_extsrc_cmd(int command)
{
	int rval;
	
	switch(command)
	{
/* There's only one command so far: checking if a valid reference clock is present on the external input
   and whether the PLL is locked to the network recovered clock or to the external reference. */

		case HEXP_EXTSRC_CMD_CHECK:
			rval = hal_extsrc_check_lock();
			if(rval > 0)
				return HEXP_EXTSRC_STATUS_LOCKED;
			else if (!rval)
				return HEXP_LOCK_STATUS_BUSY;
			else
				return HEXP_EXTSRC_STATUS_NOSRC;
			break;
	}
	
	return -100; /* fixme: add real error code */
}

/* Dummy reset call (used to be a real reset for testing, but it's no longer necessary) */
int halexp_reset_port(const char *port_name)
{
  TRACE(TRACE_INFO, "resetting port %s\n", port_name);
  return 0;
}

/* Locking call - controls the HAL locking state machine. Called by the PTPd during the WR Link Setup phase, when
   it has detected a compatible WR master. */
int halexp_lock_cmd(const char *port_name, int command, int priority)
{
	int rval;


	switch(command)
	{
/* Start locking - i.e. tell the HAL locking state machine to use the port (port_name) as the source of the reference 
   frequency. (priority) parameter allows to distinguish between various reference sources and establish
   a switchover order. For example when  wr0, wr1, wr2 have respectively priorities (1, 0, 2), 
   the primary clock source is wr1. When it dies (e.g. rats ate the fiber), the PLL will automatically 
   switch to wr1, and if wr1 dies, to wr2. When all the ports are down, the PLL will switch to holdover
   mode. In V3, calling this command with negative (priority) removes the port from the locking list.
*/
		case HEXP_LOCK_CMD_START:
			return hal_port_start_lock(port_name, priority);

/* Check if locked - called by the PTPd repeatedly after calling "Start locking" to check if the 
   PLL has already locked to and stabilized the reference frequency */
		case HEXP_LOCK_CMD_CHECK:
			rval = hal_port_check_lock(port_name);

			if(rval > 0)
				return HEXP_LOCK_STATUS_LOCKED;
			else if (!rval)
				return HEXP_LOCK_STATUS_BUSY;
			else
				return HEXP_LOCK_STATUS_NONE;
			break;
	}

	return -100; /* fixme: add real error code */
}


/* Phase/Clock adjutsment call. Called by the PTPd servo. Controls both the PLLs and the PPS Generator. */
int halexp_pps_cmd(int cmd, hexp_pps_params_t *params)
{

  switch(cmd)
    {
/* fixme: TODO: implement HEXP_PPSG_CMD_GET call */

/* Phase adjustment call: adjusts the phase shift between the uplink port (port_name) and the 
   local VCTCXO clock by adding a number of picoseconds given in (params->adjust_phase_shift) to the
   current phase setpoint (i.e. when adjust is positive, the resulting ref clock/PPS goes a bit into 
   future, it if's negative - it rolls back into past). Note that to have a seamless swictchover,
   the phase shifts for different uplinks have to be coherent (i.e. the phase of the uplink clock + 
   its adjustment shall result in the same VCTCXO phase. Keeping the coherency between the phase 
   setpoints for different uplinks is the task of the PTPd.*/

    case HEXP_PPSG_CMD_ADJUST_PHASE:
      shw_dmpll_phase_shift(params->port_name, params->adjust_phase_shift);
      return 0;

/* PPS adjustment call, independent for the nanosecond (a.k.a. 8ns cycle) counter and the seconds (UTC)
   counter. The counters are adjusted by atomically adding (params->adjust_nsec/utc). Since there's a 
   single PPS counter, these adjustments are port-independent. Once the coarse (8ns) offset is fixed,
   fine adjustments are done with ADJUST_PHASE call, independently for each uplink to accommodate
   the different phase shifts on each port (and the fiber/cable connected to it).
 */
 
    case HEXP_PPSG_CMD_ADJUST_NSEC:
      shw_pps_gen_adjust_nsec(params->adjust_nsec);
      return 0;

    case HEXP_PPSG_CMD_ADJUST_UTC:
      shw_pps_gen_adjust_utc(params->adjust_utc);
      return 0;

/* Returns non-zero if the PPS/PLL adjustment is in progress.
   WARNING: timestamps of the packets sent/received during the PPS counter adjustment are VERY LIKELY
   to be broken. Currently, the servo in PTPd just introduces a dumb, 2-second delay after each adjustment,
   to make sure following packets will have already their timestamps generated using the updated counter.

   fixme: the NIC driver should check the status of the PPS generator adjustment and if it detects
   a pending adjustment it shall not timestamp any packets, so the PTPd will simply ignore them during
   delay calculation. */
 
    case HEXP_PPSG_CMD_POLL:
      return shw_dmpll_shifter_busy(params->port_name) || shw_pps_gen_busy();
    }
}

/* PLL debug call, foreseen for live adjustment of some internal PLL parameters (gains, timeouts, etc.)
   To be implemented. */
int halexp_pll_cmd(int cmd, hexp_pll_cmd_t *params)
{
}

static void hal_cleanup_wripc()
{
	wripc_close(hal_ipc);
}


/* Creates a wripc server and exports all public API functions */
int hal_init_wripc()
{
	hal_ipc = wripc_create_server(WRSW_HAL_SERVER_ADDR);


	if(hal_ipc < 0)
		return -1;

	wripc_export(hal_ipc, T_INT32, "halexp_pps_cmd", halexp_pps_cmd, 2, T_INT32, T_STRUCT(hexp_pps_params_t));
	wripc_export(hal_ipc, T_INT32, "halexp_pll_cmd", halexp_pll_cmd, 2, T_INT32, T_STRUCT(hexp_pll_cmd_t));
	wripc_export(hal_ipc, T_INT32, "halexp_check_running", halexp_check_running, 0);
	wripc_export(hal_ipc, T_STRUCT(hexp_port_state_t), "halexp_get_port_state", halexp_get_port_state, 1, T_STRING);
	wripc_export(hal_ipc, T_INT32, "halexp_calibration_cmd", halexp_calibration_cmd, 3, T_STRING, T_INT32, T_INT32);
	wripc_export(hal_ipc, T_INT32, "halexp_lock_cmd", halexp_lock_cmd, 3, T_STRING, T_INT32, T_INT32);
	wripc_export(hal_ipc, T_STRUCT(hexp_port_list_t), "halexp_query_ports", halexp_query_ports, 0);


	hal_add_cleanup_callback(hal_cleanup_wripc);

	TRACE(TRACE_INFO, "Started WRIPC server '%s'", WRSW_HAL_SERVER_ADDR);

	return 0;
}

/* wripc update function, must be called in the main program loop */
int hal_update_wripc()
{
	return wripc_process(hal_ipc);
}


/* Returns 1 if there's already an instance of the HAL running. Used to prevent from
   launching multiple HALs simultaneously. */
int hal_check_running()
{
	wripc_handle_t fd;

	fd = wripc_connect(WRSW_HAL_SERVER_ADDR);

	if(fd >= 0)
	{
		wripc_close(fd);
		return 1;
	}
	return 0;

}
