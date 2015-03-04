/* HAL public API available via WR-IPC */

#include <stdio.h>
#include <stdlib.h>

#include <trace.h>
#include <pps_gen.h> /* for direct access to DMPLL and PPS generator */

#include "wrsw_hal.h"
#include "rt_ipc.h"

#include <minipc.h>

#define HAL_EXPORT_STRUCTURES
#include "hal_exports.h" /* for exported structs/function protos */

static struct minipc_ch *hal_ch;

/* Locking call - controls the HAL locking state machine. Called by the PTPd during the WR Link Setup phase, when
   it has detected a compatible WR master. */
int halexp_lock_cmd(const char *port_name, int command, int priority)
{
	int rval;

//	TRACE(TRACE_INFO, "halexp_lock_cmd: cmd=%d port=%s tics =%lld\n", command, port_name, shw_get_tics());

	switch(command)
	{
		case HEXP_LOCK_CMD_ENABLE_TRACKING:
			return hal_enable_tracking(port_name);
			

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
	int busy;

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

/* PPS adjustment call, independent for the nanosecond (a.k.a. 8ns cycle) counter and the seconds (UTC)
   counter. The counters are adjusted by atomically adding (params->adjust_nsec/utc). Since there's a
   single PPS counter, these adjustments are port-independent. Once the coarse (16/8ns) offset is fixed,
   fine adjustments are done with ADJUST_PHASE call, independently for each uplink to accommodate
   the different phase shifts on each port (and the fiber/cable connected to it).
 */
//       return rts_adjust_phase(params->channel, params->adjust_phase_shift) ? 0 : -1;
      return rts_adjust_phase(params->channel, params->adjust_phase_shift);

    case HEXP_PPSG_CMD_ADJUST_NSEC:
      shw_pps_gen_adjust(PPSG_ADJUST_NSEC, params->adjust_nsec);
      return 0;

    case HEXP_PPSG_CMD_ADJUST_SEC:
      shw_pps_gen_adjust(PPSG_ADJUST_SEC, params->adjust_sec);
      return 0;

/* Returns non-zero if the PPS/PLL adjustment is in progress.
   WARNING: timestamps of the packets sent/received during the PPS counter adjustment are VERY LIKELY
   to be broken. Currently, the servo in PTPd just introduces a dumb, 2-second delay after each adjustment,
   to make sure following packets will have already their timestamps generated using the updated counter.

   fixme: the NIC driver should check the status of the PPS generator adjustment and if it detects
   a pending adjustment it shall not timestamp any packets, so the PTPd will simply ignore them during
   delay calculation. */

    case HEXP_PPSG_CMD_POLL:
			busy = shw_pps_gen_busy() || hal_phase_shifter_busy();
	    return busy ? 0 : 1;

    case HEXP_PPSG_CMD_SET_VALID:
        return shw_pps_gen_enable_output(params->pps_valid);

    }
  return -1; /* fixme: real error code */
}

extern int any_port_locked(void);

int halexp_get_timing_state(hexp_timing_state_t *state)
{
    state->timing_mode = hal_get_timing_mode(); // this is the mode of wrsw_hal, determined by wrsw_hal.config
    state->locked_port = any_port_locked();     // this is state from hal_ports.c, reflects softPLL with delay

    return 0;
}
extern int active_port(void);

/* communicatin between PPSi and wrsw_hal and SoftPLL, mainly getting state
 * NOTE: reading the flags clears them in SoftPLL (it is kind-of synchronization of state)
 */
int halexp_swover_cmd(int cmd, int channel, hexp_backup_state_t *s)
{
	struct rts_bpll_state state;
	s->phase_good_val =  0;
	s->flags          =  0;
	s->active_chan    = -1;
	switch(cmd)
	{
	  case HEXP_BCKP_CMD_GET_ACTIVE:
		s->active_chan = active_port();
	  break;
	  case HEXP_BCKP_CMD_GET_STATE :
		rts_get_backup_state(&state,channel);
		s->phase_good_val = state.phase_good_val;
		s->flags = state.flags;
	  break;
	  default:
		TRACE(TRACE_INFO, "[halexp_swover_cmd:err] ups, wrong cmd=%d\n", cmd);
		return -1;
	}
	return 0;
}

/* communicatin between PPSi and wrsw_hal and SoftPLL,
 * 1. getting state of holdover by PPSi
 * 2. controlling holdover by wr_phytool
 */
int halexp_hdover_cmd(int cmd, int value, hexp_holdover_state_t *s)
{
	struct rts_hdover_state state;
	int ret=0;
	int ret_state = 0;	
	s->data_valid =  0;
	switch(cmd)
	{
		case HEXP_HDOVER_CMD_GET_STATE :


		if((ret = rts_get_holdover_state(&state,0)) < 0)
		{
		   TRACE(TRACE_INFO, "[HEXP_HDOVER_CMD_GET_STATE] no data,ret=%d",ret);
		   return ret;
		}
		
		TRACE(TRACE_INFO, "[HEXP_HDOVER_CMD_GET_STATE] state=%d, hd_time=%d, enabled=%d",
		state.state, state.hd_time, state.enabled);
		s->data_valid     =  1;
		s->enabled        = state.enabled;
		ret_state         = state.state;
		s->type           = state.type;
		s->hd_time        = state.hd_time;
		s->flags          = state.flags;
		switch(ret_state)
		{
			case RTS_HOLDOVER_DISABLED:
			case RTS_HOLDOVER_ACQUIRING:
			case RTS_HOLDOVER_READY:
			default:
			s->state = HEXP_HDOVER_INACTIVE;
			TRACE(TRACE_INFO, "  HEXP_HDOVER_INACTIVE [%d]\n",ret_state);
			break;
			case RTS_HOLDOVER_ACTIVE_IN:
			s->state = HEXP_HDOVER_ACTIVE;
			TRACE(TRACE_INFO, "  RTS_HOLDOVER_ACTIVE_IN [%d]\n",ret_state);
			break;
			case RTS_HOLDOVER_ACTIVE_OUT:
			s->state = HEXP_HDOVER_OUTSIDE_SPEC;
			TRACE(TRACE_INFO, "  RTS_HOLDOVER_ACTIVE_OUT [%d]\n",ret_state);
			break;
		}
		break;
		default:
		TRACE(TRACE_INFO, "[halexp_hdover_cmd:err] ups, wrong cmd=%d\n", cmd);
		return -1;
	}
	return 0;
}


static void hal_cleanup_wripc()
{
	minipc_close(hal_ch);
}

/* The functions to manage packet/args conversions */
static int export_pps_cmd(const struct minipc_pd *pd,
			  uint32_t *args, void *ret)
{
	int rval;

	/* First argument is command next is param structure */
	rval = halexp_pps_cmd(args[0], (hexp_pps_params_t *)(args + 1));
	*(int *)ret = rval;
	return 0;
}

static int export_get_port_state(const struct minipc_pd *pd,
				 uint32_t *args, void *ret)
{
	hexp_port_state_t *state = ret;

	return halexp_get_port_state(state, (char *)args /* name */);
}

static int export_lock_cmd(const struct minipc_pd *pd,
				 uint32_t *args, void *ret)
{
	int rval;
	char *pname = (void *)args;

	/* jump over the string */
	args = minipc_get_next_arg(args, pd->args[0]);

	rval = halexp_lock_cmd(pname, args[0] /* cmd */, args[1] /* prio */);
	*(int *)ret = rval;
	return 0;
}

static int export_swover_cmd(const struct minipc_pd *pd, uint32_t *args, void *ret)
{
	hexp_backup_state_t *state = ret;
	return halexp_swover_cmd(args[0] /*cmd*/,args[1] /*channel*/, state);
}

static int export_hdover_cmd(const struct minipc_pd *pd, uint32_t *args, void *ret)
{
	hexp_holdover_state_t *state = ret;
	return halexp_hdover_cmd(args[0] /*cmd*/,args[1] /*value*/, state);
}

static int export_query_ports(const struct minipc_pd *pd,
			      uint32_t *args, void *ret)
{
	hexp_port_list_t *list = ret;
	halexp_query_ports(list);
	return 0;
}

static int export_get_timing_state(const struct minipc_pd *pd,
			      uint32_t *args, void *ret)
{
	halexp_get_timing_state(ret);
	return 0;
}

/* Creates a wripc server and exports all public API functions */
int hal_init_wripc()
{
	hal_ch = minipc_server_create(WRSW_HAL_SERVER_ADDR, 0);


	if(hal_ch < 0)
	{
		TRACE(TRACE_ERROR, "Failed to create mini-rpc server '%s'", WRSW_HAL_SERVER_ADDR);
		return -1;
	}
	/* NOTE: check_running is not remotely called, so I don't export it */

	/* fill the function pointers */
	__rpcdef_pps_cmd.f = export_pps_cmd;                 //wrs-time.c
	__rpcdef_get_port_state.f = export_get_port_state;   //wrs-socket.c  & main-loop.c
	__rpcdef_lock_cmd.f = export_lock_cmd;               //wrs-time.c
	__rpcdef_query_ports.f = export_query_ports;
	__rpcdef_get_timing_state.f = export_get_timing_state; 
	__rpcdef_swover_cmd.f = export_swover_cmd;           //wrs-time.c
	__rpcdef_hdover_cmd.f = export_hdover_cmd;           //wrs-main-loop.c

	minipc_export(hal_ch, &__rpcdef_pps_cmd);
	minipc_export(hal_ch, &__rpcdef_get_port_state);
	minipc_export(hal_ch, &__rpcdef_lock_cmd);
	minipc_export(hal_ch, &__rpcdef_query_ports);
	minipc_export(hal_ch, &__rpcdef_get_timing_state);
	minipc_export(hal_ch, &__rpcdef_swover_cmd);
	minipc_export(hal_ch, &__rpcdef_hdover_cmd);


	/* FIXME: pll_cmd is empty anyways???? */

	hal_add_cleanup_callback(hal_cleanup_wripc);

	TRACE(TRACE_INFO, "Started mini-rpc server '%s'", WRSW_HAL_SERVER_ADDR);

	return 0;
}

/* wripc update function, must be called in the main program loop */
int hal_update_wripc()
{
	minipc_server_action(hal_ch, 10 /* ms */);
	return 0;
}


/* Returns 1 if there's already an instance of the HAL running. Used to prevent from
   launching multiple HALs simultaneously. */
int hal_check_running()
{
	struct minipc_ch *ch;

	ch = minipc_client_create(WRSW_HAL_SERVER_ADDR, 0);
	if (!ch)
		return 0;
	minipc_close(ch);
	return 1;
}
