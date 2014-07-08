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

#include <stdarg.h>
static FILE *logf;

/* local hack */
static int logmsg(const char *fmt, ...)
{
	va_list args;
	int ret;

	if (WRS_WITH_SNMP_HACKISH_LOG) {
		if (!logf)
			logf = fopen("/dev/console", "w");

		va_start(args, fmt);
		ret = vfprintf(logf, fmt, args);
		va_end(args);

		return ret;
	} else {
		return 0;
	}
}

int dumpstruct(FILE *dest, char *name, void *ptr, int size)
{
	int ret = 0, i;
	unsigned char *p = ptr;

	if (WRS_WITH_SNMP_HACKISH_LOG) {
		ret = fprintf(dest, "dump %s at %p (size 0x%x)\n",
			      name, ptr, size);
		for (i = 0; i < size; ) {
			ret += fprintf(dest, "%02x", p[i]);
			i++;
			ret += fprintf(dest, i & 3 ? " " : i & 0xf ? "	" : "\n");
		}
		if (i & 0xf)
			ret += fprintf(dest, "\n");
	}
	return ret;
}
/* end local hack */

#define PSTATS_CACHE_TIMEOUT 5 /* seconds */

/* Our structure for caching data */
#define PSTATS_N_COUNTERS  39
#define PSTATS_N_PORTS 18 /* this hardwired in the mib too */

struct pstats_per_port {
	uint32_t  val[PSTATS_N_COUNTERS];
};

static struct pstats_global_data {
	struct pstats_per_port port[PSTATS_N_PORTS];
	char *pname[PSTATS_N_PORTS];
} pstats_global_data;



static char *pstats_names[] = {
	[0] = "TX Underrun",
	[1] = "RX Overrun",
	[2] = "RX Invalid Code",
	[3] = "RX Sync Lost",
	[4] = "RX Pause Frames",
	[5] = "RX Pfilter Dropped",
	[6] = "RX PCS Errors",
	[7] = "RX Giant Frames",
	[8] = "RX Runt Frames",
	[9] = "RX CRC Errors",
	[10] = "RX Pclass 0",
	[11] = "RX Pclass 1",
	[12] = "RX Pclass 2",
	[13] = "RX Pclass 3",
	[14] = "RX Pclass 4",
	[15] = "RX Pclass 5",
	[16] = "RX Pclass 6",
	[17] = "RX Pclass 7",
	[18] = "TX Frames",
	[19] = "RX Frames",
	[20] = "RX Drop RTU Full",
	[21] = "RX PRIO 0",
	[22] = "RX PRIO 1",
	[23] = "RX PRIO 2",
	[24] = "RX PRIO 3",
	[25] = "RX PRIO 4",
	[26] = "RX PRIO 5",
	[27] = "RX PRIO 6",
	[28] = "RX PRIO 7",
	[29] = "RTU Valid",
	[30] = "RTU Responses",
	[31] = "RTU Dropped",
	[32] = "FastMatch: Priority",
	[33] = "FastMatch: FastForward",
	[34] = "FastMatch: NonForward",
	[35] = "FastMatch: Resp Valid",
	[36] = "FullMatch: Resp Valid",
	[37] = "Forwarded",
	[38] = "TRU Resp Valid"
};

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
		for (request=requests; request; request=request->next) {
			requestvb = request->requestvb;

			logmsg("%s: %i\n", __func__, __LINE__);
			/* our "context" is the counter number; "subid" the column i.e. the port */
			counter = (intptr_t)netsnmp_extract_iterator_context(request);

			table_info = netsnmp_extract_table_info(request);
			wrport = table_info->colnum - 2; /* port is 0-based and position 1 is the string */
			logmsg("counter %i, port %i\n", counter, wrport);

			if (wrport < 0) {
				char *s = pstats_names[counter];
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
wrsPstats_next_entry( void **loop_context,
			void **data_context,
			netsnmp_variable_list *index,
			netsnmp_iterator_info *data)
{
	intptr_t i;

	/* create the line ID from counter number */
	i = (intptr_t)*loop_context;
	//logmsg("%s: %i (i = %i)\n", __func__, __LINE__, i);
	if (i >= PSTATS_N_COUNTERS)
		return NULL; /* no more */
	i++;
	/* Create the row OID: only the counter index */
	snmp_set_var_value(index, (u_char*)&i, sizeof(i));

	/* Set the data context (1..39 -> 0..38) */
	*data_context = (void *)(intptr_t)(i - 1);
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
	*loop_context = (void*)0; /* first counter */
	return wrsPstats_next_entry(loop_context, data_context, index, data);
}

static int
wrsPstats_load(netsnmp_cache *cache, void *vmagic)
{
	FILE *f;
	char fname[32];
	int wrport, counter;
	struct pstats_per_port  *stat;

	for (wrport = 0; wrport < PSTATS_N_PORTS; wrport++) {
		sprintf(fname, "/proc/sys/pstats/port%i", wrport);
		stat = pstats_global_data.port + wrport;
		f = fopen(fname, "r");
		if (!f) {
			memset(stat, 0x7f, sizeof(*stat));
			continue;
		}
		for (counter = 0; counter < PSTATS_N_COUNTERS; counter++) {
			if (fscanf(f, "%u", stat->val + counter) != 1)
				stat->val[counter] = 0xffffff;
		}
		fclose(f);
	}
	//dumpstruct(logf, "global data", &pstats_global_data, sizeof(pstats_global_data));
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
