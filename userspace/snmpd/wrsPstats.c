/*
 *  White Rabbit Switch pstats table
 *  Using the netsnmp iterator, like in tcpTable
 *  Alessandro Rubini for CERN, 2014
 */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/auto_nlist.h>

#include "wrsSnmp.h"

#define PSTATS_CACHE_TIMEOUT 5 /* seconds */

/* Our structure for caching data */
#define PSTATS_MAX_N_COUNTERS  50 /* Maximum number of counters */
#define PSTATS_N_PORTS 18 /* this hardwired in the mib too */
#define PSTATS_MAX_COUNTERS_NAME_LEN 35 /* maximum length of counter's name */
#define PSTATS_SYSCTL_PATH "/proc/sys/pstats/" /* Path to sysclt entries */
#define PSTATS_SYSCTL_DESCRIPTION_FILE "description" /* file with counters' descriptions */


struct pstats_per_port {
	uint32_t  val[PSTATS_MAX_N_COUNTERS];
};

static struct pstats_global_data {
	struct pstats_per_port port[PSTATS_N_PORTS];
	char counter_name[PSTATS_MAX_N_COUNTERS][PSTATS_MAX_COUNTERS_NAME_LEN];
	int n_counters;
} pstats_global_data;


/* FIXME: build error if ARRAY_SIZE(pstats_names) != PSTATS_N_COUNTERS */

static int
wrsPstats_handler(netsnmp_mib_handler          *handler,
		    netsnmp_handler_registration *reginfo,
		    netsnmp_agent_request_info   *reqinfo,
		    netsnmp_request_info         *requests)
{
	netsnmp_request_info  *request;
	netsnmp_variable_list *requestvb;
	netsnmp_table_request_info *table_info;

	struct pstats_global_data *data = &pstats_global_data; /* a shorter name */
	int counter;
	int wrport;
	uint32_t *c;

	logmsg("%s: %i\n", __func__, __LINE__);
	switch (reqinfo->mode) {
	case MODE_GET:
		for (request = requests; request; request = request->next) {
			requestvb = request->requestvb;

			logmsg("%s: %i\n", __func__, __LINE__);
			/* our "context" is the counter number; "subid" the column i.e. the port */
			counter = (intptr_t)netsnmp_extract_iterator_context(request);
			if (!counter)
				/* NULL returned from
				 * netsnmp_extract_iterator_context shuld be
				 * interpreted as end of table */
				continue;
			/* change range of counter (1..39 (snmp is 1 based) ->
			 * 0..38 (pstats_global_data array is 0 based)) */
			counter--;

			table_info = netsnmp_extract_table_info(request);
			/* port is 0-based and position 1 is the string */
			wrport = table_info->colnum - 2;
			logmsg("counter %i, port %i\n", counter, wrport);

			if (wrport < 0) {
				char *s = pstats_global_data.counter_name[counter];
				snmp_set_var_typed_value(requestvb, ASN_OCTET_STR, s, strlen(s));
				continue;
			}
			/* While most tables do "switch(subid)" we'd better just index */
			c = &data->port[wrport].val[counter];
			snmp_set_var_typed_value(requestvb, ASN_COUNTER,
						 (u_char *)c, sizeof(*c));
		}
		break;

	case MODE_GETNEXT:
	case MODE_GETBULK:
	case MODE_SET_RESERVE1:
	case MODE_SET_RESERVE2:
	case MODE_SET_ACTION:
	case MODE_SET_COMMIT:
	case MODE_SET_FREE:
	case MODE_SET_UNDO:
		/* unsupported mode */
		break;
	default:
		/* unknown mode */
		break;
	}

	return SNMP_ERR_NOERROR;
}


static netsnmp_variable_list *
wrsPstats_next_entry(void **loop_context,
			void **data_context,
			netsnmp_variable_list *index,
			netsnmp_iterator_info *data)
{
	intptr_t i;

	/* create the line ID from counter number */
	i = (intptr_t)*loop_context;
	//logmsg("%s: %i (i = %i)\n", __func__, __LINE__, i);
	if (i >= pstats_global_data.n_counters)
		return NULL; /* no more */
	i++;
	/* Create the row OID: only the counter index */
	snmp_set_var_value(index, (u_char *)&i, sizeof(i));

	/* Set the data context (1..39)
	 * Cannot be set to 0, because netsnmp_extract_iterator_context returns
	 * NULL in function wrsPstats_handler when table is over */
	*data_context = (void *)i;
	/* and set the loop context for the next iteration */
	*loop_context = (void *)i;
	return index;
}

static netsnmp_variable_list *
wrsPstats_first_entry(void **loop_context,
			void **data_context,
			netsnmp_variable_list *index,
			netsnmp_iterator_info *data)
{
	logmsg("%s: %i\n", __func__, __LINE__);

	/* reset internal position, so "next" is "first" */
	*loop_context = (void *)0; /* first counter */
	return wrsPstats_next_entry(loop_context, data_context, index, data);
}

static int
wrsPstats_load(netsnmp_cache *cache, void *vmagic)
{
	FILE *f;
	char fname[32];
	int wrport, counter;
	struct pstats_per_port  *stat;
	char *p;

	/* fill names of counters */
	f = fopen(PSTATS_SYSCTL_PATH PSTATS_SYSCTL_DESCRIPTION_FILE, "r");
	if (f) {
		for (counter = 0; counter < PSTATS_MAX_N_COUNTERS; counter++) {
			/* parse new line delimited file */
			p = fgets(pstats_global_data.counter_name[counter],
				  PSTATS_MAX_COUNTERS_NAME_LEN, f);
			if (p == NULL)
				break;
			/* fgets usualy returns strings with newline, return
			   string shall contain maximum one newline character */
			p = strchr(pstats_global_data.counter_name[counter],
				   '\n');
			if (p != NULL)
				*p = '\0';

		}
		pstats_global_data.n_counters = counter;
		fclose(f);
	} else {
		/* use PSTATS_MAX_N_COUNTERS as number of counters */
		pstats_global_data.n_counters = PSTATS_MAX_N_COUNTERS;
		/* fill counters' names */
		for (counter = 0; counter < PSTATS_MAX_N_COUNTERS; counter++) {
			snprintf(pstats_global_data.counter_name[counter],
				 PSTATS_MAX_COUNTERS_NAME_LEN,
				 "pstats counter %d", counter);
		}
	}

	for (wrport = 0; wrport < PSTATS_N_PORTS; wrport++) {
		sprintf(fname, PSTATS_SYSCTL_PATH"port%i", wrport);
		stat = pstats_global_data.port + wrport;
		f = fopen(fname, "r");
		if (!f) {
			memset(stat, 0x7f, sizeof(*stat));
			continue;
		}
		for (counter = 0;
		     counter < pstats_global_data.n_counters;
		     counter++) {
			if (fscanf(f, "%u", stat->val + counter) != 1)
				stat->val[counter] = 0xffffffff;
		}
		fclose(f);
	}
	return 0;
}

void
init_wrsPstats(void)
{
	const oid wrsPstats_oid[] = { WRS_OID, 2 };

	netsnmp_table_registration_info *table_info;
	netsnmp_iterator_info *iinfo;
	netsnmp_handler_registration *reginfo;

	table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
	if (!table_info)
		return;

	/* Add indexes: we only use one integer OID member as line identifier */
	netsnmp_table_helper_add_indexes(table_info, ASN_INTEGER, 0);

	table_info->min_column = 1;
	table_info->max_column = 1 + PSTATS_N_PORTS;

	/* Iterator info */
	iinfo  = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
	if (!iinfo)
		return; /* free table_info? */

	iinfo->get_first_data_point = wrsPstats_first_entry;
	iinfo->get_next_data_point  = wrsPstats_next_entry;
	iinfo->table_reginfo        = table_info;

	/* register the table */
	reginfo = netsnmp_create_handler_registration("wrsPstats",
						      wrsPstats_handler,
						      wrsPstats_oid, OID_LENGTH(wrsPstats_oid),
						      HANDLER_CAN_RONLY);
	netsnmp_register_table_iterator(reginfo, iinfo);

	/* and create a local cache */
	netsnmp_inject_handler(reginfo,
				netsnmp_get_cache_handler(PSTATS_CACHE_TIMEOUT,
							  wrsPstats_load, NULL,
							  wrsPstats_oid, OID_LENGTH(wrsPstats_oid)));
}
