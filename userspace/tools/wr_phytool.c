/*\
 * WR Switch PHY testing tool
 	 Tomasz Wlostowski / 2012

	 WARNING: This is just my internal code for testing some timing-related stuff.
	 				  I'll document it and clean it up in the future, but for now, please
	 				  don't ask questions. sorry
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <signal.h>

#include <regs/endpoint-regs.h>

#undef PACKED
#include <libwr/ptpd_netif.h>
#define __EXPORTED_HEADERS__ /* prevent a #warning notice from linux/types.h */
#define _LINUX_IF_ETHER_H /* prevent linux/if_ether.h that redefines ethhdr  */
#include <linux/mii.h>

#include <rt_ipc.h>
#include <libwr/switch_hw.h>

#include <libwr/shmem.h>
#include <libwr/hal_shmem.h>
#include <libwr/wrs-msg.h>
#include <libwr/util.h>

#define WRS3_FPGA_BASE 0x10000000
#define WRS3_FPGA_SIZE 0x100000

void *fpga;

static struct EP_WB _ep_wb;


#define EP_REG(regname) ((uint32_t)((void*)&_ep_wb.regname - (void*)&_ep_wb))
#define IDX_TO_EP(x) (0x30000 + ((x) * 0x400))

#define fpga_writel(val, addr) *(volatile uint32_t *)(fpga + (addr)) = val
#define fpga_readl(addr) (*(volatile uint32_t *)(fpga + (addr)))

static int hal_nports_local;
static struct wrs_shm_head *hal_head;
static struct hal_port_state *hal_ports;

int hal_shm_init(void)
{
	int ii;
	struct hal_shmem_header *h;
	int n_wait = 0;
	int ret;

	/* wait for HAL */
	while ((ret = wrs_shm_get_and_check(wrs_shm_hal, &hal_head)) != 0) {
		n_wait++;
		if (n_wait > 10) {
			if (ret == WRS_SHM_OPEN_FAILED) {
				fprintf(stderr, "wr_phytool: Unable to open "
					"HAL's shm !\n");
			}
			if (ret == WRS_SHM_WRONG_VERSION) {
				fprintf(stderr, "wr_phytool: Unable to read "
					"HAL's version!\n");
			}
			if (ret == WRS_SHM_INCONSISTENT_DATA) {
				fprintf(stderr, "wr_phytool: Unable to read "
					"consistent data from HAL's shmem!\n");
			}
			return(-1);
		}
		sleep(1);
	}

	/* check hal's shm version */
	if (hal_head->version != HAL_SHMEM_VERSION) {
		fprintf(stderr, "wr_mon: unknown hal's shm version %i "
			"(known is %i)\n",
			hal_head->version, HAL_SHMEM_VERSION);
		return -1;
	}

	h = (void *)hal_head + hal_head->data_off;

	while (1) { /* wait forever for HAL to produce consistent nports */
		ii = wrs_shm_seqbegin(hal_head);
		/* Assume number of ports does not change in runtime */
		hal_nports_local = h->nports;
		if (!wrs_shm_seqretry(hal_head, ii))
			break;
		fprintf(stderr, "INFO: wr_phytool Wait for HAL.\n");
		sleep(1);
	}

	/* Even after HAL restart, HAL will place structures at the same
	 * addresses. No need to re-dereference pointer at each read. */
	hal_ports = wrs_shm_follow(hal_head, h->ports);
	if (hal_nports_local > HAL_MAX_PORTS) {
		fprintf(stderr,
			"FATAL: wr_phytool Too many ports reported by HAL."
			"%d vs %d supported\n",
			hal_nports_local, HAL_MAX_PORTS);
		return -1;
	}
	return 0;
}

uint32_t pcs_read(int ep, uint32_t reg)
{
	fpga_writel(EP_MDIO_CR_ADDR_W(reg), IDX_TO_EP(ep) + EP_REG(MDIO_CR));
	while(! (fpga_readl(IDX_TO_EP(ep) + EP_REG(MDIO_ASR)) & EP_MDIO_ASR_READY));
	return EP_MDIO_CR_DATA_R(fpga_readl(IDX_TO_EP(ep) + EP_REG(MDIO_ASR)));
}

void pcs_write(int ep, uint32_t reg, uint32_t val)
{
	fpga_writel(EP_MDIO_CR_ADDR_W(reg) | EP_MDIO_CR_DATA_W(val) | EP_MDIO_CR_RW, IDX_TO_EP(ep) + EP_REG(MDIO_CR));
	while(! (fpga_readl(IDX_TO_EP(ep) + EP_REG(MDIO_ASR)) & EP_MDIO_ASR_READY));
}

void dump_pcs_regs(int ep, int argc, char *argv[])
{
	int i;
	printf("PCS registers dump for endpoint %d:\n", ep);
	for(i=0;i<17;i++)
		printf("R%d = 0x%08x\n",i,pcs_read(ep, i));
}

void write_pcs_reg(int ep, int argc, char *argv[])
{
	int reg, data;
	if (argc < 4) {
		printf("Not enough parameters %d\n", argc);
		exit(1);
	}
	sscanf(argv[3], "%x", &reg);
	sscanf(argv[4], "%x", &data);
	pcs_write(ep, reg, data);
	printf("R%d = 0x%08x\n",reg, pcs_read(ep, reg));
}


void tx_cal(int ep, int argc, char *argv[])
{
	int on_off = atoi(argv[3]) ? 1 : 0;
	printf("TX cal pattern %s\n", on_off ? "ON":"OFF");

	pcs_write(ep, 16, on_off);
	printf("rdbk:%x\n",pcs_read(ep, 16));
}

void try_autonegotiation(int ep, int argc, char *argv[])
{
	pcs_write(ep, MII_ADVERTISE, 0x1a0);
	pcs_write(ep, MII_BMCR, BMCR_ANENABLE | BMCR_ANRESTART);

	printf("Autonegotiation: ");
	for(;;)
	{
		printf(".");
		fflush(stdout);
		if(pcs_read(ep, MII_BMSR) & BMSR_ANEGCOMPLETE)
			break;
		sleep(1);
	}
	printf("\nComplete: LPA %x\n", pcs_read(ep, MII_LPA));
}

int get_bitslide(int ep)
{
	return (pcs_read(ep, 16) >> 4) & 0x1f;
}

int get_phase(int ep, int *phase)
{
	uint32_t dmsr=fpga_readl(IDX_TO_EP(ep) + EP_REG(DMSR));
	if(dmsr & EP_DMSR_PS_RDY)
	{
		*phase = (dmsr & 0xffffff) / 1024;
//		fprintf(stderr ,"NewPhase: %d\n", *phase);

		return 1;
	}
	return 0;
}

#define MAX_BITSLIDES 20


static	struct {
		int occupied;
		int phase_min, phase_max, phase_dev;
		int rx_ahead;
		int delta;
		int hits;
} bslides[MAX_BITSLIDES];

int bslide_bins(void)
{
	int i, hits = 0;

	for(i=0;i<MAX_BITSLIDES;i++)
		if(bslides[i].occupied) hits++;

	return hits;
}

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
void bslide_update(int phase, int delta, int ahead, int bs)
{
	bslides[bs].occupied = 1;
	bslides[bs].phase_min = MIN(phase, bslides[bs].phase_min);
	bslides[bs].phase_max = MAX(phase, bslides[bs].phase_max);
	bslides[bs].phase_dev = bslides[bs].phase_max - bslides[bs].phase_min;
	bslides[bs].delta = delta;
	bslides[bs].rx_ahead = ahead;
	bslides[bs].hits++;
}

static int quit = 0;

void sighandler(int sig)
{
	quit = 1;
}

static void print_cal_stats(void)
{
	int i,last_occupied = -1;
	printf("Calibration statistics: \n");
	printf("bitslide[UIs] | Delta[ns] | Ahead[bool] | phase_min[tics] | phase_max[tics] | phase_dev[tics] | hits | delta_prev[ps] \n");
	for(i=0;i<MAX_BITSLIDES;i++)
		if(bslides[i].occupied)
		{
			printf("%-15d %-11d %-13d %-17d %-17d %-17d %-6d %d\n",
				i,
				bslides[i].delta,
				bslides[i].rx_ahead,
				bslides[i].phase_min,
				bslides[i].phase_max,
				bslides[i].phase_dev,
				bslides[i].hits,
				(last_occupied >= 0) ? bslides[i].delta-bslides[last_occupied].delta:0
			);

			last_occupied = i;
		}
	printf("\n");
}

void calc_trans(int ep, int argc, char *argv[])
{
	struct wr_tstamp ts_tx, ts_rx;
	struct wr_socket *sock;
	FILE *f_log = NULL;
	struct wr_sockaddr sock_addr, from;
	int bitslide,phase, i;
	struct hal_port_state *port;

	memset(&ts_tx, 0, sizeof(ts_tx));
	memset(&ts_rx, 0, sizeof(ts_rx));
	signal (SIGINT, sighandler);
	for(i=0;i<MAX_BITSLIDES;i++)
	{
		bslides[i].occupied = 0;
		bslides[i].phase_min = 1000000;
		bslides[i].phase_max= -1000000;
	}

	if(argc >= 3)
		f_log = fopen(argv[3], "wb");

	snprintf(sock_addr.if_name, sizeof(sock_addr.if_name), "wri%d", ep + 1);
	sock_addr.family = PTPD_SOCK_RAW_ETHERNET; // socket type
	sock_addr.ethertype = 12345;
	memset(sock_addr.mac, 0xff, 6);

	assert(ptpd_netif_init() == 0);
	assert(hal_shm_init() == 0);
	port = hal_lookup_port(hal_ports, hal_nports_local,
			       sock_addr.if_name);
	sock = ptpd_netif_create_socket(PTPD_SOCK_RAW_ETHERNET, 0, &sock_addr,
					port);

//	fpga_writel(EP_DMCR_N_AVG_W(1024) | EP_DMCR_EN, IDX_TO_EP(ep) + EP_REG(DMCR));

	if( rts_connect(NULL) < 0)
	{
		printf("Can't connect to the RT subsys\n");
		return;
	}


	while(!quit)
	{
		char buf[64];
		struct wr_sockaddr to;
		struct rts_pll_state pstate;

		pcs_write(ep, MII_BMCR, BMCR_PDOWN);
		usleep(10000);
		pcs_write(ep, MII_BMCR, 0); //BMCR_ANENABLE | BMCR_ANRESTART);
		pcs_read(ep, MII_BMSR);


		while(! (pcs_read(ep, MII_BMSR) & BMSR_LSTATUS)) usleep(10000);


		usleep(200000);
		bitslide = get_bitslide(ep);
		rts_enable_ptracker(ep, 0);
		rts_enable_ptracker(ep, 1);
		usleep(1000000);

//		get_phase(ep, &phase);
		rts_get_state(&pstate);

		phase = pstate.channels[ep].phase_loopback;
		printf("phase %d flags %x\n", phase,  pstate.channels[ep].flags);

	//		ptpd_netif_get_dmtd_phase(sock, &phase);



		memset(to.mac, 0xff, 6);
		to.ethertype = 12345;
		to.family = PTPD_SOCK_RAW_ETHERNET; // socket type

		ptpd_netif_sendto(sock, &to, buf, 64, &ts_tx);
		int n = ptpd_netif_recvfrom(sock, &from, buf, 64, &ts_rx,
					    port);


		if(n>0)
		{
			int delta = ts_rx.nsec * 1000 + ts_rx.phase - ts_tx.nsec * 1000;
			bslide_update(phase, delta, ts_rx.raw_ahead, bitslide);
			printf("delta %d (adv %d), bitslide: %d bins: %d phase %d, rxphase %d\n", delta, ts_rx.raw_ahead, bitslide, bslide_bins(), phase, ts_rx.phase);

			if(f_log) fprintf(f_log, "%d %d %d %d %d\n", bitslide, ts_tx.nsec, ts_rx.raw_nsec, ts_rx.raw_phase, ts_rx.raw_ahead);
		}
		usleep(500000);
	}

	if(f_log) fclose(f_log);
	print_cal_stats();
}



void analyze_phase_log(int ep, int argc, char *argv[])
{
	FILE *f_log = NULL;
	int bitslide,phase, i,trans_point;

	f_log=fopen(argv[3], "r");

	if(argc>4)
		trans_point = atoi(argv[4]);
	else
		trans_point = 1000;

	for(i=0;i<MAX_BITSLIDES;i++)
	{
		bslides[i].occupied = 0;
		bslides[i].phase_min = 1000000;
		bslides[i].phase_max= -1000000;
	}


	while(!feof(f_log))
	{
			struct wr_tstamp ts_tx, ts_rx;
			fscanf(f_log, "%d %d %d %d %d\n", &bitslide, &ts_tx.nsec, &ts_rx.raw_nsec, &ts_rx.raw_phase, &ts_rx.raw_ahead);

			ts_rx.nsec = ts_rx.raw_nsec;
			phase = ts_rx.raw_phase;

			ptpd_netif_linearize_rx_timestamp(&ts_rx, ts_rx.raw_phase, ts_rx.raw_ahead, trans_point, 16000);

			int delta = ts_rx.nsec * 1000 + ts_rx.phase - ts_tx.nsec * 1000;
			bslide_update(phase, delta, ts_rx.raw_ahead, bitslide);

	}

	printf("Transition point: %d ps. \n", trans_point);
	print_cal_stats();
	fclose(f_log);
}

void pps_adjustment_test(int ep, int argc, char *argv[])
{
	struct wr_tstamp ts_tx, ts_rx;
	struct wr_socket *sock;
	struct wr_sockaddr sock_addr, from;
	int adjust_count = 0;
	struct hal_port_state *port;

	memset(&ts_tx, 0, sizeof(ts_tx));
	memset(&ts_rx, 0, sizeof(ts_rx));
	signal (SIGINT, sighandler);

	snprintf(sock_addr.if_name, sizeof(sock_addr.if_name), "wri%d", ep + 1);
	sock_addr.family = PTPD_SOCK_RAW_ETHERNET; // socket type
	sock_addr.ethertype = 12345;
	memset(sock_addr.mac, 0xff, 6);

	assert(ptpd_netif_init() == 0);
	assert(hal_shm_init() == 0);
	port = hal_lookup_port(hal_ports, hal_nports_local,
			       sock_addr.if_name);
	sock = ptpd_netif_create_socket(PTPD_SOCK_RAW_ETHERNET, 0, &sock_addr,
					port);

	while(!quit)
	{
		char buf[64];
		struct wr_sockaddr to;

		memset(to.mac, 0xff, 6);
		to.ethertype = 12345;
		to.family = PTPD_SOCK_RAW_ETHERNET; // socket type

		if(adjust_count == 0)
		{
			ptpd_netif_adjust_counters(1,0);// 500000000);
			adjust_count = 8;
		}

//		if(!ptpd_netif_adjust_in_progress())
		{
			ptpd_netif_sendto(sock, &to, buf, 64, &ts_tx);
			ptpd_netif_recvfrom(sock, &from, buf, 64, &ts_rx,
					    port);
			printf("TX timestamp: correct %d %12lld:%12d\n", ts_tx.correct, ts_tx.sec, ts_tx.nsec);
			printf("RX timestamp: correct %d %12lld:%12d\n", ts_rx.correct, ts_rx.sec, ts_rx.nsec);
			adjust_count --;
		}// else printf("AdjustInProgress\n");

		sleep(1);
	}
}

void rt_command(int ep, int argc, char *argv[])
{
/* ep is 0..17 */

	struct rts_pll_state pstate;
	int i;

	assert(hal_shm_init() == 0); /* to get hal_nports_local */

	if(	rts_connect(NULL) < 0)
	{
		printf("Can't connect to the RT subsys\n");
		exit(1);
	}

	rts_get_state(&pstate);

	if(!strcmp(argv[3], "show"))
	{
		printf("RTS State Dump [%d physical ports]:\n",
		       hal_nports_local);
		printf("CurrentRef: %d Mode: %d Flags: %x\n", pstate.current_ref, pstate.mode, pstate.flags);
		for (i = 0; i < hal_nports_local; i++)
			printf("wri%-2d: setpoint: %-8dps current: %-8dps "
			       "loopback: %-8dps flags: %x\n", i + 1,
			       pstate.channels[i].phase_setpoint,
			       pstate.channels[i].phase_current,
			       pstate.channels[i].phase_loopback,
			       pstate.channels[i].flags);
	} else if (!strcmp(argv[3], "lock"))
	{
		int i;
		printf("locking to: port %d wri%d\n", ep + 1, ep + 1);
		for(i=0;i<100;i++)
		{
			rts_get_state(&pstate);
			printf("setmode rv %d\n", rts_set_mode(RTS_MODE_BC));
			printf("lock rv %d\n", rts_lock_channel(ep, 0));
		}
	} else if (!strcmp(argv[3], "master"))
	{
		int i;
		printf("Enabling free-running master mode\n");
		for(i=0;i<10;i++)
		{
				printf("rv: %d\n", rts_set_mode(RTS_MODE_GM_FREERUNNING));
		}
//		rts_lock_channel(ep);
	}
	else if (!strcmp(argv[3], "track"))
	{
		printf("Enabling ptracker @ port %d (wri%d)\n", ep + 1, ep + 1);

		rts_enable_ptracker(ep, 1);
	}
}


struct {
	char *cmd;
	char *params;
	char *desc;
	void (*func)(int, int, char *argv[]);
} commands[] = {
	{
	"txcal",
	"[0/1]",
	"enable/disable transmission of a calibration pattern",
	tx_cal},
	{
	"autoneg",
	"",
	"perform autonegotiation procedure",
	try_autonegotiation},

	{
	"ttrans",
	"",
	"determine transition point",
	calc_trans},

	{
	"dump",
	"",
	"dump PCS regs ",
	dump_pcs_regs},
	{
	"wr",
	"",
	"write PCS reg ",
	write_pcs_reg},

	{
	"analyze",
	"log_file [transition_point]",
	"analyze phase log",
	analyze_phase_log},

	{
	"ppsadj",
	"",
	"PPS adjustment test",
	pps_adjustment_test},

	{
	"rt",
	"",
	"RT subsystem command [show,lock,master,gm]",
	rt_command},
	{NULL}

};


int main(int argc, char **argv)
{
	int i;
	int ep;

	wrs_msg_init(1, argv); /* only use argv[0]: no cmdline */

	if(argc<3)
	{

		printf("phytool - GTX serdes testing program\n");
		printf("usage: %s endpoint command [parameters]\n", argv[0]);
		printf("Commands:\n");
		for(i=0; commands[i].cmd;i++)
			printf("%-20s %-20s: %s\n", commands[i].cmd, commands[i].params, commands[i].desc);
		return 0;
	}

	fpga = create_map(WRS3_FPGA_BASE, WRS3_FPGA_SIZE);
	if (!fpga) {
		printf("%s: Unable to mmap\n", argv[0]);
		exit(1);
	}

	for(i=0; commands[i].cmd;i++)
		if(!strcmp(commands[i].cmd, argv[2]))
		{
			/* first parameter is an enpoint number 1..18 */
			ep = atoi(argv[1]) - 1;
			if (ep < 0) {
				printf("Wrong endpoint number %d\n", ep + 1);
				exit(1);
			}
			/* pass endpoint as number 0..17 */
			commands[i].func(ep, argc, argv);
			return 0;
		}

	printf("Unrecognized command '%s'\n", argv[2]);
	return -1;
}
