#ifndef __HW_UTIL_H
#define __HW_UTIL_H

#include <stdio.h>
#include <inttypes.h>


void shw_udelay(uint32_t microseconds);
void *shw_malloc(size_t nbytes);
void shw_free(void *ptr);

#endif
