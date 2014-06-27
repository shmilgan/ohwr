/*
 * A global (library-wide) init function to register several things
 */
#include <stdio.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

/* The sub-init functions */
#include "wrsScalar.h"
#include "pstatsTable.h"
void
init_wrsSnmp(void)
{
	FILE *f = fopen("/dev/console", "w");
	fprintf(f, "FUCK FUCK FUCK\n");
	fclose(f);

	init_wrsScalar();
	init_pstatsTable();
}
