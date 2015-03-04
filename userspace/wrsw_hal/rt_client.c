/*
 * Example mini-ipc client
 *
 * Copyright (C) 2011 CERN (www.cern.ch)
 * Author: Alessandro Rubini <rubini@gnudd.com>
 *
 * Released in the public domain
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#include "minipc.h"

#define RTIPC_EXPORT_STRUCTURES
#include "rt_ipc.h"

#define RTS_MAILBOX_ADDR "mem:1000F000"

#define RTS_TIMEOUT 200 /* ms */

static struct minipc_ch *client;

//#define VERBOSE

/* Queries the RT CPU PLL state */
int rts_get_state(struct rts_pll_state *state)
{
	int i, ret = minipc_call(client, RTS_TIMEOUT, &rtipc_rts_get_state_struct, state);

    if(ret < 0)
        return ret;


    state->current_ref = (state->current_ref);
    state->backup_ref = (state->backup_ref);
    state->flags = (state->flags);
    state->holdover_duration = (state->holdover_duration);
    state->mode = (state->mode);

    for(i=0; i<RTS_PLL_CHANNELS;i++)
    {
        state->channels[i].priority = (state->channels[i].priority);
        state->channels[i].phase_setpoint = (state->channels[i].phase_setpoint);
        state->channels[i].phase_current = (state->channels[i].phase_current);
        state->channels[i].phase_loopback = (state->channels[i].phase_loopback);
        state->channels[i].flags = (state->channels[i].flags);
    }


#ifdef VERBOSE
    printf("RTS State Dump: \n");
    printf("CurrentRef: %d Mode: %d Flags: %x\n", state->current_ref, state->mode, state->flags);
    for(i=0;i<RTS_PLL_CHANNELS;i++)
        printf("Ch%d: setpoint: %dps current: %dps loopback: %dps flags: %x\n", i,
               state->channels[i].phase_setpoint,
               state->channels[i].phase_current,
               state->channels[i].phase_loopback,
               state->channels[i].flags);

#endif
    return 0;
}

/* Sets the RT subsystem mode (Boundary Clock or Grandmaster) */
int rts_set_mode(int mode)
{
	int rval;
	int ret = minipc_call(client, RTS_TIMEOUT, &rtipc_rts_set_mode_struct, &rval, mode);

    if(ret < 0)
        return ret;

    return rval;
}

/* Manage the backup channel */
int rts_backup_channel(int channel, int cmd)
{
	int rval;
	int ret = minipc_call(client, RTS_TIMEOUT, &rtipc_rts_backup_channel_struct, &rval, 
			      channel, cmd);

    if(ret < 0)
        return ret;

    return rval;
}

/* Get state of the backup stuff, i.e. good phase, switchover & update notifications */
int rts_get_backup_state(struct rts_bpll_state *s, int channel)
{
	int ret = minipc_call(client, RTS_TIMEOUT, &rtipc_rts_get_backup_state_struct,
			      s,channel);

	if(ret < 0)
		return ret;

	s->flags          = (s->flags);
	s->phase_good_val = (s->phase_good_val);
	s->active_chan    = (s->active_chan);
    return 0;
}


int rts_is_holdover(void)
{
	int rval;
	int ret = minipc_call(client, RTS_TIMEOUT, &rtipc_rts_is_holdover_struct, &rval);
    printf("[rt_client.c: rts_is_holdover] call rv %d client %p rval %d errno=%d\n", ret, client, rval,errno);
    if(ret < 0)
        return ret;

    return rval;
}

/* Get state of the holdover stuff, i.e.  */
int rts_get_holdover_state(struct rts_hdover_state *s, int value)
{
	struct rts_hdover_state state = {0,0,0,0,0};
	s->enabled          = 0;
	s->state            = 0;
	s->type             = 0;
	s->hd_time          = 0;
	s->flags            = 0;
	int ret = minipc_call(client, RTS_TIMEOUT, &rtipc_rts_get_holdover_state_struct,&state,value);
	
	printf("call rv %d client %p state %p errno=%d\n", ret, client, s,errno);
	if(ret < 0)
		return ret;
	printf("state=%d, hd_time=%d, enabled=%d errno=%d\n",state.state, state.hd_time, 
	state.enabled, errno);
	s->enabled          = (state.enabled);
	s->state            = (state.state);
	s->type             = (state.type);
	s->hd_time          = (state.hd_time);
	s->flags            = (state.flags);
	printf("state=%d, hd_time=%d, enabled=%d errno=%d\n",s->state, s->hd_time, s->enabled, errno);
    return 0;
}

/* Sets the phase setpoint on a given channel */
int rts_adjust_phase(int channel, int32_t phase_setpoint)
{
  int rval;
	int  ret = minipc_call(client, RTS_TIMEOUT, &rtipc_rts_adjust_phase_struct, &rval, channel, phase_setpoint);

    if(ret < 0)
        return ret;

    return rval;
}

/* Reference channel configuration (BC mode only) */
int rts_lock_channel(int channel, int priority)
{
    int rval;
		int ret = minipc_call(client, RTS_TIMEOUT, &rtipc_rts_lock_channel_struct, &rval, channel,priority);

    if(ret < 0)
        return ret;

    return rval;
}

int rts_enable_ptracker(int channel, int enable)
{
    int rval;
		int ret = minipc_call(client, RTS_TIMEOUT, &rtipc_rts_enable_ptracker_struct, &rval, channel, enable);

    if(ret < 0)
        return ret;

    return rval;
}

int rts_debug_command(int command, int value)
{
    int rval;
		int ret = minipc_call(client, RTS_TIMEOUT, &rtipc_rts_debug_command_struct, &rval, command, value);

    if(ret < 0)
        return ret;

    return rval;
}


int rts_connect()
{
	client = minipc_client_create(RTS_MAILBOX_ADDR, /*MINIPC_FLAG_VERBOSE*/0);
	if (!client)
        return -1;
	return 0;
}

