#ifndef WRS_SNMP_H
#define WRS_SNMP_H

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "libwr/util.h"

/* Crap! -- everybody makes them different, and even ppsi::ieee wants them */
#undef FALSE
#undef TRUE

/* conflict between definition in net-snmp-agent-includes.h (which include
 * snmp_vars.h) and ppsi.h where INST is defined as a inline function */
#undef INST
#include <ppsi/ieee1588_types.h> /* for ClockIdentity */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>



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
