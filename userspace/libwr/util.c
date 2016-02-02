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

/*
 * Function to print time from the pps_gen.h file borrowed from util.c in wrpc-sw
 */

/* cut from libc sources */

#define         YEAR0   1900
#define         EPOCH_YR   1970
#define         SECS_DAY   (24L * 60L * 60L)
#define         LEAPYEAR(year)   (!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define         YEARSIZE(year)   (LEAPYEAR(year) ? 366 : 365)
#define         FIRSTSUNDAY(timp)   (((timp)->tm_yday - (timp)->tm_wday + 420) % 7)
#define         FIRSTDAYOF(timp)   (((timp)->tm_wday - (timp)->tm_yday + 420) % 7)
#define         TIME_MAX   ULONG_MAX
#define         ABB_LEN   3

static const char *_days[] = {
        "Sun", "Mon", "Tue", "Wed",
        "Thu", "Fri", "Sat"
};

static const char *_months[] = {
        "Jan", "Feb", "Mar",
        "Apr", "May", "Jun",
        "Jul", "Aug", "Sep",
        "Oct", "Nov", "Dec"
};

static const int _ytab[2][12] = {
        {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
        {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};
 
char *format_time(uint64_t sec)
{
        struct tm t;
        static char buf[64];
        unsigned long dayclock, dayno;
        int year = EPOCH_YR;

        dayclock = (unsigned long)sec % SECS_DAY;
        dayno = (unsigned long)sec / SECS_DAY;

        t.tm_sec = dayclock % 60;
        t.tm_min = (dayclock % 3600) / 60;
        t.tm_hour = dayclock / 3600;
        t.tm_wday = (dayno + 4) % 7;    /* day 0 was a thursday */
        while (dayno >= YEARSIZE(year)) {
                dayno -= YEARSIZE(year);
                year++;
        }
        t.tm_year = year - YEAR0;
        t.tm_yday = dayno;
        t.tm_mon = 0;
        while (dayno >= _ytab[LEAPYEAR(year)][t.tm_mon]) {
                dayno -= _ytab[LEAPYEAR(year)][t.tm_mon];
                t.tm_mon++;
        }
        t.tm_mday = dayno + 1;
        t.tm_isdst = 0;

        sprintf(buf, "%s, %s %d, %d, %02d:%02d:%02d", _days[t.tm_wday],
                _months[t.tm_mon], t.tm_mday, t.tm_year + YEAR0, t.tm_hour,
                t.tm_min, t.tm_sec);

        return buf;
}

