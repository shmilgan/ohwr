#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <libwr/switch_hw.h>
#include "fpga_io.h"
#include "regs/gen10mhz-regs.h"
#include "regs/ppsg-regs.h"

#define OPT_HELP 'h'
#define OPT_FREQ 2
#define OPT_DUTY 3
#define OPT_CSHIFT 4
#define OPT_SIGDEL 5
#define OPT_PPSHIFT 6

/* default parameters to generate 10MHz signal */
#define DEF_FREQ 	10
#define DEF_DUTY 	0.5
#define DEF_CSHIFT 	30
#define DEF_SIGDEL 	0
#define DEF_PPSHIFT 	0

#define MAX_FREQ 250	/* min half-period is 2ns */
#define MIN_FREQ 0.004	/* max half-period is 65535*2ns */
#define CNT_RES	 2

#define gen10_write(reg, val) \
	_fpga_writel(FPGA_BASE_GEN_10MHZ + offsetof(struct GEN10_WB, reg), val)

#define gen10_read(reg) \
	_fpga_readl(FPGA_BASE_GEN_10MHZ + offsetof(struct GEN10_WB, reg))

/* runtime options */
struct option ropts[] = {
	{"help", 0, NULL, OPT_HELP},
	{"freq", 1, NULL, OPT_FREQ},
	{"duty", 1, NULL, OPT_DUTY},
	{"cshift", 1, NULL, OPT_CSHIFT},
	{"sigdel", 1, NULL, OPT_SIGDEL},
	{"ppshift", 1, NULL, OPT_PPSHIFT},
	{0,}};
/*******************/

int print_help(char *prgname)
{
	fprintf(stderr, "Use: %s [--freq <MHz>] [--duty <frac>] [--cshift <ns>]"
			" [--ppshift <taps>] [--sigdel <taps>]\n", prgname);

	return 0;
}

int apply_settings(float freq_mhz, float duty, int cshift_ns, int sigdel_taps,
		int ppshift_taps)
{
	int period_ns;
	int h_width, l_width;

	/*first check if values are in range*/
	if( freq_mhz > MAX_FREQ || freq_mhz < MIN_FREQ ) {
		fprintf(stderr, "Frequency outside range <%f; %d>\n", MIN_FREQ,
				MAX_FREQ);
		return 1;
	}
	if( !(duty > 0 && duty < 1) ) {
		fprintf(stderr, "Duty %f outside range (0; 1)\n", duty);
		return 1;
	}

	/* calculate high and low width from frequency and duty */
	period_ns = 1000 / freq_mhz;
	h_width = period_ns/CNT_RES * duty;
	l_width = period_ns/CNT_RES - h_width;

	/* now check the coarse shift */
	if( cshift_ns > period_ns || cshift_ns < 0 ) {
		fprintf(stderr, "Coarse shift outside range <0; %d>\n",
				period_ns);
		return 1;
	}

	gen10_write(PR, h_width);
	gen10_write(DCR, l_width);
	gen10_write(CSR, cshift_ns/CNT_RES);
	gen10_write(IOR, sigdel_taps);
	gen10_write(PPS_IOR, ppshift_taps);
	sleep(1);
	/* now read the actual delay (in taps) from IODelays */
	sigdel_taps = gen10_read(IOR);
	sigdel_taps >>= GEN10_IOR_TAP_CUR_SHIFT;
	ppshift_taps = gen10_read(PPS_IOR);
	ppshift_taps >>= GEN10_PPS_IOR_TAP_CUR_SHIFT;

	printf("Calculated settings:\n");
	printf("period: %d ns (%d MHz)\n", period_ns, 1000/period_ns);
	printf("high: %d ns; low: %d ns\n", h_width, l_width);
	printf("duty: %f\n", (float)h_width*CNT_RES/period_ns);
	printf("coarse shift: %d\n", (cshift_ns/CNT_RES)*CNT_RES);
	printf("PPS shift: %d taps\n", ppshift_taps);
	printf("Signal delay: %d taps\n", sigdel_taps);

	return 0;
}

int main(int argc, char *argv[])
{
	char *prgname = argv[0];
	float freq_mhz = DEF_FREQ;
	float duty     = DEF_DUTY;
	int cshift_ns  = DEF_CSHIFT;
	int sigdel_taps  = DEF_SIGDEL;
	int ppshift_taps = DEF_PPSHIFT;
	int c;

	if (shw_fpga_mmap_init() < 0) {
		fprintf(stderr, "%s: Can't access device memory\n", prgname);
		exit(1);
	}

	while( (c = getopt_long(argc, argv, "h", ropts, NULL)) != -1) {
		switch(c) {
			case OPT_FREQ:
				freq_mhz = (float) atof(optarg);
				break;
			case OPT_DUTY:
				duty = (float) atof(optarg);
				break;
			case OPT_CSHIFT:
				cshift_ns = atoi(optarg);
				break;
			case OPT_SIGDEL:
				sigdel_taps = atoi(optarg);
				break;
			case OPT_PPSHIFT:
				ppshift_taps = atoi(optarg);
				break;
			case OPT_HELP:
			default:
				print_help(prgname);
				return 1;
		}
	}

	return apply_settings(freq_mhz, duty, cshift_ns, sigdel_taps,
			      ppshift_taps);
}
