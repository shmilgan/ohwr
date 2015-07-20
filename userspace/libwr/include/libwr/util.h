#ifndef __LIBWR_HW_UTIL_H
#define __LIBWR_HW_UTIL_H

#include <stdio.h>
#include <inttypes.h>

#define atoidef(str,def) (str)?atoi(str):def

void shw_udelay_init(void);
void shw_udelay(uint32_t microseconds);
uint64_t shw_get_tics(void);

#endif /* __LIBWR_HW_UTIL_H */
