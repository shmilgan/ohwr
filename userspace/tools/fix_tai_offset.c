/* This is just a subset of wr_date, user to test on the host */
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/timex.h>

#ifndef MOD_TAI
#define MOD_TAI 0x80
#endif
#define WRDATE_CFG_FILE "/wr/etc/wrdate.conf"
#define WRDATE_LEAP_FILE "/etc/leap-seconds.list"


int opt_verbose = 1;
char *prgname;

int main(int argc, char **argv)
{
	struct timex t;
	char s[128];
	unsigned long long now, leapt, expire = 0;
	int i, tai_offset = 0;

	prgname = argv[0];

	/* first: get the current offset */
	memset(&t, 0, sizeof(t));
	if (adjtimex(&t) < 0) {
		fprintf(stderr, "%s: adjtimex(): %s\n", prgname,
			strerror(errno));
		return 0;
	}

	/* then, find the current time, using such offset */
	now = time(NULL) +  2208988800LL; /* (for TAI: + utc_offset */

	FILE *f = fopen(WRDATE_LEAP_FILE, "r");
	if (!f) {
		fprintf(stderr, "%s: %s: %s\n", prgname, WRDATE_LEAP_FILE,
			strerror(errno));
		return 0;
	}
	while (fgets(s, sizeof(s), f)) {
		if (sscanf(s, "#@ %lli", &expire) == 1)
			continue;
		if (sscanf(s, "%lli %i", &leapt, &i) != 2)
			continue;
		/* check this line, and apply if if it's in the past */
		if (leapt < now)
			tai_offset = i;
	}
	fclose(f);

	if (tai_offset != t.tai) {
		if (opt_verbose)
			printf("Previous TAI offset: %i\n", t.tai);
		t.constant = tai_offset;
		t.modes = MOD_TAI;
		if (adjtimex(&t) < 0) {
			fprintf(stderr, "%s: adjtimex(): %s\n", prgname,
				strerror(errno));
			return tai_offset;
		}
	}
	if (opt_verbose)
		printf("Current TAI offset: %i\n", t.tai);
	return tai_offset;
}
