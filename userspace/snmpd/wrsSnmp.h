#ifndef WRS_SNMP_H
#define WRS_SNMP_H

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

/* Crap! -- everybody makes them different, and even ppsi::ieee wants them */
#undef FALSE
#undef TRUE

/* conflict between definition in net-snmp-agent-includes.h (which include
 * snmp_vars.h) and ppsi.h where INST is defined as a inline function */
#undef INST
#include <ppsi/ieee1588_types.h> /* for ClockIdentity */

#include <libwr/shmem.h>
#include <libwr/hal_shmem.h>

extern struct wrs_shm_head *hal_head;
extern struct hal_shmem_header *hal_shmem;
extern struct hal_port_state *hal_ports;
extern int hal_nports_local;

/*
 * local hack: besides the file pointer, that is there anyways,
 * everything else  is not actually built if WRS_WITH_SNMP_HACKISH_LOG
 * is set 0 at build time (currently the default)
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
extern FILE *wrs_logf;

static inline int logmsg(const char *fmt, ...)
{
	va_list args;
	int ret;

	if (WRS_WITH_SNMP_HACKISH_LOG) {
		if (!wrs_logf) {
			char *fname = getenv("WRS_SNMP_LOGFILE");
			if (!fname)
				fname = "/dev/console";
			wrs_logf = fopen(fname, "w");
		}
		if (!wrs_logf)
			return 0;

		va_start(args, fmt);
		ret = vfprintf(wrs_logf, fmt, args);
		va_end(args);

		return ret;
	} else {
		return 0;
	}
}

static inline int dumpstruct(FILE *dest, char *name, void *ptr, int size)
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



/*  be safe, in case some other header had them slightly differntly */
#undef container_of
#undef offsetof
#undef ARRAY_SIZE

#define container_of(ptr, type, member) ({                      \
	const typeof( ((type *)0)->member ) *__mptr = (ptr); \
	(type *)( (char *)__mptr - offsetof(type, member) );})

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))


#define WRS_N_PORTS  18

/* Scalar is just a stupid thing, but let's keep it */
extern void init_wrsScalar(void);

/* Real stuff follows */
extern void init_wrsPstats(void);
extern void init_wrsPpsi(void);
extern void init_wrsVersion(void);
extern void init_wrsDate(void);

#define WRS_OID 1, 3, 6, 1, 4, 1, 96, 100

struct pickinfo {
	/* Following fields are used to format the output */
	int type; int offset; int len;
};

#define FIELD(_struct, _type, _field) {			\
	.type = _type,					\
	.offset = offsetof(struct _struct, _field),	\
	.len = sizeof(((struct _struct *)0)->_field),			\
	 }

#endif /* WRS_SNMP_H */
