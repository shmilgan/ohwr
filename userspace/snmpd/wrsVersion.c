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

#include "wrsSnmp.h"

/* Our structure for caching data */
#define VERSION_N_STRINGS 6  /* sw, 3 gw, 2 hw */

struct wrs_v_item {
	char *key;
	char *value;
};

static struct wrs_v_item wrs_version[] = {
	/* Warning: the order here msut match the MIB file */
	[0] = {.key = "software-version:"},
	[1] = {"wr_switch_hdl-commit:"},
	[2] = {"general-cores-commit:"},
	[3] = {"wr-cores-commit:"},
	[4] = {"pcb-version:"},
	[5] = {"fpga-type:"},
};


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

		s = wrs_version[obj - 1].value ? : "unknown";
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

/*
 * We parse versions at initialization time, as they won't change.
 * FIXME: we should factorize parsing, this duplicates ./wrsPpsi.c
 * (while being different, as here we need simpler stuff)
 */
static void wrs_v_init(void)
{
	char s[80], key[40], value[40];
	FILE *f = popen("/wr/bin/wrsw_version -t", "r");
	int i;

	if (!f) {
		/* The "unknown" above will apply, bad but acceptable */
		return;
	}

	while (fgets(s, sizeof(s), f)) {
		if (sscanf(s, "%s %[^\n]", key, value) != 2)
			continue; /* error... */
		for (i = 0; i < ARRAY_SIZE(wrs_version); i++) {
			if (strcmp(key, wrs_version[i].key))
				continue;
			wrs_version[i].value = strdup(value);
		}
	}
	pclose(f);
}

void
init_wrsVersion(void)
{

	const oid wrsVersion_oid[] = {  WRS_OID, 4 };
	netsnmp_handler_registration *hreg;

	wrs_v_init();

	/* do the registration */
	hreg = netsnmp_create_handler_registration(
		"wrsVersion", version_group,
		wrsVersion_oid, OID_LENGTH(wrsVersion_oid),
		HANDLER_CAN_RONLY);
	netsnmp_register_scalar_group(
		hreg, 1 /* min */, VERSION_N_STRINGS /* max */);
}
