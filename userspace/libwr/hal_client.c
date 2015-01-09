#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <minipc.h>

#define HAL_EXPORT_STRUCTURES
#include <hal/hal_exports.h>
#include <libwr/shmem.h>
#include <libwr/hal_shmem.h>

#define DEFAULT_TO 200000	/* ms */

static struct minipc_ch *hal_ch;
static struct wrs_shm_head *hal_head;
static struct hal_port_state *hal_ports;
static int hal_nports;

int halexp_lock_cmd(const char *port_name, int command, int priority)
{
	int ret, rval;
	ret = minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_lock_cmd,
			  &rval, port_name, command, priority);
	if (ret < 0)
		return ret;
	return rval;
}

/* This used to be a mini-rpc call; it is now a shmem lookup */
int halexp_get_port_state(hexp_port_state_t * state, const char *port_name)
{
	return hal_port_get_exported_state(state, hal_ports, port_name);
}

int halexp_pps_cmd(int cmd, hexp_pps_params_t * params)
{
	int ret, rval;
	ret = minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_pps_cmd,
			  &rval, cmd, params);
	if (ret < 0)
		return ret;
	return rval;
}

int halexp_get_timing_state(hexp_timing_state_t * tstate)
{
	int ret;
	ret = minipc_call(hal_ch, DEFAULT_TO, &__rpcdef_get_timing_state,
			  tstate);
	if (ret < 0)
		return ret;
	return 0;
}

/* Some clients call this, some call the client_init() defined later */
int halexp_client_try_connect(int retries, int timeout)
{
	struct hal_shmem_header *h;
	struct hal_port_state *p;

	hal_head = wrs_shm_get(wrs_shm_hal,"", WRS_SHM_READ);
	if (!hal_head)
		return -1;
	h = (void *)hal_head + hal_head->data_off;
	hal_nports = h->nports;
	p = wrs_shm_follow(hal_head, h->ports);
	hal_ports = p; /* This is used in later calls */

	for (;;) {
		hal_ch =
		    minipc_client_create(WRSW_HAL_SERVER_ADDR,
					 MINIPC_FLAG_VERBOSE);
		if (hal_ch == 0)
			retries--;
		else
			return 0;

		if (!retries)
			return -1;

		usleep(timeout);
	}

	return -1;
}

int halexp_client_init()
{
	return halexp_client_try_connect(0, 0);
}
