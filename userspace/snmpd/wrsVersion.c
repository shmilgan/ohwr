/*
 *  White Rabbit Switch versions.  This is a series of scalars,
 *  so I used the approach of disman/expr/expScalars.c
 *
 *  Alessandro Rubini for CERN, 2014
 */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/auto_nlist.h>

#include "wrsVersion.h"

/* Our structure for caching data */
#define VERSION_N_STRINGS 6  /* sw, 3 gw, 2 hw */
static struct wrs_version {
	char *value[VERSION_N_STRINGS];
} wrs_version;

static int version_group(netsnmp_mib_handler          *handler,
			 netsnmp_handler_registration *reginfo,
			 netsnmp_agent_request_info   *reqinfo,
			 netsnmp_request_info         *requests)
{
	oid obj; /* actually, an integer, i.e. the final index */
	char *s;

	switch (reqinfo->mode) {
	case MODE_GET:
		/* "- 2" because last is 0 for all scalars, I suppose */
		obj = requests->requestvb->name[
			requests->requestvb->name_length - 2
			];

		s = wrs_version.value[obj - 1];
		snmp_set_var_typed_value(requests->requestvb,
					 ASN_OCTET_STR, s, strlen(s));
		break;
	default:
		snmp_log(LOG_ERR, "unknown mode (%d) in wrs version group\n",
			 reqinfo->mode);
		return SNMP_ERR_GENERR;
	}
	return SNMP_ERR_NOERROR;
}

void
init_wrsVersion(void)
{

	const oid wrsVersion_oid[] = {  WRS_VERSION_OID };

	netsnmp_handler_registration *hreg;

	/* FIXME.... */
	wrs_version.value[0] = "fake-v4.0-rc1";
	wrs_version.value[1] = "fake-7cce708";
	wrs_version.value[2] = "fake-5118070";
	wrs_version.value[3] = "fake-7efeb16";
	wrs_version.value[4] = "fake-3.30";
	wrs_version.value[5] = "fake-LX240T";

	/* do the registration */
	hreg = netsnmp_create_handler_registration(
		"wrsVersion", version_group,
		wrsVersion_oid, OID_LENGTH(wrsVersion_oid),
		HANDLER_CAN_RONLY);
	netsnmp_register_scalar_group(
		hreg, 1 /* min */, VERSION_N_STRINGS /* max */);
}
