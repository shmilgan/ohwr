#include<stdio.h>
#include<unistd.h>
#include<fpga_io.h>
#include<regs/pstats-regs.h>
// #include<regs/dummy-regs.h>
#include<time.h>
#include<poll.h>

/*temp*/
#include <stddef.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <switch_hw.h>
#include <hal_client.h>



#define pstats_rd(reg) \
	 _fpga_readl(FPGA_BASE_PSTATS + offsetof(struct PSTATS_WB, reg))

#define pstats_wr(reg, val) \
	 _fpga_writel(FPGA_BASE_PSTATS + offsetof(struct PSTATS_WB, reg), val)

#define NPORTS 8
#define CNT_PP 30
#define CNT_WRDS ((CNT_PP+3)/4)

struct s_word {
	uint32_t cnt[4];	//4 cntrs per 32-bit word
	uint32_t init[4];
	time_t last;
};

struct s_word cntrs[NPORTS][CNT_WRDS];
uint8_t irqs[NPORTS];
uint8_t dbg_cnt=0, dbg_evt=0, L2_dbg_cnt=0, L2_dbg_evt=0;
uint8_t rrp_port, rrp_word;
uint8_t rrr_port, rrr_word;
uint32_t irqn = 0;

extern int shw_fpga_mmap_init();

//TEMP
uint8_t readouts[2][2000] = {{0}};
uint32_t r_iter[2]={0};

char info[][20] = {{"Tu-run|"}, // 0
                   {"Ro-run|"}, // 1 
                   {"Riv-cd|"}, // 2
                   {"Rsyn-l|"}, // 3
                   {"Rpause|"}, // 4
                   {"Rpf-dp|"}, // 5
                   {"Rpcs-e|"}, // 6
                   {"Rgiant|"}, // 7
                   {"Rrunt |"}, // 8
                   {"Rcrc_e|"}, // 9
                   {"Rpcl_0|"}, // 10
                   {"Rpcl_1|"}, // 11
                   {"Rpcl_2|"}, // 12
                   {"Rpcl_3|"}, // 13
                   {"Rpcl_4|"}, // 14
                   {"Rpcl_5|"}, // 15
                   {"Rpcl_6|"}, // 16
                   {"Rpcl_7|"}, // 17
                   {"Tframe|"}, // 18 
                   {"Rframe|"}, // 19
                   {"Rrtu_f|"}, // 20
                   {"RTUreq|"}, // 21
                   {"RTUrsp|"}, // 22
                   {"RTUdrp|"}, // 23
                   {"RTUhp |"}, // 24
                   {"RTUf-f|"}, // 25
                   {"RTUn-f|"}, // 26
                   {"RTUfst|"}, // 27
                   {"RTUful|"}, // 28
                   {"RTUfwd|"}  // 29
                 };
static void read_cntval(int port, int adr, uint32_t *data);

int debug = 0;

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

	rrp_port = 0;
	rrp_word = 0;
	rrr_port = 0;
	rrr_word = 0;

	return 0;
}

static void read_cntval(int port, int adr, uint32_t *data)
{
	uint32_t cr_val;

	cr_val = ( adr<<PSTATS_CR_ADDR_SHIFT | 
			port<<PSTATS_CR_PORT_SHIFT |
			PSTATS_CR_RD_EN );
	pstats_wr(CR, cr_val);

	data[0] = pstats_rd(L1_CNT_VAL);
	data[1] = pstats_rd(L2_CNT_VAL);
}

static void print_cntrs(void)
{
	uint8_t port, cnt;
	/*clear screen*/
	//printf("\e[2J\e[1;1H");
	printf("\n");

	for(port=0; port<NPORTS; ++port) {
		printf("PORT %u:", port);
		for(cnt=0; cnt<CNT_PP; ++cnt) { printf("\t%u", cntrs[port][cnt/4].cnt[cnt%4]);
		}
		printf("\n");
	}
	printf("L1: CNT_OV=%02x \tEVT_OV=%02x\n", dbg_cnt, dbg_evt);
	printf("L2: CNT_OV=%02x \tEVT_OV=%02x\n", L2_dbg_cnt, L2_dbg_evt);
	
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
			printf("\n");
			fclose(file);
		}
	}
	else {
		for(port=0; port<NPORTS; ++port) {

			sprintf(filename, "/proc/sys/pstats/port%u", port);
			file = fopen(filename, "r");
			printf("%2u|", port);
			for(cntr=0; cntr<CNT_PP; ++cntr) {
				fscanf(file, "%u\t", &val);
				printf("%9u|", val-cntrs[port][cntr/4].init[cntr%4]);
			}
			printf("\n");
			fclose(file);
		}
	}
}


void print_info(void)
{
  int cnt = 0;
  printf("P |");
  for(cnt=0; cnt<CNT_PP; ++cnt) 
  {
    printf("%2d:%s", cnt,info[cnt]);
  }
  printf("\n");
  printf("----");
  for(cnt=0; cnt<CNT_PP; ++cnt) 
  {
    printf("----------");
  }
  
  printf("\n");
}


int main(void)
{
	time_t last_show=0;

	if(pstats_init()) return -1;

	last_show = 0;
	while(1) {
               printf("\033[2J\033[1;1H");
               print_info();
		parse_sysfs(0);
		sleep(5);
	}
	return 0;
}

