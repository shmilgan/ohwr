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

#define __EXPORTED_HEADERS__ /* prevent a #warning notice from linux/types.h */
#include <linux/mii.h>

#include <regs/endpoint-regs.h>

#undef PACKED
#include "ptpd_netif.h"

#include "rt_ipc.h"
#include "hal_client.h"
#include "switch_hw.h"

#define WRS3_FPGA_BASE 0x10000000
#define WRS3_FPGA_SIZE 0x100000

void *fpga;

static struct EP_WB _ep_wb;

#define EP_REG(regname) ((uint32_t)((void*)&_ep_wb.regname - (void*)&_ep_wb))
#define IDX_TO_EP(x) (0x30000 + ((x) * 0x400))

#define fpga_writel(val, addr) *(volatile uint32_t *)(fpga + (addr)) = val
#define fpga_readl(addr) (*(volatile uint32_t *)(fpga + (addr)))

extern int rts_connect();

int fpga_map(char *prgname)
{
	int fdmem;
	void *addr;

	/* /dev/mem for mmap of both gpio and spi1 */
	if ((fdmem = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		fprintf(stderr, "%s: /dev/mem: %s\n",
			prgname, strerror(errno));
		return -1;
	}

	/* map a whole page (4kB, but we called getpagesize to know it) */
	addr = mmap(0, WRS3_FPGA_BASE, PROT_READ | PROT_WRITE,
		       MAP_SHARED, fdmem,
		       WRS3_FPGA_BASE);
	if (addr == MAP_FAILED) {
		fprintf(stderr, "%s: mmap(/dev/mem): %s\n",
			prgname, strerror(errno));
		return -1;
	}
	close(fdmem);

	fpga = addr;
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

int bslide_bins()
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

static void print_cal_stats()
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
	wr_timestamp_t ts_tx, ts_rx;
  wr_socket_t *sock;
	FILE *f_log = NULL;
  wr_sockaddr_t sock_addr, from;
	int bitslide,phase, i;

	signal (SIGINT, sighandler);
	for(i=0;i<MAX_BITSLIDES;i++)
	{
		bslides[i].occupied = 0;
		bslides[i].phase_min = 1000000;
		bslides[i].phase_max= -1000000;
	}

	if(argc >= 3)
		f_log = fopen(argv[3], "wb");
	
	sprintf(sock_addr.if_name, "wr%d", ep);
  sock_addr.family = PTPD_SOCK_RAW_ETHERNET; // socket type
  sock_addr.ethertype = 12345;
  memset(sock_addr.mac, 0xff, 6);

	assert(ptpd_netif_init() == 0);
    
  sock = ptpd_netif_create_socket(PTPD_SOCK_RAW_ETHERNET, 0, &sock_addr);
//	fpga_writel(EP_DMCR_N_AVG_W(1024) | EP_DMCR_EN, IDX_TO_EP(ep) + EP_REG(DMCR));

	if(	rts_connect() < 0)
	{
		printf("Can't connect to the RT subsys\n");
		return;
	}


	while(!quit)
	{
		char buf[64];
		wr_sockaddr_t to;
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
		int n = ptpd_netif_recvfrom(sock, &from, buf, 64, &ts_rx);
		

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
			wr_timestamp_t ts_tx, ts_rx;
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
	wr_timestamp_t ts_tx, ts_rx;
  wr_socket_t *sock;
  wr_sockaddr_t sock_addr, from;
  int adjust_count = 0;

	signal (SIGINT, sighandler);

	sprintf(sock_addr.if_name, "wr%d", ep);
  sock_addr.family = PTPD_SOCK_RAW_ETHERNET; // socket type
  sock_addr.ethertype = 12345;
  memset(sock_addr.mac, 0xff, 6);

	assert(ptpd_netif_init() == 0);
    
  sock = ptpd_netif_create_socket(PTPD_SOCK_RAW_ETHERNET, 0, &sock_addr);


	while(!quit)
	{
		char buf[64];
		wr_sockaddr_t to;

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
			ptpd_netif_recvfrom(sock, &from, buf, 64, &ts_rx);
			printf("TX timestamp: correct %d %12lld:%12d\n", ts_tx.correct, ts_tx.sec, ts_tx.nsec);
			printf("RX timestamp: correct %d %12lld:%12d\n", ts_rx.correct, ts_rx.sec, ts_rx.nsec);
			adjust_count --;
		}// else printf("AdjustInProgress\n");
			
		sleep(1);
	}
}

void rt_command(int ep, int argc, char *argv[])
{
	struct rts_pll_state pstate;
	hexp_port_list_t plist;
	int i;
	if(	rts_connect() < 0)
	{
		printf("Can't connect to the RT subsys\n");
		return;
	}
// #if 0
	if(	halexp_client_init() < 0)
	{
		printf("Can't connect to the HAL \n");
		plist.num_physical_ports = 18;
//		return -1;
	}
// #endif
	rts_get_state(&pstate);
//	halexp_query_ports(&plist);
	
	if(!strcmp(argv[3], "show"))
	{
    printf("RTS State Dump [%d physical ports]: \n", plist.num_physical_ports);
    printf("CurrentRef: %d Mode: %d Flags: %x\n", pstate.current_ref, pstate.mode, pstate.flags);
    for(i=0;i<plist.num_physical_ports;i++)
        printf("wr%-2d: setpoint: %-8dps current: %-8dps loopback: %-8dps flags: %x\n", i,
               pstate.channels[i].phase_setpoint,
               pstate.channels[i].phase_current,
               pstate.channels[i].phase_loopback,
               pstate.channels[i].flags);
	} else if (!strcmp(argv[3], "lock"))
	{
		int i;
		printf("locking to: %d\n", ep);
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
		printf("Enabling ptracker @ port %d\n", ep);
		
		rts_enable_ptracker(ep, 1);
	}
	else if (!strcmp(argv[3], "adj"))
	{
		printf("Enabling ptracker @ port %d\n", ep);
		
		rts_adjust_phase(ep, atoi(argv[4]));
	}
	else if (!strcmp(argv[3], "hdo"))
	{
		printf("Enabling holdover %s\n", argv[4]);
		if (!strcmp(argv[4], "enable"))
			rts_backup_channel(ep,RTS_HOLDOVER_ENA);
		else if (!strcmp(argv[4], "disable"))
			rts_backup_channel(ep,RTS_HOLDOVER_DIS);
		else
		      printf("wrong hdo command: %s\n", argv[4]);
		  
	}
	else if (!strcmp(argv[3], "mbox"))
	{
		int i;
		uint32_t tmp;
		for (i=0;i<0x10000;i=i+4)
		{
			tmp = fpga_readl(i);
			printf("%c%c%c%c",
			0x000000FF& tmp>>24,
			0x000000FF & tmp>>16,
			0x000000FF & tmp>>8 ,
			0x000000FF & tmp>>0 );
		}
		  
	}
	else if (!strcmp(argv[3], "hdost"))
	{
		
		printf("holdover state:\n");
		
		struct rts_hdover_state state;
		rts_get_holdover_state(&state,0);
		
		printf("enabled: %d\n",state.enabled);
		printf("state: %d\n",state.state);
		printf("type: %d\n",state.type);
		printf("hd_time: %d\n",state.hd_time);
		printf("hd_time: %d\n",state.flags);
		
		
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
	"RT subsystem command [show,lock,master,gm,adj,hdo, mbox, hdost]",
	rt_command},
	{NULL}

};


int main(int argc, char **argv)
{
	int i;
	if (fpga_map(argv[0]) < 0)
		exit(1);

	shw_init();

	if(argc<3)
	{

		printf("phytool - GTX serdes testing program\n");
		printf("usage: %s endpoint command [parameters]\n", argv[0]);
		printf("Commands:\n");
		for(i=0; commands[i].cmd;i++)
			printf("%-20s %-20s: %s\n", commands[i].cmd, commands[i].params, commands[i].desc);		
		return 0;
	}

	
	for(i=0; commands[i].cmd;i++)
		if(!strcmp(commands[i].cmd, argv[2]))
		{
			commands[i].func(atoi(argv[1]), argc, argv);
			return 0;
		}
	
	printf("Unrecognized command '%s'\n", argv[2]);
	return -1;
}
