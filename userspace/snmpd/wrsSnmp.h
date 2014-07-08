#ifndef WRS_SNMP_H
#define WRS_SNMP_H

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

#endif /* WRS_SNMP_H */
