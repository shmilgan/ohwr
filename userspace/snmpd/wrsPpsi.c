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

#include <ppsi/ppsi.h>
#include <minipc.h>

#include "wrsSnmp.h"

#define PPSI_CACHE_TIMEOUT 1 /* 1 second: refresh every so often */

/* Our data */
static struct wrs_p_globals {
	ClockIdentity gm_id;
	ClockIdentity my_id;
	int ppsi_mode;
	int ppsi_servo_state;
	int phase_tracking;
	char sync_source[32];
	int64_t clock_offset;
	int32_t skew;
	int64_t rtt;
	uint32_t llength;
	int64_t servo_updates;
} wrs_p_globals;

static struct wrs_p_perport {
	ClockIdentity peer_id;
	unsigned char  link_up;
	unsigned char  port_mode;
	unsigned char  port_locked;
} wrs_p_perport, wrs_p_array[WRS_N_PORTS];

/* Table-driven memcpy: declare how to pick fields (pickinfo) */
struct ppsi_pickinfo {
	int type; int offset; int len;
};

#define FIELD(_struct, _type, _field) {			\
		_type,					\
		offsetof(struct _struct, _field),	\
		sizeof(_struct._field)			\
		 }

static struct ppsi_pickinfo g_pickinfo[] = {
	FIELD(wrs_p_globals, ASN_OCTET_STR, gm_id),
	FIELD(wrs_p_globals, ASN_OCTET_STR, my_id),
	FIELD(wrs_p_globals, ASN_INTEGER, ppsi_mode),
	FIELD(wrs_p_globals, ASN_INTEGER, ppsi_servo_state),
	FIELD(wrs_p_globals, ASN_INTEGER, phase_tracking),
	FIELD(wrs_p_globals, ASN_OCTET_STR, sync_source), /* special case! */
	FIELD(wrs_p_globals, ASN_COUNTER64, clock_offset),
	FIELD(wrs_p_globals, ASN_INTEGER, skew),
	FIELD(wrs_p_globals, ASN_COUNTER64, rtt),
	FIELD(wrs_p_globals, ASN_INTEGER, llength),
	FIELD(wrs_p_globals, ASN_COUNTER64, servo_updates),
};

static struct ppsi_pickinfo p_pickinfo[] = {
	FIELD(wrs_p_perport, ASN_INTEGER, link_up),
	FIELD(wrs_p_perport, ASN_INTEGER, port_mode),
	FIELD(wrs_p_perport, ASN_INTEGER, port_locked),
	FIELD(wrs_p_perport, ASN_OCTET_STR, peer_id),
};

/* This is the filler for the global scalars */
static int ppsi_g_group(netsnmp_mib_handler          *handler,
			netsnmp_handler_registration *reginfo,
			netsnmp_agent_request_info   *reqinfo,
			netsnmp_request_info         *requests)
{
	int obj; /* the final index */
	struct ppsi_pickinfo *pi;
	void *ptr;
	int len;

	/* FIXME: retrieve information from ppsi itself */

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
			;//len = strlen(ptr);
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
ppsi_p_next_entry( void **loop_context,
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
	snmp_set_var_value(index, (u_char*)&i, sizeof(i));

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
	*loop_context = (void*)0; /* first counter */
	return ppsi_p_next_entry(loop_context, data_context, index, data);
}

static int
ppsi_p_load(netsnmp_cache *cache, void *vmagic)
{
	/* FIXME: load information */
	strcpy(wrs_p_globals.sync_source, "wr I suppose");
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


	for (request=requests; request; request=request->next) {
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
