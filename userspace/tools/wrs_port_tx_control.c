#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <libwr/wrs-msg.h>
#include <regs/endpoint-regs.h>
#include <fpga_io.h>
#include <libwr/switch_hw.h>
#include <libwr/shmem.h>

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

int get_nports_from_hal(void)
{
	struct hal_shmem_header *h;
	struct wrs_shm_head *hal_head = NULL;
	int hal_nports_local; /* local copy of number of ports */
	int ii;
	int n_wait = 0;
	int ret;

	/* wait for HAL */
	while ((ret = wrs_shm_get_and_check(wrs_shm_hal, &hal_head)) != 0) {
		n_wait++;
		if (n_wait > 10) {
			if (ret == 1) {
				fprintf(stderr, "rtu_stat: Unable to open "
					"HAL's shm !\n");
			}
			if (ret == 2) {
				fprintf(stderr, "rtu_stat: Unable to read "
					"HAL's version!\n");
			}
			exit(1);
		}
		sleep(1);
	}

	h = (void *)hal_head + hal_head->data_off;

	n_wait = 0;
	while (1) { /* wait for 10 sec for HAL to produce consistent nports */
		n_wait++;
		ii = wrs_shm_seqbegin(hal_head);
		/* Assume number of ports does not change in runtime */
		hal_nports_local = h->nports;
		if (!wrs_shm_seqretry(hal_head, ii))
			break;
		fprintf(stderr, "rtu_stat: Wait for HAL.\n");
		if (n_wait > 10) {
			exit(1);
		}
		sleep(1);
	}

	/* check hal's shm version */
	if (hal_head->version != HAL_SHMEM_VERSION) {
		fprintf(stderr, "rtu_stat: unknown HAL's shm version %i "
			"(known is %i)\n",
			hal_head->version, HAL_SHMEM_VERSION);
		exit(-1);
	}

	if (hal_nports_local > HAL_MAX_PORTS) {
		fprintf(stderr, "rtu_stat: Too many ports reported by HAL. "
			"%d vs %d supported\n",
			hal_nports_local, HAL_MAX_PORTS);
		exit(-1);
	}
	return hal_nports_local;
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

		if (port_number < 0 || port_number > get_nports_from_hal()-1) {
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
