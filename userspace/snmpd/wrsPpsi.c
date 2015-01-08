/*
 *  White Rabbit PTP information, from ppsi.  Both globals and a table.
 *
 *  Alessandro Rubini for CERN, 2014
 */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/auto_nlist.h>

/* Crap! -- everybody makes them different, and even ppsi::ieee wants them */
#undef FALSE
#undef TRUE

#include <ppsi/ieee1588_types.h> /* for ClockIdentity */
#include <minipc.h>
#include <libwr/shmem.h>
#include <libwr/hal_shmem.h>

#include <stdio.h>

#include "wrsSnmp.h"

#define PPSI_CACHE_TIMEOUT 5 /* 1 second: refresh table every so often */

/* Table-driven memcpy: declare how to pick fields (pickinfo) -- and scan */
struct ppsi_pickinfo {
	/* Following fields are used to format the output */
	int type; int offset; int len;
	/* The following field is used to scan the input */
	char *name;
};

static struct wrs_shm_head *hal_head;
static struct hal_port_state *hal_ports;
static int hal_nports_local;

#define FIELD(_struct, _type, _field, _name) {			\
		.name = _name,					\
		.type = _type,					\
		.offset = offsetof(struct _struct, _field),	\
		.len = sizeof(_struct._field),			\
		.name = _name, /* Warning: see wr_mon */	\
		 }


/* Our data: globals */
static struct wrs_p_globals {
	ClockIdentity gm_id; /* Scanned as %x:... because it's 8-long */
	ClockIdentity my_id; /* Same as above */
	int ppsi_mode;
	char ppsi_servo_state[32]; /* Scanned as "%s" */
	int phase_tracking;
	char sync_source[32]; /* Scanned as "%s" because length > 8 bytes */
	int64_t clock_offset;
	int32_t skew;
	int64_t rtt;
	uint32_t llength;
	int64_t servo_updates;
} wrs_p_globals;

static struct ppsi_pickinfo g_pickinfo[] = {
	/* Warning: strings are a special case for snmp format */
	FIELD(wrs_p_globals, ASN_OCTET_STR, gm_id, "gm_id:"),
	FIELD(wrs_p_globals, ASN_OCTET_STR, my_id, "clock_id:"),
	FIELD(wrs_p_globals, ASN_INTEGER, ppsi_mode, "mode:"),
	FIELD(wrs_p_globals, ASN_OCTET_STR, ppsi_servo_state, "servo_state:"),
	FIELD(wrs_p_globals, ASN_INTEGER, phase_tracking, "tracking:"),
	FIELD(wrs_p_globals, ASN_OCTET_STR, sync_source, "source:"),
	FIELD(wrs_p_globals, ASN_COUNTER64, clock_offset, "ck_offset:"),
	FIELD(wrs_p_globals, ASN_INTEGER, skew, "skew:"),
	FIELD(wrs_p_globals, ASN_COUNTER64, rtt, "rtt:"),
	FIELD(wrs_p_globals, ASN_UNSIGNED, llength, "llength:"),
	FIELD(wrs_p_globals, ASN_COUNTER64, servo_updates, "servo_upd:"),
};

/* Our data: per-port information */
static struct wrs_p_perport {
	ClockIdentity peer_id;
	/* These can't be "unsigned char" because we scanf a %i in there */
	unsigned link_up;
	unsigned port_mode;
	unsigned port_locked;
} wrs_p_perport, wrs_p_array[WRS_N_PORTS];

static struct ppsi_pickinfo p_pickinfo[] = {
	FIELD(wrs_p_perport, ASN_INTEGER, link_up, "linkup:"),
	FIELD(wrs_p_perport, ASN_INTEGER, port_mode, "mode:"),
	FIELD(wrs_p_perport, ASN_INTEGER, port_locked, "locked:"),
	FIELD(wrs_p_perport, ASN_OCTET_STR, peer_id, "peer_id:"),
};

/* Parse a single line, used by both global and per-port */
static void wrs_ppsi_parse_line(char *line, void *baseaddr,
				struct ppsi_pickinfo *pi, int npi)
{
	char key[20], value[60];
	void *addr;
	long long *ptr64;
	int i;

	i = sscanf(line, "%s %s", key, value); /* value string has no spaces */
	logmsg("%s", line);
	logmsg("--%s--%s--\n", key, value);
	if (i == 0)
		return;
	if (i != 2) {
		snmp_log(LOG_ERR, "%s: can't parse line: %s", __func__,
			 line /* this includes a trailing newline already */);
		return;
	}
	/* now use parseinfo to find the key */
	for (i = 0; i < npi; i++, pi++)
		if (!strcmp(key, pi->name))
			break;
	if (i == npi) {
		snmp_log(LOG_ERR, "%s: can't find key \"%s\"\n", __func__,
			 key);
		return;
	}

	addr = baseaddr + pi->offset;

	/* Here I'm lazy in error checking, let's hope it's ok */
	switch (pi->type) {
	case ASN_UNSIGNED:
		/*
		 * our unsigned is line length, definitely less than 2G,
		 * so fall through...
		 */
	case ASN_INTEGER:
		sscanf(value, "%i", (int *)addr);
		break;

	case ASN_COUNTER64:
		ptr64 = addr;
		sscanf(value, "%lli", ptr64);
		/*
		 * WARNING: the current snmpd is bugged: it has
		 * endianness problems with 64 bit, and the two
		 * halves are swapped. So pre-swap them here
		 */
		*ptr64 = (*ptr64 << 32) | (*ptr64 >> 32);
		break;

	case ASN_OCTET_STR:
		if (pi->len == 8) {
			char *a = addr;
			sscanf(value,
			       "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
			       a+0, a+1, a+2, a+3, a+4, a+5, a+6, a+7);
			break;
		}
		if (pi->len > 8) {
			strcpy(addr, value);
			break;
		}
		snmp_log(LOG_ERR, "%s: no rule to parse OCTET_STREAM \"%s\"\n",
			 __func__, key);
		break;

	default:
		snmp_log(LOG_ERR, "%s: no rule to parse type of key \"%s\"\n",
			 __func__, key);
	}
}


static void wrs_ppsi_get_globals(void)
{
	static char *fname;
	FILE *f;
	char s[80];

	/* Allow the environment to override the fname, during development */
	if (!fname) {
		fname = getenv("WRS_SNMP_MON_FNAME");
		if (!fname)
			fname = "|/wr/bin/wr_mon -g";
	}

	f = wrs_fpopen(fname, "r");
	if (!f) {
		snmp_log(LOG_ERR, "%s: can't open \"%s\"\n", __func__, fname);
		return;
	}
	memset(&wrs_p_globals, 0, sizeof(wrs_p_globals));
	while (fgets(s, sizeof(s), f)) {
		wrs_ppsi_parse_line(s, &wrs_p_globals, g_pickinfo,
				    ARRAY_SIZE(g_pickinfo));
	}
	wrs_fpclose(f, fname);
}

void init_shm(void)
{
	struct hal_shmem_header *h;

	hal_head = wrs_shm_get(wrs_shm_hal, "", WRS_SHM_READ);
	if (!hal_head) {
		fprintf(stderr, "unable to open shm for HAL!\n");
		exit(-1);
	}
	h = (void *)hal_head + hal_head->data_off;
	/* Assume number of ports does not change in runtime */
	hal_nports_local = h->nports;
	/* Even after HAL restart, HAL will place structures at the same
	 * addresses. No need to re-dereference pointer at each read. */
	hal_ports = wrs_shm_follow(hal_head, h->ports);
	if (hal_nports_local > WRS_N_PORTS) {
		snmp_log(LOG_ERR, "Too many ports reported by HAL. "
			"%d vs %d supported\n",
			hal_nports_local, HAL_MAX_PORTS);
		exit(-1);
	}
}

static void wrs_ppsi_get_per_port(void)
{
	unsigned ii, i;
	unsigned retries = 0;

	/* read data, with the sequential lock to have all data consistent */
	struct hal_port_state *port_state;
	memset(wrs_p_array, 0, sizeof(wrs_p_array));
	do {
		ii = wrs_shm_seqbegin(hal_head);
		for (i = 0; i < hal_nports_local; ++i) {
			/* Assume that number of ports does not change between
			 * reads */
			char if_name[10];

			snprintf(if_name, 10, "wr%d", i);
			port_state = hal_lookup_port(hal_ports,
						    hal_nports_local, if_name);
			/* No need to copy all ports structures, only what
			 * we're interested in */
			wrs_p_array[i].link_up = state_up(port_state->state);
			wrs_p_array[i].port_mode = (port_state->mode ==
					      HEXP_PORT_MODE_WR_SLAVE ? 0 : 1);
			wrs_p_array[i].port_locked = port_state->locked;
			/* FIXME: get real peer_id */
			memset(&wrs_p_array[i].peer_id, 0xff,
			       sizeof(ClockIdentity));
			logmsg("reading ports name %s link %d, mode %d, "
			 "locked %d\n", port_state->name,
			 wrs_p_array[i].link_up, wrs_p_array[i].port_mode,
			 wrs_p_array[i].port_locked);
		}

		retries++;
		if (retries > 100) {
			snmp_log(LOG_ERR, "%s: too many retries to read HAL\n",
				 __func__);
			retries = 0;
			}
	} while (wrs_shm_seqretry(hal_head, ii));

}


/* From here on it is netsnmp-specific hook an tools */


/* This is the filler for the global scalars */
static int ppsi_g_group(netsnmp_mib_handler          *handler,
			netsnmp_handler_registration *reginfo,
			netsnmp_agent_request_info   *reqinfo,
			netsnmp_request_info         *requests)
{
	static time_t t0, t1;
	int obj; /* the final index */
	struct ppsi_pickinfo *pi;
	void *ptr;
	int len;

	/*
	 * Retrieve information from ppsi itself. But this function
	 * is called once for every item, so only query the whole set
	 * once every 2 seconds.
	 */
	t1 = time(NULL);
	if (!t0 || t1 - t0 > 1) {
		wrs_ppsi_get_globals();
		t0 = t1;
	}

	switch (reqinfo->mode) {
	case MODE_GET:
		/* "- 2" because last is 0 for all scalars, I suppose */
		obj = requests->requestvb->name[
			requests->requestvb->name_length - 2
			];
		obj--; /* we are 0-based */
		if (obj < 0 || obj >= ARRAY_SIZE(g_pickinfo)) {
			snmp_log(LOG_ERR, "wrong index (%d) in wrs ppsi\n",
				 obj + 1);
			return SNMP_ERR_GENERR;
		}
		pi = g_pickinfo + obj;
		ptr = (void *)&wrs_p_globals + pi->offset;
		len = pi->len;
		if (len > 8) /* special case for strings */
			len = strlen(ptr);
		snmp_set_var_typed_value(requests->requestvb,
					 pi->type, ptr, len);
		break;
	default:
		snmp_log(LOG_ERR, "unknown mode (%d) in wrs ppsi group\n",
			 reqinfo->mode);
		return SNMP_ERR_GENERR;
	}
	return SNMP_ERR_NOERROR;
}

/* For the per-port table we use an iterator like in wrsPstats.c */

static netsnmp_variable_list *
ppsi_p_next_entry(void **loop_context,
			void **data_context,
			netsnmp_variable_list *index,
			netsnmp_iterator_info *data)
{
	intptr_t i;

	/* create the line ID from counter number */
	i = (intptr_t)*loop_context;
	//logmsg("%s: %i (i = %i)\n", __func__, __LINE__, i);
	if (i >= WRS_N_PORTS)
		return NULL; /* no more */
	i++;
	/* Create the row OID: only the counter index */
	snmp_set_var_value(index, (u_char *)&i, sizeof(i));

	/* Set the data context (1..4 -> 0..3) */
	*data_context = (void *)(intptr_t)(i - 1);
	/* and set the loop context for the next iteration */
	*loop_context = (void *)i;
	return index;
}

static netsnmp_variable_list *
ppsi_p_first_entry(void **loop_context,
			void **data_context,
			netsnmp_variable_list *index,
			netsnmp_iterator_info *data)
{
	/* reset internal position, so "next" is "first" */
	*loop_context = (void *)0; /* first counter */
	return ppsi_p_next_entry(loop_context, data_context, index, data);
}

/* This function must fill the per-port information for the whole table */
static int ppsi_p_load(netsnmp_cache *cache, void *vmagic)
{
	wrs_ppsi_get_per_port();
	return 0;
}

static int
ppsi_p_handler(netsnmp_mib_handler          *handler,
	      netsnmp_handler_registration *reginfo,
	      netsnmp_agent_request_info   *reqinfo,
	      netsnmp_request_info         *requests)
{
	netsnmp_request_info  *request;
	netsnmp_variable_list *requestvb;
	netsnmp_table_request_info *table_info;

	struct wrs_p_perport *data = wrs_p_array; /* a shorter name */
	struct ppsi_pickinfo *pi;
	int wrport, subid;

	//logmsg("%s: %i\n", __func__, __LINE__);
	switch (reqinfo->mode) {
	case MODE_GET:
		/* "break;" so read code is not indented too much */
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
		return SNMP_ERR_NOERROR;
	default:
		/* unknown mode */
		return SNMP_ERR_NOERROR;
	}


	for (request = requests; request; request = request->next) {
		requestvb = request->requestvb;

		//logmsg("%s: %i\n", __func__, __LINE__);

		/* "context" is the port number */
		wrport = (intptr_t)netsnmp_extract_iterator_context(request);
		table_info = netsnmp_extract_table_info(request);
		subid = table_info->colnum - 1;

		pi = p_pickinfo + subid;
		snmp_set_var_typed_value(requestvb, pi->type,
					 (void *)(data + wrport)
					 + pi->offset, pi->len);
	}
	return SNMP_ERR_NOERROR;
}

void
init_wrsPpsi(void)
{
	const oid wrsPpsiG_oid[] = {  WRS_OID, 3, 1 };
	netsnmp_handler_registration *hreg;
	/* Above for globals, below for per-port */
	const oid wrsPpsiP_oid[] = {  WRS_OID, 3, 2 };
	netsnmp_table_registration_info *table_info;
	netsnmp_iterator_info *iinfo;
	netsnmp_handler_registration *reginfo;

	/* open shm */
	init_shm();

	/* do the registration for the scalars/globals */
	hreg = netsnmp_create_handler_registration(
		"wrsPpsiGlobals", ppsi_g_group,
		wrsPpsiG_oid, OID_LENGTH(wrsPpsiG_oid),
		HANDLER_CAN_RONLY);
	netsnmp_register_scalar_group(
		hreg, 1 /* min */, ARRAY_SIZE(g_pickinfo) /* max */);

	/* do the registration for the table/per-port */
	table_info = SNMP_MALLOC_TYPEDEF(netsnmp_table_registration_info);
	if (!table_info)
		return;

	/* Add indexes: we only use one integer OID member as line identifier */
	netsnmp_table_helper_add_indexes(table_info, ASN_INTEGER, 0);

	table_info->min_column = 1;
	table_info->max_column = ARRAY_SIZE(p_pickinfo);

	/* Iterator info */
	iinfo  = SNMP_MALLOC_TYPEDEF(netsnmp_iterator_info);
	if (!iinfo)
		return; /* free table_info? */

	iinfo->get_first_data_point = ppsi_p_first_entry;
	iinfo->get_next_data_point  = ppsi_p_next_entry;
	iinfo->table_reginfo        = table_info;

	/* register the table */
	reginfo = netsnmp_create_handler_registration("wrsPpsiPerport",
						      ppsi_p_handler,
						      wrsPpsiP_oid,
						      OID_LENGTH(wrsPpsiP_oid),
						      HANDLER_CAN_RONLY);
	netsnmp_register_table_iterator(reginfo, iinfo);

	/* and create a local cache */
	netsnmp_inject_handler(reginfo,
				netsnmp_get_cache_handler(PPSI_CACHE_TIMEOUT,
							  ppsi_p_load, NULL,
							  wrsPpsiP_oid,
							  OID_LENGTH(wrsPpsiP_oid)));
}
