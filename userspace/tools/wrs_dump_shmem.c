#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <libwr/shmem.h>
#include <libwr/hal_shmem.h>

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
 * A structure to dump fields. This is meant to simplify things, see use here
 */
struct dump_info {
	char *name;
	int type;   /* see below */
	int offset;
	int size;  /* only for strings or binary strings */
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
	dump_type_int,
	dump_type_double,
};

void dump_one_field(void *addr, struct dump_info *info)
{
	void *p = addr + info->offset;
	int i;

	printf("  %s: ", info->name);
	switch(info->type) {
	case dump_type_char:
		printf("\"%s\"\n", (char *)p);
		break;
	case dump_type_bina:
		for (i = 0; i < info->size; i++)
			printf("%02x%c", ((unsigned char *)p)[i],
			       i == info->size - 1 ? '\n' : ':');
		break;
	case dump_type_int:
		printf("%i\n", *(int *)p);
		break;
	case dump_type_uint32_t:
		printf("0x%08lx\n", (long)*(uint32_t *)p);
		break;
	case dump_type_double:
		printf("%lf\n", *(double *)p);
		break;
	}
}
void dump_many_fields(void *addr, struct dump_info *info, int ninfo)
{
	int i;

	for (i = 0; i < ninfo; i++)
		dump_one_field(addr, info + i);
}

/* the macro below relies on an externally-defined structure type */
#define DUMP_FIELD(_type, _fname) { \
	.name = #_fname, \
	.type = dump_type_ ## _type, \
	.offset = offsetof(DUMP_STRUCT, _fname), \
}
#define DUMP_FIELD_SIZE(_type, _fname, _size) { \
	.name = #_fname, \
	.type = dump_type_ ## _type, \
	.offset = offsetof(DUMP_STRUCT, _fname), \
	.size = _size, \
}

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
	DUMP_FIELD(int, mode),
	DUMP_FIELD(int, state),
	DUMP_FIELD(int, index),
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

	/* Another internal structure, with a final pointer, so int32_t */
	DUMP_FIELD(int,       calib.sfp.flags),
	DUMP_FIELD_SIZE(char, calib.sfp.part_num, 16),
	DUMP_FIELD_SIZE(char, calib.sfp.vendor_serial, 16),
	DUMP_FIELD(double,    calib.sfp.alpha),
	DUMP_FIELD(uint32_t,  calib.sfp.delta_tx),
	DUMP_FIELD(uint32_t,  calib.sfp.delta_rx),
	DUMP_FIELD(uint32_t,  calib.sfp.next),

	DUMP_FIELD(uint32_t, phase_val),
	DUMP_FIELD(int, phase_val_valid),
	DUMP_FIELD(int, tx_cal_pending),
	DUMP_FIELD(int, rx_cal_pending),
	DUMP_FIELD(int, lock_state),
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
	n = h->nports;
	p = wrs_shm_follow(head, h->ports);

	for (i = 0; i < n; i++, p++) {
		printf("dump port %i\n", i);
		dump_many_fields(p, hal_port_info, ARRAY_SIZE(hal_port_info));
	}
	return 0;
}

int dump_any_mem(struct wrs_shm_head *head)
{
	unsigned char *p = (void *)head;
	int i, j;

	p += head->data_off;
	for (i = 0; i < head->data_size; i++) {
		printf("%02x ", p[i]);
		if (i % 16 == 15) { /* ascii row */
			printf("    ");
			for (j = i & ~15; j <= i; j++)
				printf("%c", p[j] >= 0x20 && p[j] > 0x7f
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
};

int main(int argc, char **argv)
{
	struct wrs_shm_head *head;
	dump_f *f;
	void *m;
	int i;

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
