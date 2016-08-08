#include <stdlib.h>
#include <unistd.h>
#include <libwr/switch_hw.h>
#include <libwr/wrs-msg.h>
#include <libwr/shw_io.h>
/* for shw_sfp_buses_init and shw_sfp_print_header*/
#include "../libwr/i2c_sfp.h"

#define SFP_EEPROM_READ 1
#define SFP_EEPROM_WRITE 2
#define HAL_PROCESS_NAME "/wr/bin/wrsw_hal"
#define PROCESS_COMMAND_HAL "/bin/ps axo command"
#define MONIT_PROCESS_NAME "/usr/bin/monit"
#define PROCESS_COMMAND_MONIT "/bin/ps axo stat o command"

void print_info(char *prgname)
{
	printf("usage: %s [parameters]\n", prgname);
	printf(""
		"                   Dump sfp header info for all ports\n"
		"   -p <num>        Dump sfp header for specific port (1-18)\n"
		"   -x              Dump sfp header also in hex\n"
		"   -h              Show this message\n"
		"   -q              Decrease verbosity\n"
		"   -v              Increase verbosity\n"
		"   -V              Print version\n"
		"   -a <READ|WRITE> Read/write SFP's eeprom; before reads/writes disable HAL and monit!\n"
		"   -f <file>       File to READ/WRITE SFP's eeprom\n"
		"\n"
		"NOTE: Be carefull! All reads (i2c transfers) are in race with hal!\n"
	);

}

static int check_monit(void)
{
	FILE *f;
	char command[41]; /* 1 for null char */
	char stat[5]; /* 1 for null char */
	int ret = 0;

	f = popen(PROCESS_COMMAND_MONIT, "r");
	if (!f) {
		pr_error("Error while checking the presence of HAL!\n");
		exit(1);
	}
	while (ret != EOF) {
		/* read first word from line (process name) ignore rest of
		 * the line */
		ret = fscanf(f, "%4s %40s%*[^\n]", stat, command);

		if (ret != 2)
			continue; /* error... or EOF */
		if (!strcmp(MONIT_PROCESS_NAME, command)) {
			if (strcmp(stat, "T")) {
				/* if monit in "T" then not really running */
				pclose(f);
				return 1;
			}
		}
	}
	pclose(f);
	return 0;
}

static int check_hal(void)
{
	FILE *f;
	char key[41]; /* 1 for null char */
	int ret = 0;

	f = popen(PROCESS_COMMAND_HAL, "r");
	if (!f) {
		pr_error("Error while checking the presence of HAL!\n");
		exit(1);
	}
	while (ret != EOF) {
		/* read first word from line (process name) ignore rest of
		 * the line */
		ret = fscanf(f, "%40s%*[^\n]", key);
		if (ret != 1)
			continue; /* error... or EOF */
		if (!strcmp(HAL_PROCESS_NAME, key)) {
			pclose(f);
			return 1;
		}
	}
	pclose(f);
	return 0;
}

static void sfp_eeprom_read(char *eeprom_file, int port)
{
	struct shw_sfp_header sfp_header;
	FILE *fp;
	int ret;

	memset(&sfp_header, 0, sizeof(struct shw_sfp_header));
	if (check_hal() > 0) {
		/* HAL may disturb sfp's eeprom read! */
		pr_warning("HAL is running! It may disturb SFP's eeprom read"
			   "\n");
	}
	if (check_monit() > 0) {
		/* Monit may restart, which may disturb sfp's eeprom read! */
		pr_warning("Monit is running! It may restart HAL\n");
	}
	ret = shw_sfp_read(port - 1, I2C_SFP_ADDRESS, 0x0,
			   sizeof(struct shw_sfp_header),
			   (uint8_t *) &sfp_header);
	if (ret == I2C_DEV_NOT_FOUND) {
		pr_error("Unable to read SFP header for port %d\n", port);
		return;
	}

	fp = fopen(eeprom_file, "wb");
	if (!fp) {
		pr_error("Unable to open file %s!\n", eeprom_file);
		return;
	}

	ret = fwrite(&sfp_header, 1, sizeof(struct shw_sfp_header), fp);

	pr_info("Written %d bytes to file \"%s\" for port %d\n", ret,
		eeprom_file, port);
	fclose(fp);
}

static void sfp_eeprom_write(char *eeprom_file, int port)
{
	struct shw_sfp_header sfp_header;
	FILE *fp;
	int ret;

	memset(&sfp_header, 0, sizeof(struct shw_sfp_header));
	if (check_hal() > 0) {
		/* HAL may disturb sfp's eeprom read! */
		pr_error("HAL is running! It may disturb SFP's eeprom read\n");
		exit(1);
	}
	if (check_monit() > 0) {
		/* Monit may restart, which may disturb sfp's eeprom read! */
		pr_error("Monit is running! It may restart HAL\n");
		exit(1);
	}

	fp = fopen(eeprom_file, "rb");
	if (!fp) {
		pr_error("Unable to open file %s!\n", eeprom_file);
		exit(1);
	}

	ret = fread(&sfp_header, 1, sizeof(struct shw_sfp_header), fp);

	if (ret != sizeof(struct shw_sfp_header)) {
		pr_error("Wrong number of bytes read. Expected %d, read %d\n",
			 sizeof(struct shw_sfp_header), ret);
		exit(1);
	}
	fclose(fp);

	ret = shw_sfp_write(port - 1, I2C_SFP_ADDRESS, 0x0,
			   sizeof(struct shw_sfp_header),
			   (uint8_t *) &sfp_header);
	if (ret == I2C_DEV_NOT_FOUND) {
		pr_error("Unable to write SFP header for port %d\n", port);
		return;
	}
	pr_info("Written %d bytes to SFP's eeprom from file \"%s\" for port "
		"%d\n", ret, eeprom_file, port);
}


void print_version(char *prgname)
{
	printf("%s version %s, build by %s\n", prgname, __GIT_VER__,
	       __GIT_USR__);
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
	int operation = 0;
	char *eeprom_file = NULL;

	wrs_msg_init(argc, argv);
	nports = 18;
	dump_port = 1;

	while ((c = getopt(argc, argv, "a:hqvp:xVf:")) != -1) {
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
		case 'V':
			print_version(argv[0]);
			break;
		case 'q': break; /* done in wrs_msg_init() */
		case 'v': break; /* done in wrs_msg_init() */
		case 'a':
			if (!strcmp(optarg, "READ"))
				operation = SFP_EEPROM_READ;
			else if (!strcmp(optarg, "WRITE"))
				operation = SFP_EEPROM_WRITE;
			else {
				operation = 0;
				pr_error("Wrong operation!\n");
				exit(1);
			}
			break;
		case 'f':
			eeprom_file = strdup(optarg);
			if (!eeprom_file) {
				pr_error("File error!\n");
				exit(1);
			}
			break;
		case 'h':
		default:
			print_info(argv[0]);
			print_version(argv[0]);
			exit(1);
		}
	}

	/* init i2c, be carefull all i2c transfers are in race with hal! */
	assert_init(shw_io_init());
	assert_init(shw_fpga_mmap_init());
	assert_init(shw_sfp_buses_init());

	if (operation == SFP_EEPROM_READ) {
		sfp_eeprom_read(eeprom_file, dump_port);
		exit(0);
	}
	if (operation == SFP_EEPROM_WRITE) {
		sfp_eeprom_write(eeprom_file, dump_port);
		exit(0);
	}

	for (i = dump_port; i <= nports; i++) {
		memset(&shdr, 0, sizeof(shdr));
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
