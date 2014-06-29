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

	table_info->min_column = TCPCONNSTATE;  /* 1 */
	table_info->max_column = TCPCONNREMOTEPORT; /* 5 */

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
    long     port;
    long     state;

    switch (reqinfo->mode) {
    case MODE_GET:
        for (request=requests; request; request=request->next) {
            requestvb = request->requestvb;
            DEBUGMSGTL(( "mibII/tcpTable", "oid: "));
            DEBUGMSGOID(("mibII/tcpTable", requestvb->name,
                                           requestvb->name_length));
            DEBUGMSG((   "mibII/tcpTable", "\n"));

            entry = (TCPTABLE_ENTRY_TYPE *)netsnmp_extract_iterator_context(request);
            if (!entry)
                continue;
            table_info = netsnmp_extract_table_info(request);
            subid      = table_info->colnum;

            switch (subid) {
            case TCPCONNSTATE: // 1
                state = entry->TCPTABLE_STATE;
	        snmp_set_var_typed_value(requestvb, ASN_INTEGER,
                                 (u_char *)&state, sizeof(state));
                break;
            case TCPCONNLOCALADDRESS: // 2
#if defined(osf5) && defined(IN6_EXTRACT_V4ADDR)
	        snmp_set_var_typed_value(requestvb, ASN_IPADDRESS,
                              (u_char*)IN6_EXTRACT_V4ADDR(&entry->pcb.inp_laddr),
                                sizeof(IN6_EXTRACT_V4ADDR(&entry->pcb.inp_laddr)));
#else
	        snmp_set_var_typed_value(requestvb, ASN_IPADDRESS,
                                 (u_char *)&entry->TCPTABLE_LOCALADDRESS,
                                     sizeof(entry->TCPTABLE_LOCALADDRESS));
#endif
                break;
            case TCPCONNLOCALPORT: // 3
		    port = /* TCP_PORT_TO_HOST_ORDER( */ (u_short)entry->TCPTABLE_LOCALPORT;
	        snmp_set_var_typed_value(requestvb, ASN_INTEGER,
                                 (u_char *)&port, sizeof(port));
                break;
            case TCPCONNREMOTEADDRESS: // 4
#if defined(osf5) && defined(IN6_EXTRACT_V4ADDR)
	        snmp_set_var_typed_value(requestvb, ASN_IPADDRESS,
                              (u_char*)IN6_EXTRACT_V4ADDR(&entry->pcb.inp_laddr),
                                sizeof(IN6_EXTRACT_V4ADDR(&entry->pcb.inp_laddr)));
#else
	        snmp_set_var_typed_value(requestvb, ASN_IPADDRESS,
                                 (u_char *)&entry->TCPTABLE_REMOTEADDRESS,
                                     sizeof(entry->TCPTABLE_REMOTEADDRESS));
#endif
                break;
            case TCPCONNREMOTEPORT: // 5
		    port = /* TCP_PORT_TO_HOST_ORDER( */ (u_short)entry->TCPTABLE_REMOTEPORT;
	        snmp_set_var_typed_value(requestvb, ASN_INTEGER,
                                 (u_char *)&port, sizeof(port));
                break;
	    }
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
	if (tcp_head == NULL)
		return NULL;
	logmsg("%s: %i\n", __func__, __LINE__);

	/* reset internal position, so "next" is "first" */
	*loop_context = (void*)tcp_head;
	return pstatsTable_next_entry(loop_context, data_context, index, data);
}

netsnmp_variable_list *
pstatsTable_next_entry( void **loop_context,
			void **data_context,
			netsnmp_variable_list *index,
			netsnmp_iterator_info *data)
{
    TCPTABLE_ENTRY_TYPE	 *entry = (TCPTABLE_ENTRY_TYPE *)*loop_context;
    netsnmp_variable_list *idx;
    long addr, port;
    static int i;

    logmsg("%s: %i\n", __func__, __LINE__);
    if (!entry)
        return NULL;
    logmsg("%s: %i\n", __func__, __LINE__);

    if (*loop_context == (void*)tcp_head)
	    i = 0;
    i++;
    /*
     * Set up the indexing for the specified row...
     */
    idx = index;
    snmp_set_var_value(idx, (u_char*)&i, sizeof(i));

    /*
     * ... return the data structure for this row,
     * and update the loop context ready for the next one.
     */
    *data_context = (void*)entry;
    *loop_context = (void*)entry->INP_NEXT_SYMBOL;
    return index;
}

void
pstatsTable_free(netsnmp_cache *cache, void *magic)
{
    TCPTABLE_ENTRY_TYPE *p;
	logmsg("%s: %i\n", __func__, __LINE__);
    while (tcp_head) {
	logmsg("%s: %i\n", __func__, __LINE__);
        p = tcp_head;
        tcp_head = tcp_head->INP_NEXT_SYMBOL;
        free(p);
    }

    tcp_head  = NULL;
    tcp_size  = 0;
    tcp_estab = 0;
}

int
pstatsTable_load(netsnmp_cache *cache, void *vmagic)
{
    FILE           *in;
    char            line[256];

	logmsg("%s: %i\n", __func__, __LINE__);
    pstatsTable_free(cache, NULL);

    if (!(in = fopen("/proc/net/tcp", "r"))) {
        DEBUGMSGTL(("mibII/tcpTable", "Failed to load TCP Table (linux1)\n"));
        NETSNMP_LOGONCE((LOG_ERR, "snmpd: cannot open /proc/net/tcp ...\n"));
        return -1;
    }

	logmsg("%s: %i\n", __func__, __LINE__);
    /*
     * scan proc-file and build up a linked list 
     * This will actually be built up in reverse,
     *   but since the entries are unsorted, that doesn't matter.
     */
    while (line == fgets(line, sizeof(line), in)) {
	logmsg("%s: %i\n", __func__, __LINE__);
        struct inpcb    pcb, *nnew;
        static int      linux_states[12] =
            { 1, 5, 3, 4, 6, 7, 11, 1, 8, 9, 2, 10 };
        unsigned int    lp, fp;
        int             state, uid;

        if (6 != sscanf(line,
                        "%*d: %x:%x %x:%x %x %*X:%*X %*X:%*X %*X %d",
                        &pcb.inp_laddr.s_addr, &lp,
                        &pcb.inp_faddr.s_addr, &fp, &state, &uid))
            continue;

        pcb.inp_lport = lp; //htons((unsigned short) lp);
        pcb.inp_fport = fp; //htons((unsigned short) fp);

        pcb.inp_state = (state & 0xf) < 12 ? linux_states[state & 0xf] : 2;
        if (pcb.inp_state == 5 /* established */ ||
            pcb.inp_state == 8 /*  closeWait  */ )
            tcp_estab++;
        pcb.uid = uid;

        nnew = SNMP_MALLOC_TYPEDEF(struct inpcb);
        if (nnew == NULL)
            break;
        memcpy(nnew, &pcb, sizeof(struct inpcb));
        nnew->inp_next = tcp_head;
        tcp_head       = nnew;
    }

    fclose(in);

    DEBUGMSGTL(("mibII/tcpTable", "Loaded TCP Table\n"));
    return 0;
}
