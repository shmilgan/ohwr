/*
 * A global (library-wide) init function to register several things
 */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

/* The sub-init functions */
#include "wrsSnmp.h"

void
init_wrsSnmp(void)
{
	init_wrsScalar();
	init_wrsPstats();
	init_wrsVersion();
}
