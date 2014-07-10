#ifndef WRS_SNMP_H
#define WRS_SNMP_H


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
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))


#define WRS_N_PORTS  18

/* Scalar is just a stupid thing, but let's keep it */
extern void init_wrsScalar(void);

/* Real stuff follows */
extern void init_wrsPstats(void);
extern void init_wrsPpsi(void);
extern void init_wrsVersion(void);

#define WRS_OID 1, 3, 6, 1, 4, 1, 96, 100

/* Open a file or a pipe according to name[0] (e.g. "|wr_mon", "/tmp/log") */
extern FILE *wrs_fpopen(char *file_or_pipe, char *mode);
extern void wrs_fpclose(FILE *f, char *file_or_pipe);

#endif /* WRS_SNMP_H */
