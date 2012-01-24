/* 
 * Copyright (c) 2011 Grzegorz Daniluk <g.daniluk@elproma.com.pl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define BASE_FPGA 0x10000000
#define SIZE_FPGA 0x20000


#define LM32_RAM_BASE 0x0
#define GPIO_BASE 0x10300
#define GPIO_COR  0x0
#define GPIO_SOR  0x4
#define LM32_RESET_PIN 2

static void *base_fpga;

static void fpga_writel(uint32_t data, uint32_t addr)
{
	*(volatile uint32_t *)(base_fpga + addr) = data;
}

static uint32_t fpga_readl(uint32_t addr)
{
	return	*(volatile uint32_t *)(base_fpga + addr);
}

int conv_endian(int x)
{
  return ((x&0xff000000)>>24)
	  + ((x&0x00ff0000)>>8)
	  + ((x&0x0000ff00)<<8)
	  + ((x&0x000000ff)<<24);
}

void rst_lm32(int rst)
{
	fpga_writel(1 << LM32_RESET_PIN,
		    GPIO_BASE + (rst ? GPIO_SOR : GPIO_COR));
}

void copy_lm32(uint32_t *buf, int buf_nwords, uint32_t base_addr)
{
  int i;
  printf("Writing memory: ");

  for(i=0;i<buf_nwords;i++)
  {
  	fpga_writel(conv_endian(buf[i]), base_addr + i *4);
  	if(!(i & 0xfff)) { printf("."); fflush(stdout); }
  }

  printf("\nVerifing memory: ");

  for(i=0;i<buf_nwords;i++)
  {
  	uint32_t x = fpga_readl(base_addr+ i*4);
	if(conv_endian(buf[i]) != x)
	{
	 	printf("Verify failed (%x vs %x)\n", conv_endian(buf[i]), x);
	 	return ;
	}

  	if(!(i & 0xfff)) { printf("."); fflush(stdout); }
  }

	printf("OK.\n");
}

int load_lm32_child(char *fname)
{
	uint32_t *buf;
	FILE *f;
	int fdmem;

	/* /dev/mem for mmap of both gpio and spi1 */
	if ((fdmem = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		fprintf(stderr, "%s: /dev/mem: %s\n", __func__,
			strerror(errno));
		exit(1);
	}

	/* map a whole page (4kB, but we called getpagesize to know it) */
	base_fpga = mmap(0, SIZE_FPGA, PROT_READ | PROT_WRITE,
		       MAP_SHARED, fdmem,
		       BASE_FPGA);

	if (base_fpga == MAP_FAILED) {
		fprintf(stderr, "%s: mmap(/dev/mem): %s\n",
			__func__, strerror(errno));
		exit(1);
	}


   	f=fopen(fname,"rb");
   	if(!f)
   	{
   	 	fprintf(stderr, "Input file not found.\n");
   	 	return -1;
   	}

   	fseek(f, 0, SEEK_END);
   	int size = ftell(f);
   	rewind(f);

   	buf = malloc(size + 4);
   	fread(buf, 1, size, f);
   	fclose(f);

	rst_lm32(1);
	copy_lm32(buf, (size + 3) / 4, 0);
	rst_lm32(0);

//	mbn_stats(mb_handle);

  return 0;
}

int load_lm32_main(char *fname)
{
	int pid = fork();
	int status;

	switch(pid) {
	case -1:
		fprintf(stderr, "fork(): %s\n", strerror(errno));
		return -1;
	case 0: /* child */
		load_lm32_child(fname);
		exit(0);
	default: /* parent */
		waitpid(pid, &status, 0);
		if (!WEXITSTATUS(status))
			return 0;
		return -1;
	}
}
