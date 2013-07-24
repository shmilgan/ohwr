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
#define CNT_PP 38

struct cnt_word {
	uint32_t cnt;	//4 cntrs per 32-bit word
	uint32_t init;
};

struct cnt_word cnt_pp[NPORTS][CNT_PP];

extern int shw_fpga_mmap_init();

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
                   {"RTUfwd|"}, // 29 ---
                   {"Rpri_0|"}, // 30 -> p0
                   {"Rpri_1|"}, // 31 -> p1
                   {"Rpri_2|"}, // 32 -> p2
                   {"Rpri_3|"}, // 33 -> p3
                   {"Rpri_4|"}, // 34 -> p4
                   {"Rpri_5|"}, // 35 -> p5
                   {"Rpri_6|"}, // 36 -> p6
                   {"Rpri_7|"}  // 37 -> p7
                 };
static void read_cntval(int port, int adr, uint32_t *data);

int pstats_init(void)
{
	int err, i, j;
	uint32_t mem_val[2];
	int ret = 0;
	
	err = shw_fpga_mmap_init();
	if(err) {
		printf("shw_fpga_mmap_init failed with %d\n", err);
		return err;
	}

	printf("module initialized\n");

	for(i=0; i<NPORTS; ++i)
		for(j=0; j<CNT_PP; ++j) 
		{
			cnt_pp[i][j].init = 0;
			cnt_pp[i][j].cnt = 0;
		}
	parse_sysfs(1);
	return 0;
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
				cnt_pp[port][cntr].init = val;
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
				cnt_pp[port][cntr].cnt = val - cnt_pp[port][cntr].init;
			}
			fclose(file);
		}
	}
}


void print_first_n_cnts(int n_cnts)
{
	int cnt = 0;
	int port = 0;
	printf("P |");
	for(cnt=0; cnt<n_cnts; ++cnt) 
		printf("%2d:%s", cnt,info[cnt]);
	printf("\n");
	printf("----");
	for(cnt=0; cnt<n_cnts; ++cnt) 
		printf("----------");

	printf("\n");
	for(port=0; port<NPORTS; ++port) 
	{
		printf("%2u|", port);
		for(cnt=0; cnt<n_cnts;++cnt) 
			printf("%9u|", cnt_pp[port][cnt].cnt);
		printf("\n");
	} 
}

void print_chosen_cnts( int cnts_list[], int n_cnts)
{
	int cnt = 0;
	int port = 0;
	
	printf("                 --------Printing priority counters-----------\n\n");
	
	printf("P |");
	for(cnt=0; cnt<n_cnts; ++cnt) 
		printf("%2d:%s", cnts_list[cnt],info[cnts_list[cnt]]);
	printf("\n");
	printf("----");
	for(cnt=0; cnt<n_cnts; ++cnt) 
	printf("----------");
	printf("\n");
	for(port=0; port<NPORTS; ++port) 
	{
		printf("%2u|", port);
		for(cnt=0; cnt<n_cnts;++cnt) 
			printf("%9u|", cnt_pp[port][cnts_list[cnt]].cnt);
		printf("\n");
	}
}

void print_info(char *prgname)
{
	printf("usage: %s <command> [<values>]\n", prgname);  
	printf(""
			"   -h        Show this message\n"
			"   -p        Show counters for priorities\n");
}

int main(int argc, char **argv)
{
	time_t last_show=0;
	int option=0;
	int prio_cnts[] = {30,31,32,33,34,35,36,37};
	int op = 0;

	if(pstats_init()) return -1;
	op = getopt(argc, argv, "ph");
	
	while(1) 
	{
		printf("\033[2J\033[1;1H");
		if(argc > 1 && op !=-1)
	       	switch(op)
		{
			case 'p': 
				print_chosen_cnts(prio_cnts, 8);
				break;
			case 'h':
			default:
				print_info(argv[0]);
				exit(1);
				break;
	       }
		else
			print_first_n_cnts(30);
               
		parse_sysfs(0);
		sleep(1);
	}
	return 0;
}

