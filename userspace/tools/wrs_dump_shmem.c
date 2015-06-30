#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <libwr/shmem.h>
#include <libwr/hal_shmem.h>
#include <libwr/rtu_shmem.h>
#include <ppsi/ppsi.h>
#include <ppsi-wrs.h>


/*  be safe, in case some other header had them slightly differently */
#undef container_of
#undef offsetof
#undef ARRAY_SIZE

#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

char *name_id_to_name[WRS_SHM_N_NAMES] = {
	[wrs_shm_ptp] = "ptpd/ppsi",
	[wrs_shm_rtu] = "wrsw_rtud",
	[wrs_shm_hal] = "wrsw_hal",
	[wrs_shm_vlan] = "wrs_vlans",
};

/*
 * To ease copying from header files, allow int, char and other known types.
 * Please add more type as more structures are included here
 */
enum dump_type {
	dump_type_char, /* for zero-terminated strings */
	dump_type_bina, /* for binary stull in MAC format */
	/* normal types follow */
	dump_type_uint32_t,
	dump_type_uint16_t,
	dump_type_int,
	dump_type_unsigned_long,
	dump_type_unsigned_char,
	dump_type_unsigned_short,
	dump_type_double,
	dump_type_float,
	dump_type_pointer,
	/* and strange ones, from IEEE */
	dump_type_UInteger64,
	dump_type_Integer64,
	dump_type_UInteger32,
	dump_type_Integer32,
	dump_type_UInteger16,
	dump_type_Integer16,
	dump_type_UInteger8,
	dump_type_Integer8,
	dump_type_Enumeration8,
	dump_type_Boolean,
	dump_type_ClockIdentity,
	dump_type_PortIdentity,
	dump_type_ClockQuality,
	/* and this is ours */
	dump_type_TimeInternal,
	dump_type_ip_address,
	dump_type_sfp_flags,
	dump_type_port_mode,
	dump_type_sensor_temp,
};

static int dump_all_rtu_entries = 0; /* rtu exports 4096 vlans and 2048 htab
				 entries */

/*
 * A structure to dump fields. This is meant to simplify things, see use here
 */
struct dump_info {
	char *name;
	enum dump_type type;   /* see above */
	int offset;
	int size;  /* only for strings or binary strings */
};

void dump_one_field(void *addr, struct dump_info *info)
{
	void *p = addr + info->offset;
	struct TimeInternal *ti = p;
	struct PortIdentity *pi = p;
	struct ClockQuality *cq = p;
	char format[16];
	int i;

	printf("        %-30s ", info->name); /* name includes trailing ':' */
	switch(info->type) {
	case dump_type_char:
		sprintf(format,"\"%%.%is\"\n", info->size);
		printf(format, (char *)p);
		break;
	case dump_type_bina:
		for (i = 0; i < info->size; i++)
			printf("%02x%c", ((unsigned char *)p)[i],
			       i == info->size - 1 ? '\n' : ':');
		break;
	case dump_type_UInteger64:
		printf("%lld\n", *(unsigned long long *)p);
		break;
	case dump_type_Integer64:
		printf("%lld\n", *(long long *)p);
		break;
	case dump_type_uint32_t:
		printf("0x%08lx\n", (long)*(uint32_t *)p);
		break;
	case dump_type_Integer32:
	case dump_type_int:
		printf("%i\n", *(int *)p);
		break;
	case dump_type_UInteger32:
	case dump_type_unsigned_long:
		printf("%li\n", *(unsigned long *)p);
		break;
	case dump_type_unsigned_char:
	case dump_type_UInteger8:
	case dump_type_Integer8:
	case dump_type_Enumeration8:
	case dump_type_Boolean:
		printf("%i\n", *(unsigned char *)p);
		break;
	case dump_type_UInteger16:
	case dump_type_uint16_t:
	case dump_type_unsigned_short:
		printf("%i\n", *(unsigned short *)p);
		break;
	case dump_type_double:
		printf("%lf\n", *(double *)p);
		break;
	case dump_type_float:
		printf("%f\n", *(float *)p);
		break;
	case dump_type_pointer:
		printf("%p\n", *(void **)p);
		break;
	case dump_type_Integer16:
		printf("%i\n", *(short *)p);
		break;
	case dump_type_TimeInternal:
		printf("correct %i: %10i.%09i:%04i\n", ti->correct,
		       ti->seconds, ti->nanoseconds, ti->phase);
		break;

	case dump_type_ip_address:
		for (i = 0; i < 4; i++)
			printf("%02x%c", ((unsigned char *)p)[i],
			       i == 3 ? '\n' : ':');
		break;

	case dump_type_ClockIdentity: /* Same as binary */
		for (i = 0; i < sizeof(ClockIdentity); i++)
			printf("%02x%c", ((unsigned char *)p)[i],
			       i == sizeof(ClockIdentity) - 1 ? '\n' : ':');
		break;

	case dump_type_PortIdentity: /* Same as above plus port */
		for (i = 0; i < sizeof(ClockIdentity); i++)
			printf("%02x%c", ((unsigned char *)p)[i],
			       i == sizeof(ClockIdentity) - 1 ? '.' : ':');
		printf("%04x (%i)\n", pi->portNumber, pi->portNumber);
		break;

	case dump_type_ClockQuality:
		printf("class %i, accuracy %02x (%i), logvariance %i\n",
		       cq->clockClass, cq->clockAccuracy, cq->clockAccuracy,
		       cq->offsetScaledLogVariance);
		break;
	case dump_type_sfp_flags:
		if (*(uint32_t *)p & SFP_FLAG_CLASS_DATA)
			printf("SFP class data, ");
		if (*(uint32_t *)p & SFP_FLAG_DEVICE_DATA)
			printf("SFP device data, ");
		if (*(uint32_t *)p & SFP_FLAG_1GbE)
			printf("SFP is 1GbE, ");
		if (*(uint32_t *)p & SFP_FLAG_IN_DB)
			printf("SFP in data base, ");
		printf("\n");
		break;
	case dump_type_port_mode:
		switch (*(uint32_t *)p) {
		case HEXP_PORT_MODE_WR_MASTER:
			printf("WR Master\n");
			break;
		case HEXP_PORT_MODE_WR_SLAVE:
			printf("WR Slave\n");
			break;
		case HEXP_PORT_MODE_NON_WR:
			printf("Non-WR\n");
			break;
		case HEXP_PORT_MODE_WR_M_AND_S:
			printf("Auto\n");
			break;
		default:
			printf("Undefined\n");
			break;
		}
		break;
	case dump_type_sensor_temp:
		printf("%f\n", ((float)(*(int *)p >> 4)) / 16.0);
		break;
	}
}
void dump_many_fields(void *addr, struct dump_info *info, int ninfo)
{
	int i;

	if (!addr) {
		fprintf(stderr, "dump: pointer not valid\n");
		return;
	}
	for (i = 0; i < ninfo; i++)
		dump_one_field(addr, info + i);
}

/* the macro below relies on an externally-defined structure type */
#define DUMP_FIELD(_type, _fname) { \
	.name = #_fname ":",  \
	.type = dump_type_ ## _type, \
	.offset = offsetof(DUMP_STRUCT, _fname), \
}
#define DUMP_FIELD_SIZE(_type, _fname, _size) { \
	.name = #_fname ":",		\
	.type = dump_type_ ## _type, \
	.offset = offsetof(DUMP_STRUCT, _fname), \
	.size = _size, \
}

#undef DUMP_STRUCT
#define DUMP_STRUCT struct hal_shmem_header
struct dump_info hal_shmem_info [] = {
	DUMP_FIELD(int, nports),
	DUMP_FIELD(sensor_temp, temp.fpga),
	DUMP_FIELD(sensor_temp, temp.pll),
	DUMP_FIELD(sensor_temp, temp.psl),
	DUMP_FIELD(sensor_temp, temp.psr),
};

/* map for fields of hal_port_state (hal_shmem.h) */
#undef DUMP_STRUCT
#define DUMP_STRUCT struct hal_port_state
struct dump_info hal_port_info [] = {
	DUMP_FIELD(int, in_use),
	DUMP_FIELD_SIZE(char, name, 16),
	DUMP_FIELD_SIZE(bina, hw_addr, 6),
	DUMP_FIELD(int, hw_index),
	DUMP_FIELD(int, fd),
	DUMP_FIELD(int, hw_addr_auto),
	DUMP_FIELD(port_mode, mode),
	DUMP_FIELD(int, state),
	DUMP_FIELD(int, fiber_index),
	DUMP_FIELD(int, locked),
	/* these fields are defined as uint32_t but we prefer %i to %x */
	DUMP_FIELD(int, calib.phy_rx_min),
	DUMP_FIELD(int, calib.phy_tx_min),
	DUMP_FIELD(int, calib.delta_tx_phy),
	DUMP_FIELD(int, calib.delta_rx_phy),
	DUMP_FIELD(int, calib.delta_tx_board),
	DUMP_FIELD(int, calib.delta_rx_board),
	DUMP_FIELD(int, calib.rx_calibrated),
	DUMP_FIELD(int, calib.tx_calibrated),

	/* Another internal structure, with a final pointer */
	DUMP_FIELD(sfp_flags, calib.sfp.flags),
	DUMP_FIELD_SIZE(char, calib.sfp.vendor_name, 16),
	DUMP_FIELD_SIZE(char, calib.sfp.part_num, 16),
	DUMP_FIELD_SIZE(char, calib.sfp.vendor_serial, 16),
	DUMP_FIELD(double,    calib.sfp.alpha),
	DUMP_FIELD(int,       calib.sfp.delta_tx_ps),
	DUMP_FIELD(int,       calib.sfp.delta_rx_ps),
	DUMP_FIELD(int,       calib.sfp.tx_wl),
	DUMP_FIELD(int,       calib.sfp.rx_wl),
	DUMP_FIELD(pointer,   calib.sfp.next),

	DUMP_FIELD(uint32_t, phase_val),
	DUMP_FIELD(int, phase_val_valid),
	DUMP_FIELD(int, tx_cal_pending),
	DUMP_FIELD(int, rx_cal_pending),
	DUMP_FIELD(int, lock_state),
	DUMP_FIELD(uint32_t, clock_period),
	DUMP_FIELD(uint32_t, t2_phase_transition),
	DUMP_FIELD(uint32_t, t4_phase_transition),
	DUMP_FIELD(uint32_t, ep_base),
};

int dump_hal_mem(struct wrs_shm_head *head)
{
	struct hal_shmem_header *h;
	struct hal_port_state *p;
	int i, n;

	if (head->version != HAL_SHMEM_VERSION) {
		fprintf(stderr, "dump hal: unknown version %i (known is %i)\n",
			head->version, HAL_SHMEM_VERSION);
		return -1;
	}
	h = (void *)head + head->data_off;

	/* dump hal's shmem */
	dump_many_fields(h, hal_shmem_info, ARRAY_SIZE(hal_shmem_info));

	n = h->nports;
	p = wrs_shm_follow(head, h->ports);

	if (!p) {
		fprintf(stderr, "dump hal: cannot follow pointer to *ports\n");
		return -1;
	}

	for (i = 0; i < n; i++, p++) {
		printf("dump port %i\n", i);
		dump_many_fields(p, hal_port_info, ARRAY_SIZE(hal_port_info));
	}
	return 0;
}

/* map for fields of rtud structures */
#undef DUMP_STRUCT
#define DUMP_STRUCT struct rtu_filtering_entry
struct dump_info htab_info[] = {
	DUMP_FIELD(int, addr.hash),
	DUMP_FIELD(int, addr.bucket),
	DUMP_FIELD(int, valid),
	DUMP_FIELD(int, end_of_bucket),
	DUMP_FIELD(int, is_bpdu),
	DUMP_FIELD_SIZE(bina, mac, ETH_ALEN),
	DUMP_FIELD(UInteger8, fid),
	DUMP_FIELD(uint32_t, port_mask_src),
	DUMP_FIELD(uint32_t, port_mask_dst),
	DUMP_FIELD(int, drop_when_source),
	DUMP_FIELD(int, drop_when_dest),
	DUMP_FIELD(int, drop_unmatched_src_ports),
	DUMP_FIELD(UInteger32, last_access_t),
	DUMP_FIELD(int, force_remove),
	DUMP_FIELD(UInteger8, prio_src),
	DUMP_FIELD(int, has_prio_src),
	DUMP_FIELD(int, prio_override_src),
	DUMP_FIELD(UInteger8, prio_dst),
	DUMP_FIELD(int, has_prio_dst),
	DUMP_FIELD(int, prio_override_dst),
	DUMP_FIELD(int, dynamic),
	DUMP_FIELD(int, age),
};

#undef DUMP_STRUCT
#define DUMP_STRUCT struct rtu_vlan_table_entry
struct dump_info vlan_info[] = {
	DUMP_FIELD(uint32_t, port_mask),
	DUMP_FIELD(UInteger8, fid),
	DUMP_FIELD(UInteger8, prio),
	DUMP_FIELD(int, has_prio),
	DUMP_FIELD(int, prio_override),
	DUMP_FIELD(int, drop),
};

int dump_rtu_mem(struct wrs_shm_head *head)
{
	struct rtu_shmem_header *rtu_h;
	struct rtu_filtering_entry *rtu_filters;
	struct rtu_filtering_entry *rtu_filters_cur;
	struct rtu_vlan_table_entry *rtu_vlans;
	int i, j;

	if (head->version != RTU_SHMEM_VERSION) {
		fprintf(stderr, "dump rtu: unknown version %i (known is %i)\n",
			head->version, RTU_SHMEM_VERSION);
		return -1;
	}
	rtu_h = (void *)head + head->data_off;
	rtu_filters = wrs_shm_follow(head, rtu_h->filters);
	rtu_vlans = wrs_shm_follow(head, rtu_h->vlans);

	if ((!rtu_filters) || (!rtu_vlans)) {
		fprintf(stderr, "dump rtu: cannot follow pointer in shm\n");
		return -1;
	}

	for (i = 0; i < HTAB_ENTRIES; i++) {
		for (j = 0; j < RTU_BUCKETS; j++) {
			rtu_filters_cur = rtu_filters + i*RTU_BUCKETS + j;
			if ((!dump_all_rtu_entries)
			    && (!rtu_filters_cur->valid))
				/* don't display empty entries */
				continue;
			printf("dump htab[%d][%d]\n", i, j);
			dump_many_fields(rtu_filters_cur, htab_info,
					 ARRAY_SIZE(htab_info));
		}
	}

	for (i = 0; i < NUM_VLANS; i++, rtu_vlans++) {
		if ((!dump_all_rtu_entries) && (rtu_vlans->drop != 0
			    && rtu_vlans->port_mask == 0x0))
			/* don't display empty entries */
			continue;
		printf("dump vlan %i\n", i);
		dump_many_fields(rtu_vlans, vlan_info, ARRAY_SIZE(vlan_info));
	}
	return 0;
}

/* map for fields of ppsi structures */
#undef DUMP_STRUCT
#define DUMP_STRUCT struct pp_globals
struct dump_info ppg_info [] = {
	DUMP_FIELD(pointer, pp_instances),	/* FIXME: follow this */
	DUMP_FIELD(pointer, servo),		/* FIXME: follow this */
	DUMP_FIELD(pointer, rt_opts),
	DUMP_FIELD(pointer, defaultDS),
	DUMP_FIELD(pointer, currentDS),
	DUMP_FIELD(pointer, parentDS),
	DUMP_FIELD(pointer, timePropertiesDS),
	DUMP_FIELD(int, ebest_idx),
	DUMP_FIELD(int, ebest_updated),
	DUMP_FIELD(int, nlinks),
	DUMP_FIELD(int, max_links),
	//DUMP_FIELD(struct pp_globals_cfg cfg),
	DUMP_FIELD(int, rxdrop),
	DUMP_FIELD(int, txdrop),
	DUMP_FIELD(pointer, arch_data),
	DUMP_FIELD(pointer, global_ext_data),
};

#undef DUMP_STRUCT
#define DUMP_STRUCT DSDefault /* Horrible typedef */
struct dump_info dsd_info [] = {
	DUMP_FIELD(Boolean, twoStepFlag),
	DUMP_FIELD(ClockIdentity, clockIdentity),
	DUMP_FIELD(UInteger16, numberPorts),
	DUMP_FIELD(ClockQuality, clockQuality),
	DUMP_FIELD(UInteger8, priority1),
	DUMP_FIELD(UInteger8, priority2),
	DUMP_FIELD(UInteger8, domainNumber),
	DUMP_FIELD(Boolean, slaveOnly),
};

#undef DUMP_STRUCT
#define DUMP_STRUCT DSCurrent /* Horrible typedef */
struct dump_info dsc_info [] = {
	DUMP_FIELD(UInteger16, stepsRemoved),
	DUMP_FIELD(TimeInternal, offsetFromMaster),
	DUMP_FIELD(TimeInternal, meanPathDelay), /* oneWayDelay */
	DUMP_FIELD(UInteger16, primarySlavePortNumber),
};

#undef DUMP_STRUCT
#define DUMP_STRUCT DSParent /* Horrible typedef */
struct dump_info dsp_info [] = {
	DUMP_FIELD(PortIdentity, parentPortIdentity),
	DUMP_FIELD(UInteger16, observedParentOffsetScaledLogVariance),
	DUMP_FIELD(Integer32, observedParentClockPhaseChangeRate),
	DUMP_FIELD(ClockIdentity, grandmasterIdentity),
	DUMP_FIELD(ClockQuality, grandmasterClockQuality),
	DUMP_FIELD(UInteger8, grandmasterPriority1),
	DUMP_FIELD(UInteger8, grandmasterPriority2),
};

#undef DUMP_STRUCT
#define DUMP_STRUCT DSTimeProperties /* Horrible typedef */
struct dump_info dstp_info [] = {
	DUMP_FIELD(Integer16, currentUtcOffset),
	DUMP_FIELD(Boolean, currentUtcOffsetValid),
	DUMP_FIELD(Boolean, leap59),
	DUMP_FIELD(Boolean, leap61),
	DUMP_FIELD(Boolean, timeTraceable),
	DUMP_FIELD(Boolean, frequencyTraceable),
	DUMP_FIELD(Boolean, ptpTimescale),
	DUMP_FIELD(Enumeration8, timeSource),
};

#undef DUMP_STRUCT
#define DUMP_STRUCT struct wr_servo_state
struct dump_info servo_state_info [] = {
	DUMP_FIELD_SIZE(char, if_name, 16),
	DUMP_FIELD(unsigned_long, flags),
	DUMP_FIELD(int, state),
	DUMP_FIELD(Integer32, delta_tx_m),
	DUMP_FIELD(Integer32, delta_rx_m),
	DUMP_FIELD(Integer32, delta_tx_s),
	DUMP_FIELD(Integer32, delta_rx_s),
	DUMP_FIELD(Integer32, fiber_fix_alpha),
	DUMP_FIELD(Integer32, clock_period_ps),
	DUMP_FIELD(TimeInternal, t1),
	DUMP_FIELD(TimeInternal, t2),
	DUMP_FIELD(TimeInternal, t3),
	DUMP_FIELD(TimeInternal, t4),
	DUMP_FIELD(Integer32, delta_ms_prev),
	DUMP_FIELD(int, missed_iters),
	DUMP_FIELD(TimeInternal, mu),		/* half of the RTT */
	DUMP_FIELD(Integer64, picos_mu),
	DUMP_FIELD(Integer32, cur_setpoint),
	DUMP_FIELD(Integer32, delta_ms),
	DUMP_FIELD(UInteger32, update_count),
	DUMP_FIELD(int, tracking_enabled),
	DUMP_FIELD_SIZE(char, servo_state_name, 32),
	DUMP_FIELD(Integer64, skew),
	DUMP_FIELD(Integer64, offset),
	DUMP_FIELD(UInteger32, n_err_state),
	DUMP_FIELD(UInteger32, n_err_offset),
	DUMP_FIELD(UInteger32, n_err_delta_rtt),
	DUMP_FIELD(UInteger32, n_err_rxtx_deltas),
};

#undef DUMP_STRUCT
#define DUMP_STRUCT struct pp_instance
struct dump_info ppi_info [] = {
	DUMP_FIELD(int, state),
	DUMP_FIELD(int, next_state),
	DUMP_FIELD(int, next_delay),
	DUMP_FIELD(int, is_new_state),
	DUMP_FIELD(pointer, arch_data),
	DUMP_FIELD(pointer, ext_data),
	DUMP_FIELD(unsigned_long, d_flags),
	DUMP_FIELD(unsigned_char, flags),
	DUMP_FIELD(unsigned_char, role),
	DUMP_FIELD(unsigned_char, proto),
	DUMP_FIELD(pointer, glbs),
	DUMP_FIELD(pointer, n_ops),
	DUMP_FIELD(pointer, t_ops),
	DUMP_FIELD(pointer, __tx_buffer),
	DUMP_FIELD(pointer, __rx_buffer),
	DUMP_FIELD(pointer, tx_frame),
	DUMP_FIELD(pointer, rx_frame),
	DUMP_FIELD(pointer, tx_ptp),
	DUMP_FIELD(pointer, rx_ptp),

	/* This is a sub-structure */
	DUMP_FIELD(int, ch[0].fd),
	DUMP_FIELD(pointer, ch[0].custom),
	DUMP_FIELD(pointer, ch[0].arch_data),
	DUMP_FIELD_SIZE(bina, ch[0].addr, 6),
	DUMP_FIELD(int, ch[0].pkt_present),
	DUMP_FIELD(int, ch[1].fd),
	DUMP_FIELD(pointer, ch[1].custom),
	DUMP_FIELD(pointer, ch[1].arch_data),
	DUMP_FIELD_SIZE(bina, ch[1].addr, 6),
	DUMP_FIELD(int, ch[1].pkt_present),

	DUMP_FIELD(ip_address, mcast_addr),
	DUMP_FIELD(int, tx_offset),
	DUMP_FIELD(int, rx_offset),
	DUMP_FIELD_SIZE(bina, peer, 6),
	DUMP_FIELD(uint16_t, peer_vid),

	DUMP_FIELD(TimeInternal, t1),
	DUMP_FIELD(TimeInternal, t2),
	DUMP_FIELD(TimeInternal, t3),
	DUMP_FIELD(TimeInternal, t4),
	DUMP_FIELD(TimeInternal, cField),
	DUMP_FIELD(TimeInternal, last_rcv_time),
	DUMP_FIELD(TimeInternal, last_snt_time),
	DUMP_FIELD(UInteger16, frgn_rec_num),
	DUMP_FIELD(Integer16,  frgn_rec_best),
	//DUMP_FIELD(struct pp_frgn_master frgn_master[PP_NR_FOREIGN_RECORDS]),
	DUMP_FIELD(pointer, portDS),
	//DUMP_FIELD(unsigned long timeouts[__PP_TO_ARRAY_SIZE]),
	DUMP_FIELD(UInteger16, recv_sync_sequence_id),
	DUMP_FIELD(Integer8, log_min_delay_req_interval),
	//DUMP_FIELD(UInteger16 sent_seq[__PP_NR_MESSAGES_TYPES]),
	DUMP_FIELD_SIZE(bina, received_ptp_header, sizeof(MsgHeader)),
	//DUMP_FIELD(pointer, iface_name),
	//DUMP_FIELD(pointer, port_name),
	DUMP_FIELD(int, port_idx),
	DUMP_FIELD(int, vlans_array_len),
	/* FIXME: array */
	DUMP_FIELD(int, nvlans),

	/* sub structure */
	DUMP_FIELD_SIZE(char, cfg.port_name, 16),
	DUMP_FIELD_SIZE(char, cfg.iface_name, 16),
	DUMP_FIELD(int, cfg.ext),
	DUMP_FIELD(int, cfg.ext),

	DUMP_FIELD(unsigned_long, ptp_tx_count),
	DUMP_FIELD(unsigned_long, ptp_rx_count),
};

int dump_ppsi_mem(struct wrs_shm_head *head)
{
	struct pp_globals *ppg;
	struct pp_instance *ppi;
	DSDefault *dsd;
	DSCurrent *dsc;
	DSParent *dsp;
	DSTimeProperties *dstp;
	struct wr_servo_state *global_ext_data;
	int i;

	if (head->version != WRS_PPSI_SHMEM_VERSION) {
		fprintf(stderr, "dump ppsi: unknown version %i (known is %i)\n",
			head->version, WRS_PPSI_SHMEM_VERSION);
		return -1;
	}
	ppg = (void *)head + head->data_off;
	printf("ppsi globals:\n");
	dump_many_fields(ppg, ppg_info, ARRAY_SIZE(ppg_info));

	dsd = wrs_shm_follow(head, ppg->defaultDS);
	printf("default data set:\n");
	dump_many_fields(dsd, dsd_info, ARRAY_SIZE(dsd_info));

	dsc = wrs_shm_follow(head, ppg->currentDS);
	printf("current data set:\n");
	dump_many_fields(dsc, dsc_info, ARRAY_SIZE(dsc_info));

	dsp = wrs_shm_follow(head, ppg->parentDS);
	printf("parent data set:\n");
	dump_many_fields(dsp, dsp_info, ARRAY_SIZE(dsp_info));

	dstp = wrs_shm_follow(head, ppg->timePropertiesDS);
	printf("time properties data set:\n");
	dump_many_fields(dstp, dstp_info, ARRAY_SIZE(dstp_info));

	global_ext_data = wrs_shm_follow(head, ppg->global_ext_data);
	printf("global external data set:\n");
	dump_many_fields(global_ext_data, servo_state_info,
			 ARRAY_SIZE(servo_state_info));

	ppi = wrs_shm_follow(head, ppg->pp_instances);
	for (i = 0; i < ppg->nlinks; i++) {
		printf("ppsi instance %i:\n", i);
		dump_many_fields(ppi + i, ppi_info, ARRAY_SIZE(ppi_info));
	}

	return 0; /* this is complete */
}



int dump_any_mem(struct wrs_shm_head *head)
{
	unsigned char *p = (void *)head;
	int i, j;

	p += head->data_off;
	for (i = 0; i < head->data_size; i++) {
		if (i % 16 == 0)
			printf("%04lx: ", i + head->data_off);
		printf("%02x ", p[i]);
		if (i % 16 == 15) { /* ascii row */
			printf("    ");
			for (j = i & ~15; j <= i; j++)
				printf("%c", p[j] >= 0x20 && p[j] < 0x7f
				       ? p[j] : '.');
			printf("\n");
		}
	}
	/* yes, last row has no ascii trailer. Who cares */
	if (head ->data_size ^ 0xf)
		printf("\n");
	return 0;
}

typedef int (dump_f)(struct wrs_shm_head *head);

dump_f *name_id_to_f[WRS_SHM_N_NAMES] = {
	[wrs_shm_hal] = dump_hal_mem,
	[wrs_shm_ptp] = dump_ppsi_mem,
	[wrs_shm_rtu] = dump_rtu_mem,
};

void print_info(char *prgname)
{
	printf("usage: %s [parameters]\n", prgname);
	printf(""
		"             Dump shmem\n"
		"   -a        Dump all rtu entries. By default only valid\n"
		"             entries are printed. Note there are 2048 htab\n"
		"             and 4096 vlan entries!\n"
		"   -h        Show this message\n");

}

int main(int argc, char **argv)
{
	struct wrs_shm_head *head;
	dump_f *f;
	void *m;
	int i;
	int c;

	while ((c = getopt(argc, argv, "ah")) != -1) {
		switch (c) {
		case 'a':
			dump_all_rtu_entries = 1;
			break;
		case 'h':
		default:
			print_info(argv[0]);
			exit(1);
		}
	}
	for (i = 0; i < WRS_SHM_N_NAMES; i++) {
		m = wrs_shm_get(i, "reader", 0);
		if (!m) {
			fprintf(stderr, "%s: can't attach memory area %i: %s\n",
				argv[0], i, strerror(errno));
			continue;
		}
		head = m;
		if (!head->pidsequence) {
			printf("ID %i (\"%s\"): no data\n",
			       i, name_id_to_name[i]);
			wrs_shm_put(m);
			continue;
		}
		if (head->pid) {
			printf("ID %i (\"%s\"): pid %i (%s, %i iterations)\n",
			       i, head->name, head->pid,
			       kill(head->pid, 0) < 0 ? "dead" : "alive",
			       head->pidsequence);
		} else {
			printf("ID %i (\"%s\"): no pid (after %i iterations)\n",
			       i, head->name, head->pidsequence);
		}
		f = name_id_to_f[i];

		/* if the area-specific function fails, fall back to generic */
		if (!f || f(head) != 0)
			dump_any_mem(head);
		wrs_shm_put(m);
		printf("\n"); /* separate one area from the next */
	}
	return 0;
}
