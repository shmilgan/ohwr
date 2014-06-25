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

static void parse_sysfs(int init);

#define pstats_rd(reg) \
	 _fpga_readl(FPGA_BASE_PSTATS + offsetof(struct PSTATS_WB, reg))

#define pstats_wr(reg, val) \
	 _fpga_writel(FPGA_BASE_PSTATS + offsetof(struct PSTATS_WB, reg), val)

#define NPORTS 18
#define CNT_PP 39

struct cnt_word {
	uint32_t cnt;	//4 cntrs per 32-bit word
	uint32_t init;
};

struct cnt_word cnt_pp[NPORTS][CNT_PP];
int use_ports;

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
                   {"Rpri_0|"}, // 21 -> p0
                   {"Rpri_1|"}, // 22 -> p1
                   {"Rpri_2|"}, // 23 -> p2
                   {"Rpri_3|"}, // 24 -> p3
                   {"Rpri_4|"}, // 25 -> p4
                   {"Rpri_5|"}, // 26 -> p5
                   {"Rpri_6|"}, // 27 -> p6
                   {"Rpri_7|"}, // 28 -> p7
                   {"RTUreq|"}, // 29
                   {"RTUrsp|"}, // 30
                   {"RTUdrp|"}, // 31
                   {"RTUhp |"}, // 32
                   {"RTUf-f|"}, // 33
                   {"RTUn-f|"}, // 34
                   {"RTUfst|"}, // 35
                   {"RTUful|"}, // 36
                   {"RTUfwd|"}, // 37 ---
                   {"TRUrsp|"}  // 38
                 };

int pstats_init(void)
{
	int err, i, j;

	err = shw_fpga_mmap_init();
	if(err) {
		printf("shw_fpga_mmap_init failed with %d\n", err);
		return err;
	}

	printf("module initialized\n");

	for(i=0; i<use_ports; ++i)
		for(j=0; j<CNT_PP; ++j)
		{
			cnt_pp[i][j].init = 0;
			cnt_pp[i][j].cnt = 0;
		}
	parse_sysfs(1);
	return 0;
}

static void parse_sysfs(int init)
{
	FILE *file;
	uint32_t port, cntr, val;
	char filename[30];

	if(init) {
		for(port=0; port<use_ports; ++port) {
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
		for(port=0; port<use_ports; ++port) {

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
	for(port=0; port<use_ports; ++port)
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
	
	printf("P |");
	for(cnt=0; cnt<n_cnts; ++cnt)
		printf("%2d:%s", cnts_list[cnt],info[cnts_list[cnt]]);
	printf("\n");
	printf("----");
	for(cnt=0; cnt<n_cnts; ++cnt)
	printf("----------");
	printf("\n");
	for(port=0; port<use_ports; ++port)
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
			"             Show >default< counters (no argument, counters which seem most useful and feeting on a wide screen)\n"
			"   -r        Show counters from RTU\n"
			"   -e        Show counters from Endpoints\n"
			"   -p        Show counters for priorities only (from Endpoints)\n"
			"   -a        Show all counters (don't fit screen)\n"
			"   -n        Define 8/18 ports version\n"
			"   -h        Show this message\n");

}

int main(int argc, char **argv)
{
	int prio_cnts[] = {21,22,23,24,25,26,27,28}; //8
	int def_cnts[]  = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,29,30,31,32,33,34,35,36,37}; //30
	int rtu_cnts[]  = {29,30,31,32,33,34,35,36,37,38}; //10
	int ep_cnts[]   = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28}; //29
	int op = 0, c;

	use_ports = NPORTS;
	while ( (c = getopt(argc, argv, "pheran:")) != -1) {
		switch(c) {
			case 'n':
				use_ports = atoi(optarg);
				break;
			case 'p':
			case 'e':
			case 'r':
			case 'a':
				op = c;
				break;
			case 'h':
			default:
				print_info(argv[0]);
				exit(1);
		}
	}

	if(pstats_init()) return -1;
	
	while(1)
	{
		printf("\033[2J\033[1;1H");
		parse_sysfs(0);
		switch(op) {
			case 'p':
				print_chosen_cnts(prio_cnts, 8);
				break;
			case 'e':
				print_chosen_cnts(ep_cnts, 29);
				break;
			case 'r':
				print_chosen_cnts(rtu_cnts, 10);
				break;
			case 'a':
				print_first_n_cnts(CNT_PP);
				break;
			default:
				print_chosen_cnts(def_cnts, 30);
		}
		sleep(1);
	}
	return 0;
}

