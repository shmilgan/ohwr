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

/* data structures */
struct params {
	float freq_mhz;
	int period_ns;

	float duty;
	int h_width, l_width;

	int cshift_ns;
	int sigdel_taps;
	int ppshift_taps;
};


/*******************/

int print_help(char *prgname)
{
	fprintf(stderr, "Use: %s [--freq <MHz>] [--duty <frac>] [--cshift <ns>]"
			" [--ppshift <taps>] [--sigdel <taps>]\n", prgname);

	return 0;
}

int calc_settings(struct params *req, struct params *calc)
{
	calc->freq_mhz = req->freq_mhz;
	if (req->freq_mhz > MAX_FREQ)
		calc->freq_mhz = MAX_FREQ;
	if (req->freq_mhz < MIN_FREQ)
		calc->freq_mhz = MIN_FREQ;

	if (!(req->duty > 0 && req->duty < 1))
		req->duty = 0.5;

	req->period_ns = 1000 / calc->freq_mhz;
	calc->h_width = req->period_ns/CNT_RES * req->duty;
	calc->l_width = req->period_ns/CNT_RES - calc->h_width;

	/* last step, calculate the actual frequency and period based on the
	 * actual h_width and l_width */
	calc->period_ns = (calc->h_width + calc->l_width) * CNT_RES;
	calc->freq_mhz = 1000 / calc->period_ns;
	calc->duty = (float)calc->h_width*CNT_RES/calc->period_ns;

	/* just copy the values that don't change */
	calc->cshift_ns    = req->cshift_ns;
	calc->sigdel_taps  = req->sigdel_taps;
	calc->ppshift_taps = req->ppshift_taps;

	/* check if what's about to be generated matches the request */
	if (req->period_ns == calc->period_ns && req->duty == calc->duty)
		return 0;
	else
		return -1;
}

int apply_settings(struct params *p)
{
	gen10_write(PR, p->h_width);
	gen10_write(DCR, p->l_width);
	gen10_write(CSR, p->cshift_ns/CNT_RES);
	gen10_write(IOR, p->sigdel_taps);
	gen10_write(PPS_IOR, p->ppshift_taps);
	sleep(1);
	/* now read the actual delay (in taps) from IODelays */
	p->sigdel_taps = gen10_read(IOR);
	p->sigdel_taps >>= GEN10_IOR_TAP_CUR_SHIFT;
	p->ppshift_taps = gen10_read(PPS_IOR);
	p->ppshift_taps >>= GEN10_PPS_IOR_TAP_CUR_SHIFT;

	return 0;
}

int print_settings(FILE *f, struct params *p)
{
	fprintf(f, "frequency: %.3f MHz (%d ns)\n", p->freq_mhz, p->period_ns);
	fprintf(f, "high: %d ns; low: %d ns\n", p->h_width, p->l_width);
	fprintf(f, "duty: %f\n", p->duty);
	fprintf(f, "coarse shift: %d\n", (p->cshift_ns/CNT_RES)*CNT_RES);
	fprintf(f, "PPS shift: %d taps\n", p->ppshift_taps);
	fprintf(f, "Signal delay: %d taps\n", p->sigdel_taps);

	return 0;
}

int main(int argc, char *argv[])
{
	char *prgname = argv[0];
	struct params req = {DEF_FREQ, 0, DEF_DUTY, 0, 0, DEF_CSHIFT,
		DEF_SIGDEL, DEF_PPSHIFT};
	struct params calc;
	int c, ret;

	if (shw_fpga_mmap_init() < 0) {
		fprintf(stderr, "%s: Can't access device memory\n", prgname);
		exit(1);
	}

	while( (c = getopt_long(argc, argv, "h", ropts, NULL)) != -1) {
		switch(c) {
			case OPT_FREQ:
				req.freq_mhz = (float) atof(optarg);
				break;
			case OPT_DUTY:
				req.duty = (float) atof(optarg);
				break;
			case OPT_CSHIFT:
				req.cshift_ns = atoi(optarg);
				break;
			case OPT_SIGDEL:
				req.sigdel_taps = atoi(optarg);
				break;
			case OPT_PPSHIFT:
				req.ppshift_taps = atoi(optarg);
				break;
			case OPT_HELP:
			default:
				print_help(prgname);
				return 1;
		}
	}

	ret = calc_settings(&req, &calc);

	if (!(calc.duty > 0 && calc.duty < 1)) {
		fprintf(stderr, "Requested duty %.2f (calculated %.2f)"
				" outside range (0; 1)\n", req.duty, calc.duty);
		return 1;
	}

	/* now check the coarse shift */
	if (calc.cshift_ns > calc.period_ns || calc.cshift_ns < 0) {
		fprintf(stderr, "Coarse shift outside range <0; %d>\n",
				req.period_ns);
		return 1;
	}

	if (ret != 0) {
		fprintf(stderr, "Could not generate required signal, here is "
				"the alternative you could use:\n");
		print_settings(stderr, &calc);
		return 1;
	}

	apply_settings(&calc);
	printf("Applied settings:\n");
	print_settings(stdout, &calc);

	return 0;
}
