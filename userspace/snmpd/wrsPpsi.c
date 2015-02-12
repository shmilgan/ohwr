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

/* conflict between definition in net-snmp-agent-includes.h (which include
 * snmp_vars.h) and ppsi.h where INST is defined as a inline function */
#undef INST
#include <ppsi/ieee1588_types.h> /* for ClockIdentity */
#include <libwr/shmem.h>
#include <ppsi/ppsi.h>
#include <libwr/hal_shmem.h>
#include <stdio.h>

#include "wrsSnmp.h"

#define PPSI_CACHE_TIMEOUT 5 /* 1 second: refresh table every so often */

/* Table-driven memcpy: declare how to pick fields (pickinfo) */
struct ppsi_pickinfo {
	/* Following fields are used to format the output */
	int type; int offset; int len;
};

static struct wrs_shm_head *hal_head;
static struct hal_port_state *hal_ports;
static int hal_nports_local;

static struct wrs_shm_head *ppsi_head;
static struct pp_globals *ppg;
struct wr_servo_state_t *ppsi_servo;

#define FIELD(_struct, _type, _field) {			\
		.type = _type,					\
		.offset = offsetof(struct _struct, _field),	\
		.len = sizeof(_struct._field),			\
		 }


/* Our data: globals */
static struct wrs_p_globals {
	ClockIdentity gm_id;	/* FIXME: not implemented */
	ClockIdentity my_id;	/* FIXME: not implemented */
	int ppsi_mode;		/* FIXME: not implemented */
	char servo_state_name[32]; /* State as string */
	int servo_state;	/* state number */
	int tracking_enabled;
	char sync_source[32];	/* FIXME: not implemented */
	int64_t clock_offset;
	int32_t skew;
	int64_t rtt;
	uint32_t llength;
	uint32_t servo_updates;
} wrs_p_globals;

static struct ppsi_pickinfo g_pickinfo[] = {
	/* Warning: strings are a special case for snmp format */
	FIELD(wrs_p_globals, ASN_OCTET_STR, gm_id),
	FIELD(wrs_p_globals, ASN_OCTET_STR, my_id),
	FIELD(wrs_p_globals, ASN_INTEGER, ppsi_mode),
	FIELD(wrs_p_globals, ASN_OCTET_STR, servo_state_name),
	FIELD(wrs_p_globals, ASN_INTEGER, servo_state),
	FIELD(wrs_p_globals, ASN_INTEGER, tracking_enabled),
	FIELD(wrs_p_globals, ASN_OCTET_STR, sync_source),
	FIELD(wrs_p_globals, ASN_COUNTER64, clock_offset),
	FIELD(wrs_p_globals, ASN_INTEGER, skew),
	FIELD(wrs_p_globals, ASN_COUNTER64, rtt),
	FIELD(wrs_p_globals, ASN_UNSIGNED, llength),
	FIELD(wrs_p_globals, ASN_UNSIGNED, servo_updates),
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
	FIELD(wrs_p_perport, ASN_INTEGER, link_up),
	FIELD(wrs_p_perport, ASN_INTEGER, port_mode),
	FIELD(wrs_p_perport, ASN_INTEGER, port_locked),
	FIELD(wrs_p_perport, ASN_OCTET_STR, peer_id),
};

static void wrs_ppsi_get_globals(void)
{
	unsigned ii;
	unsigned retries = 0;

	memset(&wrs_p_globals, 0, sizeof(wrs_p_globals));
	while (1) {
		ii = wrs_shm_seqbegin(ppsi_head);

		strncpy(wrs_p_globals.servo_state_name,
			ppsi_servo->servo_state_name,
			sizeof(ppsi_servo->servo_state_name));
		wrs_p_globals.servo_state = ppsi_servo->state;
		wrs_p_globals.tracking_enabled = ppsi_servo->tracking_enabled;
		wrs_p_globals.clock_offset = ppsi_servo->offset;
		wrs_p_globals.skew = ppsi_servo->skew;
		wrs_p_globals.rtt = ppsi_servo->picos_mu;
		wrs_p_globals.llength = (uint32_t)(ppsi_servo->delta_ms/1e12 *
					300e6 / 1.55);
		wrs_p_globals.servo_updates = ppsi_servo->update_count;

		retries++;
		if (retries > 100) {
			snmp_log(LOG_ERR, "%s: too many retries to read PPSI\n",
				 __func__);
			retries = 0;
			}
		if (!wrs_shm_seqretry(ppsi_head, ii))
			break; /* consistent read */
		usleep(1000);
	}
}

void init_shm(void)
{
	struct hal_shmem_header *h;

	hal_head = wrs_shm_get(wrs_shm_hal, "", WRS_SHM_READ);
	if (!hal_head) {
		snmp_log(LOG_ERR, "unable to open shm for HAL!\n");
		exit(-1);
	}
	/* check hal's shm version */
	if (hal_head->version != HAL_SHMEM_VERSION) {
		snmp_log(LOG_ERR, "unknown hal's shm version %i "
			 "(known is %i)\n", hal_head->version,
			 HAL_SHMEM_VERSION);
		exit(-1);
	}

	h = (void *)hal_head + hal_head->data_off;
	/* Assume number of ports does not change in runtime */
	hal_nports_local = h->nports;
	if (hal_nports_local > WRS_N_PORTS) {
		snmp_log(LOG_ERR, "Too many ports reported by HAL. "
			"%d vs %d supported\n",
			hal_nports_local, WRS_N_PORTS);
		exit(-1);
	}
	/* Even after HAL restart, HAL will place structures at the same
	 * addresses. No need to re-dereference pointer at each read. */
	hal_ports = wrs_shm_follow(hal_head, h->ports);
	if (!hal_ports) {
		snmp_log(LOG_ERR, "Unalbe to follow hal_ports pointer in HAL's"
			 " shmem");
		exit(-1);
	}

	ppsi_head = wrs_shm_get(wrs_shm_ptp, "", WRS_SHM_READ);
	if (!ppsi_head) {
		snmp_log(LOG_ERR, "unable to open shm for PPSI!\n");
		exit(-1);
	}

	/* check hal's shm version */
	if (ppsi_head->version != WRS_PPSI_SHMEM_VERSION) {
		snmp_log(LOG_ERR, "wr_mon: unknown PPSI's shm version %i "
			"(known is %i)\n",
			ppsi_head->version, WRS_PPSI_SHMEM_VERSION);
		exit(-1);
	}
	ppg = (void *)ppsi_head + ppsi_head->data_off;

	ppsi_servo = wrs_shm_follow(ppsi_head, ppg->global_ext_data);
	if (!ppsi_servo) {
		snmp_log(LOG_ERR, "Cannot follow ppsi_servo in shmem.\n");
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
	while (1) {
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
		if (!wrs_shm_seqretry(hal_head, ii))
			break; /* consistent read */
		usleep(1000);
	}

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

	/* Set the data context (1..4)
	 * Cannot be set to 0, because netsnmp_extract_iterator_context returns
	 * NULL in function wrsPstats_handler when table is over */
	*data_context = (void *)i;
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
		if (!wrport)
			/* NULL returned from
				 * netsnmp_extract_iterator_context shuld be
				 * interpreted as end of table */
			break;
		/* change range of wrport (1..4 (snmp is 1 based) ->
			 * 0..3 (wrs_p_array/data array is 0 based)) */
		wrport--;
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
