#ifndef __TIMEOUT_H
#define __TIMEOUT_H

#include <stdint.h>
#include <libwr/util.h>

typedef struct {
	int repeat;
	uint64_t start_tics;
	uint64_t timeout;
} timeout_t;

int libwr_tmo_init(timeout_t *tmo, uint32_t milliseconds, int repeat);
int libwr_tmo_restart(timeout_t *tmo);
int libwr_tmo_expired(timeout_t *tmo);
#endif
