#ifndef __TIMEOUT_H
#define __TIMEOUT_H

#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include "libwr/util.h"

typedef struct {
	int repeat;
	uint64_t start_tics;
	uint64_t timeout;
} timeout_t;

static inline int tmo_init(timeout_t * tmo, uint32_t milliseconds, int repeat)
{
	tmo->repeat = repeat;
	tmo->start_tics = get_monotonic_us();
	tmo->timeout = (uint64_t) milliseconds *1000ULL;
	return 0;
}

static inline int tmo_restart(timeout_t * tmo)
{
	tmo->start_tics = get_monotonic_us();
	return 0;
}

static inline int tmo_expired(timeout_t * tmo)
{
	int expired = (get_monotonic_us() - tmo->start_tics > tmo->timeout);

	if (tmo->repeat && expired)
		tmo->start_tics = get_monotonic_us();

	return expired;
}

#endif
