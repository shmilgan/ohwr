#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <libwr/wrs-msg.h>
#include <libwr/switch_hw.h>
#include <libwr/pps_gen.h>

#define PPS_ON 1
#define PPS_OFF 0

void help(char *prgname)
{
	fprintf(stderr, "%s: Use: \"%s [<options>] <cmd>\n",
		prgname, prgname);
	fprintf(stderr,
		"  The program has the following options\n"
		"  -h   print help\n"
		"\n"
		"  Commands are:\n"
		"      on  - switch PPS output on.\n"
		"      off - switch PPS output off.\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, "h")) != -1) {
		switch (opt) {
		case 'h':
		default:
			help(argv[0]);
		}
	}

	if (argc > 1) {
		if (strcmp(argv[1], "on") == 0) {
			assert_init(shw_fpga_mmap_init());
			shw_pps_gen_enable_output(PPS_ON);
			exit(0);
		} else if (strcmp(argv[1], "off") == 0) {
			assert_init(shw_fpga_mmap_init());
			shw_pps_gen_enable_output(PPS_OFF);
			exit(0);
		} else {
			printf("Unknown command\n;");
			exit(1);
		}
	} else {
		printf("No command given\n");
		exit(1);
	}
}
