#include <stdlib.h>
#include <unistd.h>
#include <libwr/switch_hw.h>
#include <libwr/wrs-msg.h>
#include <libwr/shw_io.h>
/* for shw_sfp_buses_init and shw_sfp_print_header*/
#include "../libwr/i2c_sfp.h"


void print_info(char *prgname)
{
	printf("usage: %s [parameters]\n", prgname);
	printf(""
		"             Dump sfp header info for all ports\n"
		"   -p <num>  Dump sfp header for specific port (1-18)\n"
		"   -x        Dump sfp header also in hex\n"
		"   -h        Show this message\n"
		"   -q        Decrease verbosity\n"
		"   -v        Increase verbosity\n"
		"\n"
		"NOTE: Be carefull! All reads (i2c transfers) are in race with hal!\n"
	);

}

int main(int argc, char **argv)
{
	int c;
	struct shw_sfp_header shdr;
	int err;
	int nports;
	int dump_port;
	int i;
	int dump_hex_header = 0;

	wrs_msg_init(argc, argv);
	nports = 18;
	dump_port = 1;

	while ((c = getopt(argc, argv, "ahqvp:x")) != -1) {
		switch (c) {
		case 'p':
			dump_port = atoi(optarg);
			if (dump_port) {
				nports = dump_port;
			} else {
				printf("Wrong port number!\n");
				print_info(argv[0]);
				exit(1);
			}
			break;
		case 'x':
			dump_hex_header = 1;
			break;
		case 'q': break; /* done in wrs_msg_init() */
		case 'v': break; /* done in wrs_msg_init() */
		case 'h':
		default:
			print_info(argv[0]);
			exit(1);
		}
	}

	/* init i2c, be carefull all i2c transfers are in race with hal! */
	assert_init(shw_io_init());
	assert_init(shw_fpga_mmap_init());
	assert_init(shw_sfp_buses_init());

	for (i = dump_port; i <= nports; i++) {
		printf("========= port %d =========\n", i);
		err = shw_sfp_read_verify_header(i - 1, &shdr);
		if (err == -2) {
			pr_error("SFP module not inserted in port %d. Failed "
				 "to read SFP configuration header\n", i);
		} else if (err < 0) {
			pr_error("Failed to read SFP configuration header on "
				 "port %d\n", i);
		} else {
			shw_sfp_print_header(&shdr);
			if (dump_hex_header) {
				shw_sfp_header_dump(&shdr);
			}
		}
	}
	return 0;
}
