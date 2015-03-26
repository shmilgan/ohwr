/*
 * A global (library-wide) init function to register several things
 */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

/* The sub-init functions */
#include "wrsSnmp.h"
#include "snmp_shmem.h"
#include "wrsPstatsTable.h"
#include "wrsPtpDataTable.h"
#include "wrsCurrentTimeGroup.h"
#include "wrsTemperature.h"
#include "wrsOSStatus.h"
#include "wrsVersionGroup.h"
#include "wrsPortStatusTable.h"

FILE *wrs_logf; /* for the local-hack messages */

void init_wrsSnmp(void)
{
	init_shm();
	init_wrsScalar();
	init_wrsPstatsTable();
	init_wrsPtpDataTable();
	init_wrsCurrentTimeGroup();
	init_wrsTemperature();
	init_wrsOSStatus();
	init_wrsVersionGroup();
	init_wrsPortStatusTable();
}
