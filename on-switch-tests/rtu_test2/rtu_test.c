/*
-------------------------------------------------------------------------------
-- Title      : Routing Table Unit Test Interface
-- Project    : White Rabbit Switch
-------------------------------------------------------------------------------
-- File       : rtu_test.c
-- Authors    : Maciej Lipinski (maciej.lipinski@cern.ch)
-- Company    : CERN BE-CO-HT
-- Created    : 2010-06-30
-- Last update: 2010-06-30
-- Description: This file stores entire test interface, which includes:
--               - functions used of data entry (VLAN, HCAM, HTAB, CONFIG) in both: 
--                 simulation and hardware
--               - functions used for request management


*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hw/switch_hw.h>

#include <hal_client.h>

#include "wrsw_rtu.h"
#include "rtu_testunit.h"

static volatile struct RTU_WB *RTU;
static volatile struct RTT_WB *RTT;

#define RTU_BANK_SIZE 65536

struct rtu_state {
	uint32_t htab[2][RTU_BANK_SIZE];
	int current_bank;
};

void zbt_write(uint32_t base, uint32_t *data, size_t nwords)
{
	while(RTU->MFIFO_CSR & RTU_MFIFO_CSR_FULL);
	
	RTU->MFIFO_R0 = RTU_MFIFO_R0_AD_SEL;
	RTU->MFIFO_R1 = base;

	while(nwords--)
	{
		while(RTU->MFIFO_CSR & RTU_MFIFO_CSR_FULL);
		RTU->MFIFO_R0 = 0;
		RTU->MFIFO_R1 = *data++;
	}
}


void zbt_clear()
{
	const int n_words = 65530;
	uint32_t *tmp;
	tmp = malloc(n_words * 4);
	memset(tmp, 0, n_words * 4);
	zbt_write(0, tmp, n_words);
	free(tmp);
}


void poll_ufifo()
{
		if(! (RTU->UFIFO_CSR & RTU_UFIFO_CSR_EMPTY))
		{
			uint32_t r0, r1, r2, r3, r4;
			r0 = RTU->UFIFO_R0;
			r1 = RTU->UFIFO_R1;
			r2 = RTU->UFIFO_R2;
			r3 = RTU->UFIFO_R3;
			r4 = RTU->UFIFO_R4;
			printf("UFIFO: port %d DstMac %02x:%02x:%02x:%02x:%02x:%02x SrcMac %02x:%02x:%02x:%02x:%02x:%02x ", 
				RTU_UFIFO_R4_PID_R(r4),
				(r1 >> 8) & 0xff,
				(r1 >> 0) & 0xff,
				(r0 >> 24) & 0xff,
				(r0 >> 16) & 0xff,
				(r0 >> 8) & 0xff,
				(r0 >> 0) & 0xff,
				(r3 >> 8) & 0xff,
				(r3 >> 0) & 0xff,
				(r2 >> 24) & 0xff,
				(r2 >> 16) & 0xff,
				(r2 >> 8) & 0xff,
				(r2 >> 0) & 0xff);
			
			if(r4 & RTU_UFIFO_R4_HAS_VID)
				printf("VLAN: %d ", RTU_UFIFO_R4_VID_R(r4));

			if(r4 & RTU_UFIFO_R4_HAS_PRIO)
				printf("Priority: %d ", RTU_UFIFO_R4_PRIO_R(r4));
		
			printf("\n");
		}
	}	


void poll_test_unit()
{
	uint32_t fpr = RTT -> FPR;
	uint32_t mask ,port;
	
	for(port = 0, mask = 1; port < 10; port++, mask<<=1)
	{
		if(! (fpr & mask))
		{
			volatile uint32_t *rfifo = ((volatile uint32_t *) &RTT->RFIFO_CH0_R0) + port;
			volatile uint32_t ent;

			while(! (RTT->FPR & mask))
			{
				ent = *rfifo;
				printf("RSP [port %d DPM=%08x prio=%d drop=%d\n", port, RTT_RFIFO_CH0_R0_DPM_R(ent), RTT_RFIFO_CH0_R0_PRIO_R(ent), ent & RTT_RFIFO_CH0_R0_DROP ? 1: 0);
			}
			
		}
	}
	
};

void init()
{

	if(halexp_client_init() < 0)
	{
		printf("Oops... Looks like the HAL is not running :( \n\n");
		exit(-1);
	}

	shw_fpga_mmap_init();
	RTU = _fpga_base_virt + FPGA_BASE_RTU;
	RTT = _fpga_base_virt + FPGA_BASE_RTU_TESTUNIT;

	RTU->GCR = RTU_GCR_HT_BSEL | RTU_GCR_G_ENA;
	zbt_clear();
	RTU->GCR = RTU_GCR_G_ENA;
	RTU->PCR0 = RTU_PCR0_LEARN_EN | RTU_PCR0_PASS_ALL | RTU_PCR0_B_UNREC | RTU_PCR0_PRIO_VAL_W(3) | RTU_PCR0_FIX_PRIO;
	RTU->PCR1 = RTU_PCR0_LEARN_EN | RTU_PCR0_PASS_ALL | RTU_PCR0_B_UNREC | RTU_PCR0_PRIO_VAL_W(3) | RTU_PCR0_FIX_PRIO;

}

main()
{
	init();
	
	for(;;)
	{
		poll_ufifo();
		poll_test_unit();
	}
	
	
	
}