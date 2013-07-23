#include <stdio.h>
#include <stdlib.h>

#include <minipc.h>

#include "term.h"
#include<fpga_io.h>
#include<regs/pstats-regs.h>
#include "hal_client.h"
#include "rtud_exports.h"
#define MINIPC_TIMEOUT 200

#define pstats_rd(reg) \
	 _fpga_readl(FPGA_BASE_PSTATS + offsetof(struct PSTATS_WB, reg))

#define pstats_wr(reg, val) \
	 _fpga_writel(FPGA_BASE_PSTATS + offsetof(struct PSTATS_WB, reg), val)

#define NPORTS 8
#define CNT_PP 30
#define CNT_WRDS ((CNT_PP+3)/4)
#define CNT_TX_FRAME 18
#define CNT_RX_FRAME 19
struct s_word {
	uint32_t cnt[4];	//4 cntrs per 32-bit word
	uint32_t init[4];
	time_t last;
};
struct s_word cntrs[NPORTS][CNT_WRDS];
uint32_t cnt[NPORTS][CNT_PP];

hexp_port_list_t port_list;

static struct minipc_ch *rtud_ch;

extern int shw_fpga_mmap_init();
static void read_cntval(int port, int adr, uint32_t *data);

void init()
{ 
  if(halexp_client_init() < 0)
  {
    printf("Can't conenct to HAL mini-rpc server\n");
    exit(-1);
  }

  rtud_ch = minipc_client_create("rtud", 0);

  if(!rtud_ch)
  {
    printf("Can't connect to RTUd mini-rpc server\n");
    exit(-1);
  }


  term_init();
  halexp_query_ports(&port_list);
}
void parse_sysfs(int init)
{
	FILE *file;
	uint32_t port, cntr, val;
	char filename[30];

	if(init) {
		for(port=0; port<NPORTS; ++port) {

			sprintf(filename, "/proc/sys/pstats/port%u", port);
			file = fopen(filename, "r");
			for(cntr=0; cntr<CNT_PP; ++cntr) {
				fscanf(file, "%u\t", &val);
				cntrs[port][cntr/4].init[cntr%4] = val;
			}
			fclose(file);
		}
	}
	else {
		for(port=0; port<NPORTS; ++port) {

			sprintf(filename, "/proc/sys/pstats/port%u", port);
			file = fopen(filename, "r");
			for(cntr=0; cntr<CNT_PP; ++cntr) {
				fscanf(file, "%u\t", &val);
				cnt[port][cntr] = val-cntrs[port][cntr/4].init[cntr%4];
			}
			fclose(file);
		}
	}
}

int pstats_init(void)
{
	int err, i, j;
	uint32_t mem_val[2];

	err = shw_fpga_mmap_init();
	if(err) {
		printf("shw_fpga_mmap_init failed with %d\n", err);
		return err;
	}

	printf("module initialized\n");

	for(i=0; i<NPORTS; ++i)
	for(j=0; j<CNT_WRDS; ++j) {
		cntrs[i][j].cnt[0] = 0;
		cntrs[i][j].cnt[1] = 0;
		cntrs[i][j].cnt[2] = 0;
		cntrs[i][j].cnt[3] = 0;
		cntrs[i][j].last = 0;
	}
	parse_sysfs(1);
	return 0;
}

void rtudexp_get_tru_info(truexp_info_t *info, int arg)
{
	minipc_call(rtud_ch, MINIPC_TIMEOUT, &rtud_export_get_tru_info, info, arg);
}
void show_ports()
{
	int i, j;
	hexp_port_state_t state;
	truexp_info_t tru_info;
// 	term_pcprintf(3, 1, C_BLUE, "Switch ports:");

	rtudexp_get_tru_info(&tru_info,0);
	parse_sysfs(0);
	
	for(i=0; i<18;i++)
	{
		char if_name[10], found = 0;
		hexp_port_state_t state;

		snprintf(if_name, 5, "wr%d", i);
		
		for(j=0;j<port_list.num_ports;j++)
			if(!strcmp(port_list.port_names[j], if_name)) { found = 1; break; }
			
		if(!found) continue;
		
		halexp_get_port_state(&state, if_name);

		term_pcprintf(2+i, 1, C_WHITE, "%05s: ", if_name);
		if(state.up)
			term_cprintf(C_GREEN, "OK ");
		else
			term_cprintf(C_RED,   "-- ");

		term_cprintf(C_GREY, "RTU: ");

		if(0x1 & (tru_info.ports_pass_all >> i))
			term_cprintf(C_GREEN, "Forward ");
		else
			term_cprintf(C_RED,   "Block   ");
		
		term_cprintf(C_GREY, " TRU: ");

		if(0x1 & (tru_info.ports_up >> i))
			term_cprintf(C_GREEN, "up   ");
		else
			term_cprintf(C_RED,   "down ");

		if(0x1 & (tru_info.ports_stb_up >> i))
			term_cprintf(C_GREEN, "(stably up  ) ");
		else
			term_cprintf(C_GREY,  "(surely dead) ");
		
		if((0x1 & (tru_info.ports_pass_all >> i)) && (i != tru_info.backup_port))
			term_cprintf(C_GREEN, " ACTIVE   ");
		else if((0x1 & (tru_info.ports_pass_all >> i)) && (i == tru_info.backup_port))
			term_cprintf(C_WHITE, " BACKUP %d ",tru_info.active_port);
		else 
			term_cprintf(C_WHITE, "          ",tru_info.active_port);
		
		term_cprintf(C_GREY, " [Tx:");
		term_cprintf(C_WHITE," %10u",cnt[i][CNT_TX_FRAME]);
		term_cprintf(C_GREY, "]");
		term_cprintf(C_GREY, " [Rx:");
		term_cprintf(C_WHITE," %10u",cnt[i][CNT_RX_FRAME]);
		term_cprintf(C_GREY, "]");
	}
}

int track_onoff = 1;

void show_screen()
{
	term_clear();
	term_pcprintf(1, 1, C_BLUE, "WR Switch Topology Resolution Unit (TRU) Monitor v 1.1 [q = quit]");

	show_ports();
}

main()
{
	init();
        pstats_init();
	for(;;)
	{
		if(term_poll())
		{
			int c = term_get();

			if(c=='q')
				break;
			
		}

		show_screen();
		usleep(500000);
	}

	term_cprintf(C_GREY,"bye...\n\n");
	term_clear();
	term_restore();

}
