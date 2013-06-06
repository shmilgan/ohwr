/*
 * Trivial tool to set WR date in the switch from local or NTP time
 *
 * Alessandro Rubini, 2011, for CERN, 2013. GPL2 or later
 */
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
#include "../../kernel/wbgen-regs/ppsg-regs.h"

#define WRDATE_CFG_FILE "/wr/etc/wrdate.conf"

/* Address for hardware, from nic-hardware.h */
#define FPGA_BASE_PPSG  0x10010500

void help(char *prgname)
{
	fprintf(stderr, "%s: Use: \"%s [<options>] <cmd> [<args>]\n",
		prgname, prgname);
	fprintf(stderr,
		"  The program usss %s as default config file name\n"
		"  -f       force: run even if not on a WR switch\n"
		"  -c <cfg> configfile to use in place of the default\n"
		"  -v       verbose: report what the program does\n"
		"  -n       do not act in practice\n"
		"    get             print WR time to stdout\n"
		"    set <value>     set WR time to scalar seconds\n"
		"    set host        set from current host time\n"
		"    set ntp         set from ntp (sets host time too)\n"
		"    set ntp:<ip>    set from specified ntp server\n",
		WRDATE_CFG_FILE);
	exit(1);
}

int opt_verbose, opt_force, opt_not;
char *opt_cfgfile = WRDATE_CFG_FILE;
char *prgname;

/* Check that we actualy are on the wr switch, exit if not */
int wrdate_check_host(int fdmem)
{
	int ret;

	/*
	 * Check the specific CPU: a different switch will require a
	 * different memory address so this must be match.
	 * system(3) is bad, but it's fast to code
	 */
	ret = system("grep -q ARM926EJ-S /proc/cpuinfo");
	if (opt_force && ret) {
		fprintf(stderr, "%s: not running on the WR switch\n", prgname);
		if (!opt_not)
			exit(1);
	}
	return ret ? -1 : 0;
}

int wrdate_cfgfile(char *fname)
{
	/* FIXME: parse config file */
	return 0;
}

/* create a map, that is never goind to be released */
void *create_map(int fdmem, unsigned long address, unsigned long size)
{
	unsigned long ps = getpagesize();
	unsigned long offset, fragment, len;
	void *mapaddr;

	offset = address & ~(ps -1);
	fragment = address & (ps -1);
	len = address + size - offset;

	mapaddr = mmap(0, len, PROT_READ | PROT_WRITE,
		       MAP_SHARED, fdmem, offset);
	if (mapaddr == MAP_FAILED) {
		fprintf(stderr, "%s: mmap: %s\n", prgname, strerror(errno));
		exit(1);
	}
	return mapaddr + fragment;
}

int wrdate_get(struct PPSG_WB *pps)
{
	unsigned long utch, utcl, nsec, tmp1, tmp2;
	uint64_t utc;
	time_t t;
	struct timeval tv;
	struct tm tm;
	char s[64];

	if (opt_not) {
		gettimeofday(&tv, NULL);
		utch = 0;
		utcl = tv.tv_sec;
		nsec = tv.tv_usec * 1000;
	} else {
		utch = pps->CNTR_UTCHI;

		do {
			utcl = pps->CNTR_UTCLO;
			nsec = pps->CNTR_NSEC * 16; /* we count a 16.5MHz */
			tmp1 = pps->CNTR_UTCHI;
			tmp2 = pps->CNTR_UTCLO;
		} while((tmp1 != utch) || (tmp2 != utcl));
	}

	utc = (uint64_t)(utch) << 32 | utcl;
	t = utc;
	localtime_r(&t, &tm);
	strftime(s, sizeof(s), "%Y-%m-%d %H:%M:%S", &tm);
	printf("%lli.%09li\n%s.%09li\n", utc, nsec, s, nsec);
	return 0;
}

/* This returns wr time, used for syncing to a second transition */
void gettimeof_wr(struct timeval *tv, struct PPSG_WB *pps)
{
	unsigned long utcl, nsec, tmp2;

	/* FIXME: not 2038-clean */
	do {
		utcl = pps->CNTR_UTCLO;
		nsec = pps->CNTR_NSEC * 16; /* we count a 16.5MHz */
		tmp2 = pps->CNTR_UTCLO;
	} while(tmp2 != utcl);
	tv->tv_sec = utcl;
	tv->tv_usec = nsec / 1000;
}

/* This sets WR time from host time */
int wrdate_internal_set(struct PPSG_WB *pps)
{
	struct timeval tvh, tvr; /* host, rabbit */
	signed long diff;
	unsigned long prev;

	gettimeofday(&tvh, NULL);
	gettimeof_wr(&tvr, pps);

	/* Warn if more than 200ms away */
	diff = tvh.tv_usec - tvr.tv_usec;
	if (diff > 500 * 1000)
		diff -= 1000 * 1000;
	if (diff < -500 * 1000)
		diff += 1000 * 1000;

	if (diff > 200 * 1000 || diff < -200 * 1000)
		fprintf(stderr, "%s: Warning: fractional second differs by"
			"more than 0.2 (%li ms)\n", prgname, diff / 1000);

	if (opt_verbose) {
		printf("Host time: %9li.%06li\n", (long)(tvh.tv_sec),
		       (long)(tvh.tv_usec));
		printf("WR   time: %9li.%06li\n", (long)(tvr.tv_sec),
		       (long)(tvr.tv_usec));
		printf("Fractional difference: %li usec\n", diff);
	}

	if (0) { /* Older implementation, wrong and needless */

		/* Wait for the wr time to pass the second. */
		prev = tvr.tv_sec;
		do
			gettimeof_wr(&tvr, pps);
		while (tvr.tv_sec == prev);

		/* If we are near a second change in the host, wait for it */
		gettimeofday(&tvh, NULL);
		if (tvh.tv_usec > 500 * 1000) {
			prev = tvh.tv_sec;
			do
				gettimeofday(&tvh, NULL);
			while (tvh.tv_sec == prev);
		}

		/* Finally, write the host time to White Rabbit */

		/* FIXME: not 2038-clean */
		pps->CNTR_UTCLO = tvh.tv_sec; // no effect: procedure is wrong

	} else { /* good implementation */

		signed long long diff64;

		/* diff is the expected step to be added, so host - WR */
		diff = tvh.tv_usec - tvr.tv_usec;
		diff64 = tvh.tv_sec - tvr.tv_sec;
		if (diff > 500 * 1000)
			diff64++;
		if (diff < -500 * 1000)
			diff64--;
		if (opt_verbose)
			printf("adjusting by %lli seconds\n", diff64);
		/* We must write a signed "adjustment" value to registers */
		pps->ADJ_UTCLO = diff64 & 0xffffffff;
		pps->ADJ_UTCHI = (diff64 >> 32) & 0xff;
		pps->ADJ_NSEC = 0;
		asm("" : : : "memory"); /* barrier... */
		pps->CR = pps->CR | PPSG_CR_CNT_ADJ;
	}

	return 0;
}

/* Frontend to the set mechanism: parse the argument */
int wrdate_set(struct PPSG_WB *pps, char *arg)
{
	char *s;
	unsigned long t; /* WARNING: 64 bit */
	struct timeval tv;


	if (!strcmp(arg, "host"))
		return wrdate_internal_set(pps);

	s = strdup(arg);
	if (sscanf(arg, "%li%s", &t, s) == 1) {
		tv.tv_sec = t;
		tv.tv_usec = 0;
		if (settimeofday(&tv, NULL) < 0) {
			fprintf(stderr, "%s: settimeofday(%s): %s\n",
				prgname, arg, strerror(errno));
			exit(1);
		}
		return wrdate_internal_set(pps);
	}

	/* FIXME: other time formats */
	printf(" FIXME\n");
	return 0;
}

int main(int argc, char **argv)
{
	int c, fd;
	char *cmd;
	struct PPSG_WB *pps;

	prgname = argv[0];

	while ( (c = getopt(argc, argv, "fc:vn")) != -1) {
		switch(c) {
		case 'f':
			opt_force = 1;
			break;
		case 'c':
			opt_cfgfile = optarg;
			break;
		case 'v':
			opt_verbose = 1;
			break;
		case 'n':
			opt_not = 1;
			break;
		default:
			help(argv[0]);
		}
	}
	if (optind > argc - 1)
		help(argv[0]);

	cmd = argv[optind++];


	fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (fd < 0) {
		fprintf(stderr, "%s: /dev/mem: %s\n", argv[0], strerror(errno));
		exit(1);
	}
	wrdate_check_host(fd);
	pps = create_map(fd, FPGA_BASE_PPSG, sizeof(*pps));
	close(fd);

	wrdate_cfgfile(opt_cfgfile);

	if (!strcmp(cmd, "get")) {
		if (optind < argc)
			help(argv[0]);
		return wrdate_get(pps);
	}

	/* only other command is "set", with one argument */
	if (strcmp(cmd, "set") || optind != argc - 1)
		help(argv[0]);

	return wrdate_set(pps, argv[optind]);
}
