#ifndef __HW_UTIL_H
#define __HW_UTIL_H

#include <stdio.h>
#include <inttypes.h>

#define atoidef(str,def) (str)?atoi(str):def

void shw_udelay(uint32_t microseconds);
void *shw_malloc(size_t nbytes);
void shw_free(void *ptr);
const char *shw_2binary(uint8_t x);
uint64_t shw_get_tics();

#endif
