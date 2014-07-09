/*
 * A global (library-wide) init function to register several things
 */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

/* The sub-init functions */
#include "wrsSnmp.h"

FILE *wrs_logf; /* for the local-hack messages */

void init_wrsSnmp(void)
{
	init_wrsScalar();
	init_wrsPstats();
	init_wrsPpsi();
	init_wrsVersion();
}

/* open a file or a pipe, so I test with files, and run with pipes */
FILE *wrs_fpopen(char *file_or_pipe, char *mode)
{
	if (file_or_pipe[0] == '|')
		return popen(file_or_pipe + 1, mode);
	else
		return fopen(file_or_pipe, mode);
}

void wrs_fpclose(FILE *f, char *file_or_pipe)
{
	if (file_or_pipe[0] == '|')
		pclose(f);
	else
		fclose(f);
}

