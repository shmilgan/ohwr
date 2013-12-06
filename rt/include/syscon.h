#ifndef __SYSCON_H
#define __SYSCON_H

#include <stdint.h>

#define TICS_PER_SECOND 100000

uint32_t timer_get_tics();
void timer_delay(uint32_t how_long);
int timer_expired(uint32_t t_start, uint32_t how_long);

#endif
