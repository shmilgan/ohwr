#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#include <libwr/util.h>

static int loops_per_msec = -1;

/* Calculate how many loops per millisecond we make in this CPU */
void shw_udelay_init(void)
{
	volatile int i;
	int j, cur, min = 0;
	struct timeval tv1, tv2;
	for (j = 0; j < 10; j++) {
		gettimeofday(&tv1, NULL);
		for (i = 0; i < 100*1000; i++)
			;
		gettimeofday(&tv2, NULL);
		cur = (tv2.tv_sec - tv1.tv_sec) * 1000 * 1000
			+ tv2.tv_usec - tv1.tv_usec;
		/* keep minimum time, assuming we were scheduled-off less */
		if (!min || cur < min)
			min = cur;
	}
	loops_per_msec = i * 1000 / min;

	if (0)
		printf("loops per msec %i\n", loops_per_msec);
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
