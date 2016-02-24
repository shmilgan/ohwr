#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <libwr/wrs-msg.h>
#include <regs/endpoint-regs.h>
#include <fpga_io.h>
#include <libwr/switch_hw.h>

static struct EP_WB _ep_wb;

#define EP_REG(regname) ((uint32_t)((void *)&_ep_wb.regname - (void *)&_ep_wb))
#define IDX_TO_EP(x) (0x30000 + ((x) * 0x400))

#define TX_ON_MASK	(1<<11)

void help(char *prgname)
{
	fprintf(stderr, "%s: Use: \"%s [<options>] <port_nr> <cmd>\n",
		prgname, prgname);
	fprintf(stderr,
		"  The program has the following options\n"
		"  -h   print help\n"
		"\n"
		"  Port numbers:\n"
		"      <port_nr> = 0 to 17 (on 18 port switch)\n"
		"\n"
		"  Commands <cmd> are:\n"
		"      on  - switch TX laser on.\n"
		"      off - switch TX laser off.\n");
	exit(1);
}

uint32_t pcs_read(int ep, uint32_t reg)
{
	_fpga_writel(IDX_TO_EP(ep) + EP_REG(MDIO_CR), EP_MDIO_CR_ADDR_W(reg));
	while (!(_fpga_readl(IDX_TO_EP(ep) + EP_REG(MDIO_ASR)) &
		EP_MDIO_ASR_READY))
		;
	return EP_MDIO_CR_DATA_R(_fpga_readl(IDX_TO_EP(ep) + EP_REG(MDIO_ASR)));
}

void pcs_write(int ep, uint32_t reg, uint32_t val)
{
	_fpga_writel(IDX_TO_EP(ep) + EP_REG(MDIO_CR), EP_MDIO_CR_ADDR_W(reg)
		| EP_MDIO_CR_DATA_W(val) | EP_MDIO_CR_RW);
	while (!(_fpga_readl(IDX_TO_EP(ep) + EP_REG(MDIO_ASR)) &
		EP_MDIO_ASR_READY))
		;
}

int main(int argc, char *argv[])
{
	int opt;
	int port_number;
	uint32_t reg;
	uint32_t value;

	while ((opt = getopt(argc, argv, "h")) != -1) {
		switch (opt) {
		case 'h':
		default:
			help(argv[0]);
		}
	}

	assert_init(shw_fpga_mmap_init());

	/* we need two arguments */
	if (argc > 2) {
		port_number = atoi(argv[1]);

		if (port_number < 0 || port_number > 17) {
			printf("Port number out of range\n");
			exit(1);
		}

		if (strcmp(argv[2], "on") == 0) {
			value = pcs_read(port_number, reg);
			pcs_write(port_number, reg, value&~TX_ON_MASK);
			exit(0);
		} else if (strcmp(argv[2], "off") == 0) {
			value = pcs_read(port_number, reg);
			pcs_write(port_number, reg, value|TX_ON_MASK);
			exit(0);
		} else {
			printf("Unknown command\n;");
			exit(1);
		}
	} else {
		printf("Need port and command\n");
		exit(1);
	}
}
