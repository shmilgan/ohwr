/*
 * White Rabbit MRP (Multiple Registration Protocol)
 *
 * Authors:     Juan Luis Manas (juan.manas@integrasys.es)
 *              Miguel Baizan   (miguel.baizan@integrasys.es)
 *
 * Description: Operations to be used over MRP timers.
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __WHITERABBIT_TIMER_H
#define __WHITERABBIT_TIMER_H

#include <stdint.h>
#include <string.h>
#include <time.h>

#ifndef CLOCK_MONOTONIC_RAW
#define CLOCK_MONOTONIC_RAW CLOCK_MONOTONIC
#endif

/* Conversion of ime units */
#ifndef CSEC_PER_SEC
#define CSEC_PER_SEC    100L
#endif
#ifndef NSEC_PER_CSEC
#define NSEC_PER_CSEC   10000000L
#endif
#ifndef NSEC_PER_SEC
#define NSEC_PER_SEC	1000000000L
#endif

/*
 * a < b:  return <0
 * a == b: return 0
 * a > b:  return >0
 */
static inline int timespec_compare(const struct timespec *a, const struct timespec *b)
{
	if (a->tv_sec < b->tv_sec)
		return -1;
	if (a->tv_sec > b->tv_sec)
		return 1;
	return a->tv_nsec - b->tv_nsec;
}

/* Add centiseconds to a timespec
   @param a pointer to timespec to be incremented.
   @param ns number of centiseconds */
static void timespec_add_cs(struct timespec *timestamp, unsigned long cs)
{
    long ns = (cs % CSEC_PER_SEC) * NSEC_PER_CSEC; // remainder (nanoseconds)

    timestamp->tv_sec += (cs / CSEC_PER_SEC);
    if ((timestamp->tv_nsec + ns) >= NSEC_PER_SEC) {
        timestamp->tv_sec++;
        timestamp->tv_nsec += (ns - NSEC_PER_SEC);
    } else
        timestamp->tv_nsec += ns;
}

/* Copies src timer into dst timer.
   @return pointer to dst timer. */
static inline uint8_t* timespec_copy(struct timespec *dst, struct timespec *src)
{
    return memcpy(dst, src, sizeof(struct timespec));
}

/* Get a current time as timespec.
   @param timestamp pointer to the timespec structure to store the timestamp */
static inline struct timespec *timer_now(struct timespec *now)
{
    /* Use CLOCK_MONOTONIC_RAW to get raw hardware-based time that is not
       subject to NTP/PTP adjustments */
    clock_gettime(CLOCK_MONOTONIC_RAW, now);
    return now;
}

/* Calculate an expiration time. This is built taking timestamps from a system
   clock and adding a certain period (which depends on the type of timer).
   @param timeout (out) expiration time
   @param period expiration period (in centiseconds) */
static inline void timer_start(struct timespec *timeout, long period)
{
    timespec_add_cs(timer_now(timeout), period);
}

/* Check whether the timer has expired or not.
   @param timeout Timer timeout
   @return 1 if the timer has expired; 0 otherwise */
int timer_expired(struct timespec *timeout)
{
    struct timespec now;
    return timespec_compare(timer_now(&now), timeout) > 0;
}

/* Pseudo-random period */
static inline int random_val(int min, int max)
{
    return ((rand() % (max - min + 1)) + min);
}

#endif /*__WHITERABBIT_TIMER_H*/
