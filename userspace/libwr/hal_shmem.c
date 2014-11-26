#include <string.h>
#include <math.h>

#include <libwr/hal_shmem.h>
#include <libwr/switch_hw.h>
/*
 * The following functions were in wrsw_hal/hal_ports (the callee)
 * but with moving to shared memory they are run in the caller.
 */

extern struct hal_port_state  *ports; /* FIXME: temporarily */
extern int hal_port_nports;

struct hal_port_state *hal_port_lookup(struct hal_port_state *ports,
				       const char *name)
{
	int i;
	for (i = 0; i < HAL_MAX_PORTS; i++)
		if (ports[i].in_use && !strcmp(name, ports[i].name))
			return &ports[i];

	return NULL;
}


int hal_port_get_exported_state(struct hexp_port_state *state,
				struct hal_port_state *ports,
				const char *port_name)
{
	const struct hal_port_state *p = hal_port_lookup(ports, port_name);

//      TRACE(TRACE_INFO, "GetPortState %s [lup %x]\n", port_name, p);

	if (!p)
		return -1;

	/* WARNING! when alpha = 1.0 (no asymmetry), fiber_fix_alpha = 0! */

	state->fiber_fix_alpha = (double)pow(2.0, 40.0) *
	    ((p->calib.sfp.alpha + 1.0) / (p->calib.sfp.alpha + 2.0) - 0.5);

	state->valid = 1;
	state->mode = p->mode;
	state->up = (p->state != HAL_PORT_STATE_LINK_DOWN
		     && p->state != HAL_PORT_STATE_DISABLED);

	state->is_locked = p->locked;	//lock_state == LOCK_STATE_LOCKED;
	state->phase_val = p->phase_val;
	state->phase_val_valid = p->phase_val_valid;

	state->tx_calibrated = p->calib.tx_calibrated;
	state->rx_calibrated = p->calib.rx_calibrated;

	state->delta_tx = p->calib.delta_tx_phy
	    + p->calib.sfp.delta_tx + p->calib.delta_tx_board;
	state->delta_rx = p->calib.delta_rx_phy
	    + p->calib.sfp.delta_rx + p->calib.delta_rx_board;

	state->t2_phase_transition = DEFAULT_T2_PHASE_TRANS;
	state->t4_phase_transition = DEFAULT_T4_PHASE_TRANS;
	state->clock_period = REF_CLOCK_PERIOD_PS;

	memcpy(state->hw_addr, p->hw_addr, 6);
	state->hw_index = p->hw_index;

	return 0;
}

/* Public API function - returns the array of names of all WR network
 * interfaces */
int hal_port_query_ports(struct hexp_port_list *list,
			 const struct hal_port_state *ports)
{
	int i;
	int n = 0;

	for (i = 0; i < HAL_MAX_PORTS; i++)
		if (ports[i].in_use)
			strcpy(list->port_names[n++], ports[i].name);

	list->num_physical_ports = 18; /* was hal_port_nports */
	list->num_ports = n;
	return 0;
}


