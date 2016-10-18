#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <libwr/shmem.h>
#include <libwr/hal_shmem.h>
#include <libwr/rtu_shmem.h>
#define HIST_SHMEM_STRUCTURES
#include <libwr/hist_shmem.h>
#include <libwr/softpll_export.h>
#include <libwr/util.h>
#include <ppsi/ppsi.h>
#include <ppsi-wrs.h>

/*  be safe, in case some other header had them slightly differently */
#undef container_of
#undef offsetof
#undef ARRAY_SIZE

#define FPGA_SPLL_STAT 0x10006800
#define SPLL_MAGIC 0x5b1157a7

#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static int dump_all_entries = 0; /* rtu exports 4096 vlans and 2048 htab
				  * entries; wrs_hist's database with SFP has
				  * 100 entries  */

char *name_id_to_name[WRS_SHM_N_NAMES] = {
	[wrs_shm_ptp] = "ptpd/ppsi",
	[wrs_shm_rtu] = "wrsw_rtud",
	[wrs_shm_hal] = "wrsw_hal",
	[wrs_shm_vlan] = "wrs_vlans",
	[wrs_shm_hist] = "wrs_hist",
};

/* index of a the greatest number describing the SPLL mode +1 */
#define SPLL_MODE_MAX_N 5
char *spll_mode_to_name[SPLL_MODE_MAX_N] = {
	[SPLL_MODE_GRAND_MASTER] = "Grand Master",
	[SPLL_MODE_FREE_RUNNING_MASTER] = "Free Runnig Master",
	[SPLL_MODE_SLAVE] = "Slave",
	[SPLL_MODE_DISABLED] = "Disabled",
};

/* index of a the greatest number describing the SPLL sequence +1 */
#define SPLL_SEQ_STATE_MAX_N 11
char *spll_seq_state_to_name[SPLL_SEQ_STATE_MAX_N] = {
	[SEQ_START_EXT] = "start ext",
	[SEQ_WAIT_EXT] = "wait ext",
	[SEQ_START_HELPER] = "start helper",
	[SEQ_WAIT_HELPER] = "wait helper",
	[SEQ_START_MAIN] = "start main",
	[SEQ_WAIT_MAIN] = "wait main",
	[SEQ_DISABLED] = "disabled",
	[SEQ_READY] = "ready",
	[SEQ_CLEAR_DACS] = "clear dacs",
	[SEQ_WAIT_CLEAR_DACS] = "wait clear dacs",
};

/* index of a the greatest number describing the SPLL align state +1 */
#define SPLL_ALIGN_STATE_MAX_N 11
char *spll_align_state_to_name[SPLL_ALIGN_STATE_MAX_N] = {
	[ALIGN_STATE_EXT_OFF] = "ext off",
	[ALIGN_STATE_START] = "start",
	[ALIGN_STATE_INIT_CSYNC] = "init csync",
	[ALIGN_STATE_WAIT_CSYNC] = "wait csync",
	[ALIGN_STATE_WAIT_SAMPLE] = "wait sample",
	[ALIGN_STATE_COMPENSATE_DELAY] = "compensate delay",
	[ALIGN_STATE_LOCKED] = "locked",
	[ALIGN_STATE_START_ALIGNMENT] = "start alignment",
	[ALIGN_STATE_START_MAIN] = "start main",
	[ALIGN_STATE_WAIT_CLKIN] = "wait clkIn",
	[ALIGN_STATE_WAIT_PLOCK] = "wait plock",
};

/*
 * To ease copying from header files, allow int, char and other known types.
 * Please add more type as more structures are included here
 */
enum dump_type {
	dump_type_char, /* for zero-terminated strings */
	dump_type_char_e, /* for zero-terminated strings with thw wrong
			   * endianess */
	dump_type_bina, /* for binary stull in MAC format */
	/* normal types follow */
	dump_type_uint32_t,
	dump_type_uint16_t,
	dump_type_uint8_t,
	dump_type_int8_t,
	dump_type_uint32_t_hex,
	dump_type_uint16_t_hex,
	dump_type_uint8_t_hex,
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
	/* SoftPLL's enumerations */
	dump_type_spll_mode,
	dump_type_spll_seq_state,
	dump_type_spll_align_state,
	/* rtu_filtering_entry enumerations */
	dump_type_rtu_filtering_entry_dynamic,
	dump_type_array_int,
	dump_type_asciitime,
	dump_type_asciiuptime,
	dump_type_hist_temp,
	dump_type_hist_sfp_present
};

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
	case dump_type_char_e: /* swap endianess */
		i = info->size;
		printf("\"");
		while (i > 0) {
			strncpy_e(format, (char *) p, 1);
			printf("%.4s", (char *)format);
			i -= 4;
			p = ((char *) p) + 4;
		}
		printf("\"\n");
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
	case dump_type_uint8_t:
	case dump_type_int8_t:
		printf("%i\n", *(unsigned char *)p);
		break;
	case dump_type_uint32_t_hex:
		printf("0x%08x\n", *(uint32_t *)p);
		break;
	case dump_type_uint16_t_hex:
		printf("0x%04x\n", *(uint16_t *)p);
		break;
	case dump_type_uint8_t_hex:
		printf("0x%02x\n", *(uint8_t *)p);
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
		case HEXP_PORT_MODE_NONE:
			printf("None\n");
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
	case dump_type_spll_mode:
		i = *(uint32_t *)p;
		switch (i) {
		case SPLL_MODE_GRAND_MASTER:
		case SPLL_MODE_FREE_RUNNING_MASTER:
		case SPLL_MODE_SLAVE:
		case SPLL_MODE_DISABLED:
			printf("%s(%d)\n", spll_mode_to_name[i], i);
			break;
		default:
			printf("Unknown(%d)\n", i);
		}
		break;
	case dump_type_spll_seq_state:
		i = *(uint32_t *)p;
		switch (i) {
		case SEQ_START_EXT:
		case SEQ_WAIT_EXT:
		case SEQ_START_HELPER:
		case SEQ_WAIT_HELPER:
		case SEQ_START_MAIN:
		case SEQ_WAIT_MAIN:
		case SEQ_DISABLED:
		case SEQ_READY:
		case SEQ_CLEAR_DACS:
		case SEQ_WAIT_CLEAR_DACS:
			printf("%s(%d)\n", spll_seq_state_to_name[i], i);
			break;
		default:
			printf("Unknown(%d)\n", i);
		}
		break;
	case dump_type_spll_align_state:
		i = *(uint32_t *)p;
		switch (i) {
		case ALIGN_STATE_EXT_OFF:
		case ALIGN_STATE_START:
		case ALIGN_STATE_INIT_CSYNC:
		case ALIGN_STATE_WAIT_CSYNC:
		case ALIGN_STATE_WAIT_SAMPLE:
		case ALIGN_STATE_COMPENSATE_DELAY:
		case ALIGN_STATE_LOCKED:
		case ALIGN_STATE_START_ALIGNMENT:
		case ALIGN_STATE_START_MAIN:
		case ALIGN_STATE_WAIT_CLKIN:
		case ALIGN_STATE_WAIT_PLOCK:
			printf("%s(%d)\n", spll_align_state_to_name[i], i);
			break;
		default:
			printf("Unknown(%d)\n", i);
		}
		break;
	case dump_type_rtu_filtering_entry_dynamic:
		i = *(uint32_t *)p;
		switch (i) {
		case RTU_ENTRY_TYPE_STATIC:
			printf("static\n");
			break;
		case RTU_ENTRY_TYPE_DYNAMIC:
			printf("dynamic\n");
			break;
		default:
			printf("Unknown(%d)\n", i);
		}
		break;
	case dump_type_array_int:
		{
		int *size = addr + info->size;
		for (i = 0; i < *size; i++)
			printf("%d ", *((int *)p + i));
		printf("\n");
		break;
		}
	case dump_type_asciitime:
		{
		time_t t;
		uint32_t tt;
		tt = *(uint32_t *)p;
		t = (time_t)tt;
		if (tt == 0) {
			printf("--\n");
			break;
		}
		printf("%s", ctime(&t));
		break;
		}
	case dump_type_asciiuptime:
		{
		time_t t;
		struct tm *tm;
		uint32_t tt;
		tt = *(uint32_t *)p;
		t = (time_t)tt;
		tm = gmtime(&t);
		if (tt == 0 || tm == 0) {
			printf("--\n");
			break;
		}
		printf("years %2d, months %2d, days %2d, hours %2d, min %2d, "
		       "seconds %2d\n",
		       tm->tm_year - 70, tm->tm_mon, tm->tm_mday - 1,
		       tm->tm_hour, tm->tm_min, tm->tm_sec);
		break;
		}
	case dump_type_hist_temp:
		{
		uint16_t *temp;
		int sensor;
		int entries;
		temp = (uint16_t *)p;
		printf("\n");
		printf("             temp range  FPGA   PLL   PSL   PSR\n");
		for (entries = 0; entries < WRS_HIST_TEMP_ENTRIES; entries++) {
			printf("              %9s", h_descr[entries].desc);
			for (sensor = 0; sensor < WRS_HIST_TEMP_SENSORS_N;
			     sensor++) {
				printf(" %5d",
				       *(temp + sensor * WRS_HIST_TEMP_ENTRIES
				         + entries));
			}
			printf("\n");
		}
		break;
		}
	case dump_type_hist_sfp_present:
		{
		uint32_t val;
		val = *(uint32_t *)p;
		if (val & WRS_HIST_SFP_PRESENT)
			printf("SFP present\n");
		else
			printf("SFP not present\n");
		break;
		}
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
	DUMP_FIELD(int, hal_mode),
	DUMP_FIELD(int, read_sfp_diag),
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
	DUMP_FIELD(int, has_sfp_diag),
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
		printf("dump port %i\n", i + 1);
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
	DUMP_FIELD(rtu_filtering_entry_dynamic, dynamic),
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
			if ((!dump_all_entries)
			    && (!rtu_filters_cur->valid))
				/* don't display empty entries */
				continue;
			printf("dump htab[%d][%d]\n", i, j);
			dump_many_fields(rtu_filters_cur, htab_info,
					 ARRAY_SIZE(htab_info));
		}
	}

	for (i = 0; i < NUM_VLANS; i++, rtu_vlans++) {
		if ((!dump_all_entries) && (rtu_vlans->drop != 0
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
	DUMP_FIELD(TimeInternal, t5),
	DUMP_FIELD(TimeInternal, t6),
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
	DUMP_FIELD(TimeInternal, update_time),
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
	DUMP_FIELD(int, role),
	DUMP_FIELD(int, proto),
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
	DUMP_FIELD(TimeInternal, t5),
	DUMP_FIELD(TimeInternal, t6),
	DUMP_FIELD(Integer32,  t4_cf),
	DUMP_FIELD(Integer32,  t6_cf),
	DUMP_FIELD(TimeInternal, cField),
	DUMP_FIELD(TimeInternal, last_rcv_time),
	DUMP_FIELD(TimeInternal, last_snt_time),
	DUMP_FIELD(UInteger16, frgn_rec_num),
	DUMP_FIELD(Integer16,  frgn_rec_best),
	//DUMP_FIELD(struct pp_frgn_master frgn_master[PP_NR_FOREIGN_RECORDS]),
	DUMP_FIELD(pointer, portDS),
	//DUMP_FIELD(unsigned long timeouts[__PP_TO_ARRAY_SIZE]),
	DUMP_FIELD(UInteger16, recv_sync_sequence_id),
	//DUMP_FIELD(UInteger16 sent_seq[__PP_NR_MESSAGES_TYPES]),
	DUMP_FIELD_SIZE(bina, received_ptp_header, sizeof(MsgHeader)),
	//DUMP_FIELD(pointer, iface_name),
	//DUMP_FIELD(pointer, port_name),
	DUMP_FIELD(int, port_idx),
	DUMP_FIELD(int, vlans_array_len),
	/* pass the size of a vlans array in the nvlans field */
	DUMP_FIELD_SIZE(array_int, vlans, offsetof(DUMP_STRUCT, nvlans)),
	DUMP_FIELD(int, nvlans),

	/* sub structure */
	DUMP_FIELD_SIZE(char, cfg.port_name, 16),
	DUMP_FIELD_SIZE(char, cfg.iface_name, 16),
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

#undef DUMP_STRUCT
#define DUMP_STRUCT struct spll_stats
struct dump_info spll_stats_info[] = {
	DUMP_FIELD(uint32_t, magic),	/* 0x5b1157a7 = SPLLSTAT ?;)*/
	DUMP_FIELD(int, ver),
	DUMP_FIELD(int, sequence),
	DUMP_FIELD(spll_mode, mode),
	DUMP_FIELD(int, irq_cnt),
	DUMP_FIELD(spll_seq_state, seq_state),
	DUMP_FIELD(spll_align_state, align_state),
	DUMP_FIELD(int, H_lock),
	DUMP_FIELD(int, M_lock),
	DUMP_FIELD(int, H_y),
	DUMP_FIELD(int, M_y),
	DUMP_FIELD(int, del_cnt),
	DUMP_FIELD(int, start_cnt),
	DUMP_FIELD_SIZE(char_e, commit_id, 32),
	DUMP_FIELD_SIZE(char_e, build_date, 16),
	DUMP_FIELD_SIZE(char_e, build_time, 16),
	DUMP_FIELD_SIZE(char_e, build_by, 32),
};

static int dump_spll_mem(struct spll_stats *spll)
{
	printf("ID: Soft PLL:\n");
	if (!spll) {
		fprintf(stderr, "dump spll: unable to create map to spll\n");
		return 0;
	}
	/* Check magic */
	if (spll->magic != SPLL_MAGIC) {
		/* Wrong magic */
		fprintf(stderr, "dump spll: unknown magic %x (known is %x)\n",
			spll->magic, SPLL_MAGIC);
	}

	dump_many_fields(spll, spll_stats_info, ARRAY_SIZE(spll_stats_info));

	return 0; /* this is complete */
}


/* map for fields of wrs_hist_run_nand (hist.h) */
#undef DUMP_STRUCT
#define DUMP_STRUCT struct wrs_hist_run_nand
struct dump_info wrs_hist_run_nand_info [] = {
	DUMP_FIELD(uint16_t, magic),
	DUMP_FIELD(uint8_t, ver),
	DUMP_FIELD(uint8_t, crc),
	DUMP_FIELD(asciiuptime, lifetime),
	DUMP_FIELD(asciitime, timestamp),
	DUMP_FIELD(uint8_t, temp[0]),
	DUMP_FIELD(uint8_t, temp[1]),
	DUMP_FIELD(uint8_t, temp[2]),
	DUMP_FIELD(uint8_t, temp[3]),
};

/* map for fields of hist_shmem_data (hist.h) */
#undef DUMP_STRUCT
#define DUMP_STRUCT struct hist_shmem_data
struct dump_info hist_shmem_data_info [] = {
	DUMP_FIELD(hist_temp, temp),
};

/* map for fields of wrs_hist_run_spi (hist.h) */
#undef DUMP_STRUCT
#define DUMP_STRUCT struct wrs_hist_run_spi
struct dump_info wrs_hist_run_spi_info [] = {
	DUMP_FIELD(uint16_t, magic),
	DUMP_FIELD(uint8_t, ver),
	DUMP_FIELD(uint8_t, crc),
	DUMP_FIELD(asciiuptime, lifetime),
	DUMP_FIELD(asciitime, timestamp),
};

/* map for fields of wrs_hist_sfp_nand (hist.h) */
#undef DUMP_STRUCT
#define DUMP_STRUCT struct wrs_hist_sfp_nand
struct dump_info wrs_hist_sfp_nand_info [] = {
	DUMP_FIELD(uint16_t, magic),
	DUMP_FIELD(uint8_t, ver),
	DUMP_FIELD(uint8_t, crc),
	DUMP_FIELD(asciiuptime, saved_swlifetime),
	DUMP_FIELD(asciitime, saved_timestamp),
	DUMP_FIELD(uint16_t, end_magic),
	DUMP_FIELD(uint8_t, end_ver),
	DUMP_FIELD(uint8_t, end_crc),
};

/* map for fields of wrs_hist_sfp_entry (hist.h) */
#undef DUMP_STRUCT
#define DUMP_STRUCT struct wrs_hist_sfp_entry
struct dump_info wrs_hist_sfp_entry_info [] = {
	DUMP_FIELD_SIZE(char, vn, 16),
	DUMP_FIELD_SIZE(char, pn, 16),
	DUMP_FIELD_SIZE(char, sn, 16),
	DUMP_FIELD(asciiuptime, sfp_lifetime),
	DUMP_FIELD(hist_sfp_present, sfp_lifetime),
	DUMP_FIELD(asciiuptime, lastseen_swlifetime),
	DUMP_FIELD(asciitime, lastseen_timestamp),
};


int dump_hist_mem(struct wrs_shm_head *head)
{
	struct hist_shmem_data *h;
	struct wrs_hist_sfp_entry *sfp_entry;
	int i;

	if (head->version != HIST_SHMEM_VERSION) {
		fprintf(stderr, "dump hal: unknown version %i (known is %i)\n",
			head->version, HIST_SHMEM_VERSION);
		return -1;
	}
	h = (void *)head + head->data_off;

	printf("hist run nand:\n");
	dump_many_fields(&h->hist_run_nand, wrs_hist_run_nand_info,
			 ARRAY_SIZE(wrs_hist_run_nand_info));
	dump_many_fields(h, hist_shmem_data_info,
			 ARRAY_SIZE(hist_shmem_data_info));
	printf("hist run spi:\n");
	dump_many_fields(&h->hist_run_spi, wrs_hist_run_spi_info,
			 ARRAY_SIZE(wrs_hist_run_spi_info));
	printf("hist sfp nand:\n");
	dump_many_fields(&h->hist_sfp_nand, wrs_hist_sfp_nand_info,
			 ARRAY_SIZE(wrs_hist_sfp_nand_info));

	sfp_entry = &h->hist_sfp_nand.sfps[0];
	for (i = 0; i < WRS_HIST_MAX_SFPS; i++) {
		if (!dump_all_entries
		    && sfp_entry[i].vn[0] == '\0'
		    && sfp_entry[i].pn[0] == '\0'
		    && sfp_entry[i].sn[0] == '\0'
		) {
			/* skip empty entries */
			continue;
		}
		printf("dump sfp %i:\n", i);
		dump_many_fields(&sfp_entry[i],
				 wrs_hist_sfp_entry_info,
				 ARRAY_SIZE(wrs_hist_sfp_entry_info));
	}
	return 0;
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
	[wrs_shm_hist] = dump_hist_mem,
};

void print_info(char *prgname)
{
	printf("usage: %s [parameters]\n", prgname);
	printf("             Dump shmem\n");
	printf("   -a        Dump all entries. By default only valid entries are printed.\n"
	       "             Note: there are 2048 htab and 4096 vlan entries in RTU!\n"
	       "             Note: there are SFP %d entries in wrs_hist!\n",
	       WRS_HIST_MAX_SFPS);
	printf( "   -H <dir>  Open shmem dumps from the given directory\n"
		"   -h        Show this message\n"
		"  Dump shmem for specific program (by default dump for all)\n"
		"   -P        Dump ptp entries\n"
		"   -R        Dump rtu entries\n"
		"   -L        Dump hal entries\n"
		"   -S        Dump SoftPll entries\n"
		"   -U        Dump wrs_hist entries\n");

}

static int dump_print[WRS_SHM_N_NAMES];
static int dump_spll = 0;

int main(int argc, char **argv)
{
	struct wrs_shm_head *head;
	dump_f *f;
	int i;
	int c;
	int print_all = 1;
	struct spll_stats *spll_stats_p;

	while ((c = getopt(argc, argv, "ahH:PRLSU")) != -1) {
		switch (c) {
		case 'a':
			dump_all_entries = 1;
			break;
		case 'H':
			wrs_shm_set_path(optarg);
			break;
		case 'P':
			dump_print[wrs_shm_ptp] = 1;
			print_all = 0;
			break;
		case 'R':
			dump_print[wrs_shm_rtu] = 1;
			print_all = 0;
			break;
		case 'L':
			dump_print[wrs_shm_hal] = 1;
			print_all = 0;
			break;
		case 'S':
			dump_spll = 1;
			print_all = 0;
			break;
		case 'U':
			dump_print[wrs_shm_hist] = 1;
			print_all = 0;
			break;
		case 'h':
		default:
			print_info(argv[0]);
			exit(1);
		}
	}
	for (i = 0; i < WRS_SHM_N_NAMES; i++) {
		if (!print_all && !dump_print[i])
			continue;

		head = wrs_shm_get(i, "reader", 0);
		if (!head) {
			fprintf(stderr, "%s: can't attach memory area %i: %s\n",
				argv[0], i, strerror(errno));
			continue;
		}
		if (!head->pidsequence) {
			printf("ID %i (\"%s\"): no data\n",
			       i, name_id_to_name[i]);
			wrs_shm_put(head);
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
		wrs_shm_put(head);
		printf("\n"); /* separate one area from the next */
	}
	if (print_all || dump_spll) {
		spll_stats_p = create_map(FPGA_SPLL_STAT,
					  sizeof(*spll_stats_p));
		dump_spll_mem(spll_stats_p);
	}
	return 0;
}
