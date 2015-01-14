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
