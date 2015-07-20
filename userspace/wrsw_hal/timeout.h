#ifndef __TIMEOUT_H
#define __TIMEOUT_H

#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>

typedef struct {
	int repeat;
	uint64_t start_tics;
	uint64_t timeout;
} timeout_t;

static inline uint64_t tm_get_tics(void)
{
	struct timezone tz = { 0, 0 };
	struct timeval tv;
	gettimeofday(&tv, &tz);

	return (uint64_t) tv.tv_sec * 1000000ULL + (uint64_t) tv.tv_usec;
}

static inline int tmo_init(timeout_t * tmo, uint32_t milliseconds, int repeat)
{
	tmo->repeat = repeat;
	tmo->start_tics = tm_get_tics();
	tmo->timeout = (uint64_t) milliseconds *1000ULL;
	return 0;
}

static inline int tmo_restart(timeout_t * tmo)
{
	tmo->start_tics = tm_get_tics();
	return 0;
}

static inline int tmo_expired(timeout_t * tmo)
{
	int expired = (tm_get_tics() - tmo->start_tics > tmo->timeout);

	if (tmo->repeat && expired)
		tmo->start_tics = tm_get_tics();

	return expired;
}

#endif
