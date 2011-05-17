
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hw/fpga_regs.h>
#include <hw/endpoint_regs.h>

#include <hal_client.h>

#define RTU_BANK_SIZE 65536

static const struct {
	char name[5];
	uint32_t base_addr;
} endpoints_map[]= {
	{"wru0", FPGA_BASE_MINIC_UP0 + 0x4000},
	{"wru1", FPGA_BASE_MINIC_UP1 + 0x4000},
	{NULL, 0}	
};

static const struct {
	char name[64];
	int offset;
} rmon_counters[] = {
	{"RX PCS sync lost events", 0},
	{"RX PCS invalid 8b10b codes", 1},
	{"RX PCS FIFO overruns", 2},
	{"TX PCS FIFO underruns", 3},

	{"RX CRC errors", 4},	
	{"RX valid frames", 5},	
	{"RX runt frames", 6},	
	{"RX giant frames",7},	

	{"RX errors reported by PCS", 8},	
	{"RX buffer drops", 9},	
	{"Received PAUSEs", 10},	
	{"Sent PAUSEs", 11},	
	{NULL, 0},
};

int show_stats(char *if_name)
{
	uint32_t  base_addr = 0;
	int i;
	
	for(i=0; endpoints_map[i].name; i++)
		if(!strcmp(endpoints_map[i].name, if_name))
		{
			base_addr = endpoints_map[i].base_addr;
			break;
		}
		
	if(!base_addr) return -1;
	
  printf("\033[2J\033[1;1H");

	printf("Stats for interface: %s\n-----------------------\n\n", if_name);

	for(i=0;i<12;i++)
		printf("%-40s: %d\n", rmon_counters[i].name, _fpga_readl(base_addr + 0x80 + rmon_counters[i].offset * 4));
	
	return 0;
}

void init()
{

	if(halexp_client_init() < 0)
	{
		printf("Oops... Looks like the HAL is not running :( \n\n");
		exit(-1);
	}

	shw_fpga_mmap_init();
}

main(int argc, char *argv[])
{
	init();
	
	for(;;)
	{
		if(show_stats(argv[1]) < 0)
			break;
		usleep(100000);
	}
	
	return 0;
	
	
}