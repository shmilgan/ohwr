/*
 *  White Rabbit Switch pstats table
 *  Using the netsnmp iterator, like in tcpTable
 *  Alessandro Rubini for CERN, 2014
 */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/auto_nlist.h>

#include <netinet/tcp.h>

#include "pstatsTable.h"

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

/* Our structure for caching data */
#define PSTATS_N_COUNTERS  39
#define PSTATS_N_PORTS 18 /* this hardwired in the mib too */

struct pstats_per_port {
	uint32_t  val[PSTATS_N_COUNTERS];
};

struct pstats_global_data {
	struct pstats_per_port port[PSTATS_N_PORTS];
	char *pname[PSTATS_N_PORTS];
} pstats_global_data;


#define	TCPTABLE_ENTRY_TYPE	struct inpcb
#define	TCPTABLE_STATE		inp_state
#define	TCPTABLE_LOCALADDRESS	inp_laddr.s_addr
#define	TCPTABLE_LOCALPORT	inp_lport
#define	TCPTABLE_REMOTEADDRESS	inp_faddr.s_addr
#define	TCPTABLE_REMOTEPORT	inp_fport

				/* Head of linked list, or root of table */
TCPTABLE_ENTRY_TYPE	*tcp_head  = NULL;
int                      tcp_size  = 0;	/* Only used for table-based systems */
int                      tcp_estab = 0;


#define PSTATS_CACHE_TIMEOUT 5 /* seconds */

void
init_pstatsTable(void)
{
	const oid pstatsTable_oid[] = {  1, 3, 6, 1, 4, 1, 96, 100, 2, };

	netsnmp_table_registration_info *table_info;
	netsnmp_iterator_info *iinfo;
	netsnmp_handler_registration *reginfo;

	table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
	if (!table_info)
		return;

	/* Add indexes: we only use one integer OID member as line identifier */
	netsnmp_table_helper_add_indexes(table_info, ASN_INTEGER, 0);

	table_info->min_column = 1;
	table_info->max_column = PSTATS_N_PORTS;

	/* Iterator info */
	iinfo  = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
	if (!iinfo)
		return; /* free table_info? */

	iinfo->get_first_data_point = pstatsTable_first_entry;
	iinfo->get_next_data_point  = pstatsTable_next_entry;
	iinfo->table_reginfo        = table_info;

	/* register the table */
	reginfo = netsnmp_create_handler_registration("pstatsTable",
						      pstatsTable_handler,
						      pstatsTable_oid, OID_LENGTH(pstatsTable_oid),
						      HANDLER_CAN_RONLY);
	netsnmp_register_table_iterator(reginfo, iinfo);

	/* and create a local cache */
	netsnmp_inject_handler(reginfo,
				netsnmp_get_cache_handler(PSTATS_CACHE_TIMEOUT,
							  pstatsTable_load, pstatsTable_free,
							  pstatsTable_oid, OID_LENGTH(pstatsTable_oid)));
}

int
pstatsTable_handler(netsnmp_mib_handler          *handler,
                 netsnmp_handler_registration *reginfo,
                 netsnmp_agent_request_info   *reqinfo,
                 netsnmp_request_info         *requests)
{
    netsnmp_request_info  *request;
    netsnmp_variable_list *requestvb;
    netsnmp_table_request_info *table_info;
    TCPTABLE_ENTRY_TYPE	  *entry;
    oid      subid;
    long     state;

    struct pstats_global_data *data = &pstats_global_data; /* a shorter name */
    int counter;
    int wrport;
    uint32_t *c;

    switch (reqinfo->mode) {
    case MODE_GET:
        for (request=requests; request; request=request->next) {
            requestvb = request->requestvb;

	    /* our "context" is the counter number; "subid" the column i.e. the port */
            counter = (int)netsnmp_extract_iterator_context(request);
   
            table_info = netsnmp_extract_table_info(request);
            wrport = table_info->colnum - 1; /* port is 0-based */
	    logmsg("counter %i, port %i\n", counter, wrport);
	    /* FIXME: the name as first column */

	    /* While most tables do "switch(subid)" we'd better just index */
	    data->port[wrport].val[counter]++;
	    c = &data->port[wrport].val[counter];
	    snmp_set_var_typed_value(requestvb, ASN_INTEGER, /* FIXME: counter32 */
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

netsnmp_variable_list *
pstatsTable_first_entry(void **loop_context,
			void **data_context,
			netsnmp_variable_list *index,
			netsnmp_iterator_info *data)
{
	logmsg("%s: %i\n", __func__, __LINE__);

	/* reset internal position, so "next" is "first" */
	*loop_context = (void*)0; /* first counter */
	return pstatsTable_next_entry(loop_context, data_context, index, data);
}

netsnmp_variable_list *
pstatsTable_next_entry( void **loop_context,
			void **data_context,
			netsnmp_variable_list *index,
			netsnmp_iterator_info *data)
{
	long addr, port;
	short i;

	//logmsg("%s: %i\n", __func__, __LINE__);

	/* create the line ID from counter number */
	i = (short)*loop_context;
	logmsg("%s: %i (i = %i)\n", __func__, __LINE__, i);
	if (i >= PSTATS_N_COUNTERS)
		return NULL; /* no more */
	i++;
	/* Create the row OID: only the counter index */
	snmp_set_var_value(index, (u_char*)&i, sizeof(i));

    /* Set the data context (1..39 -> 0..38) */
    *data_context = (void *)(i - 1);
    /* and set the loop context for the next iteration */
    *loop_context = (void *)i;
    return index;
}

void
pstatsTable_free(netsnmp_cache *cache, void *magic)
{
	/* nothing to free  */
}

int
pstatsTable_load(netsnmp_cache *cache, void *vmagic)
{
    FILE           *in;
    char            line[256];

    /* nothing to load -- FIXME: values */
    return 0;
}
