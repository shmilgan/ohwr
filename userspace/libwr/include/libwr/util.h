#ifndef __LIBWR_HW_UTIL_H
#define __LIBWR_HW_UTIL_H

#include <stdio.h>
#include <inttypes.h>
#include <time.h>

#define atoidef(str,def) (str)?atoi(str):def

void shw_udelay_init(void);
void shw_udelay(uint32_t microseconds);
/* get monotonic number of useconds */
uint64_t get_monotonic_tics(void);
/* get monotonic number of seconds */
time_t get_monotonic_sec(void);

/* Change endianess of the string, for example when accessing strings in
 * the SoftPLL */
void strncpy_e(char *d, char *s, int len);

#endif /* __LIBWR_HW_UTIL_H */
