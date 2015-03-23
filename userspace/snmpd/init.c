/*
 * A global (library-wide) init function to register several things
 */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

/* The sub-init functions */
#include "wrsSnmp.h"
#include "wrsPtpDataTable.h"
#include "wrsTemperature.h"
#include "wrsOSStatus.h"
#include "wrsPortStatusTable.h"

FILE *wrs_logf; /* for the local-hack messages */

void init_wrsSnmp(void)
{
	init_wrsScalar();
	init_wrsPstats();
	init_wrsPpsi();
	init_wrsVersion();
	init_wrsDate();
	init_wrsPtpDataTable();
	init_wrsTemperature();
	init_wrsOSStatus();
	init_wrsPortStatusTable();
}
