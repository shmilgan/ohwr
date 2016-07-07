#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <libwr/wrs-msg.h>
#include <libwr/util.h>

static int loops_per_msec = -1;

/* Calculate how many loops per millisecond we make in this CPU */
void shw_udelay_init(void)
{
	volatile int i;
	int j, cur, min = 0;
	uint64_t tv1, tv2;
	for (j = 0; j < 10; j++) {
		tv1 = get_monotonic_tics();
		for (i = 0; i < 100*1000; i++)
			;
		tv2 = get_monotonic_tics();
		cur = tv2 - tv1;
		/* keep minimum time, assuming we were scheduled-off less */
		if (!min || cur < min)
			min = cur;
	}
	loops_per_msec = i * 1000 / min;

	if (0)
		pr_debug("loops per msec %i\n", loops_per_msec);
	/*
	 * I get 39400 more or less; it makes sense at 197 bogomips.
	 * The loop is 6 instructions with 3 (cached) memory accesses
	 * and 1 jump.  197/39.4 = 5.0 .
	 */
}
/*
 * This function is needed to for slow delays to overcome the jiffy-grained
 * delays that the kernel offers. We can't wait for 1ms when needing 4us.
 */
void shw_udelay(uint32_t microseconds)
{
	volatile int i;

	if (loops_per_msec < 0)
		shw_udelay_init();

	if (microseconds > 1000) {
		usleep(microseconds);
		return;
	}
	for (i = 0; i < loops_per_msec * microseconds / 1000; i++)
		;
}

/* get monotonic number of useconds */
uint64_t get_monotonic_tics(void)
{
	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv);
 
	return (uint64_t) tv.tv_sec * 1000000ULL +
			(uint64_t) (tv.tv_nsec / 1000);
}

/* get monotonic number of seconds */
time_t get_monotonic_sec(void)
{
	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv);
	return tv.tv_sec;
}
