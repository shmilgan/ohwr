/*
 * Trivial gpio bit banger.
 * Alessandro Rubini, 2011, for CERN.
 * Released to the public domain.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <linux/types.h>

#include <mach/at91_pio.h> /* -I$LINUX/arch/arm/mach-at91/include/ */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

unsigned char bstream[10*1000*1000]; /* me lazy bastard */

void *pio[5]; /* from mmap(/dev/mem) */

/* describe the g45 memory layout */
unsigned char *piobase = (void *)0xfffff000;

int offsets[] = {0x200, 0x400, 0x600, 0x800, 0xa00};

void *pio[5]; /* from mmap(/dev/mem) plus offsets above */

enum {
	PIOA = 0,
	PIOB = 1,
	PIOC = 2,
	PIOD = 3,
	PIOE = 4
};

/* macro to access 32-bit registers */
#define __PIO(port, regname) (*(volatile __u32 *)(pio[port] + regname))

/* This sets a bit. To clear output se set ODR (output disable register) */
static inline void pio_set(int regname, int port, int bit)
{
	__PIO(port, regname)  |= (1 << bit);
}

/* This only reads input data, returns 0 or non-0 */
static int pio_get(int port, int bit)
{
	return __PIO(port, PIO_PDSR) & (1 << bit);
}

/* our bits */
#define TK0		PIOD, 0 //out
#define TD0		PIOD, 2 //out

#define LED0		PIOA, 0
#define LED1		PIOA, 1
#define DONE		PIOA, 2 //in -- inverted
#define INITB		PIOA, 3 //in
#define PROGRAMB	PIOA, 4 //out
#define FPGA_RESET	PIOA, 5 //out


void ud(int usecs) /* horrible udelay thing without scheduling */
{
	struct timeval tv1, tv2;
	gettimeofday(&tv1, NULL);
	do
		gettimeofday(&tv2, NULL);
	while ((tv2.tv_sec - tv1.tv_sec) * 1000*1000
	       + tv2.tv_usec - tv1.tv_usec < usecs);
}


int main(int argc, char **argv)
{
	int pagesize, bs_size, i;
	int fdmem;

	if (argc != 2) {
		fprintf(stderr, "%s: Use: \"%s <bitstream>\"\n", argv[0],
			argv[0]);
		exit(1);
	}

	{
		/* read the bisstream file */
		FILE *f = fopen(argv[1], "r");

		if (!f) {
			fprintf(stderr, "%s: %s: %s\n", argv[0], argv[1],
				strerror(errno));
			exit(1);
		}
		bs_size = fread(bstream, 1, sizeof(bstream), f);
		if (bs_size < 0) {
			fprintf(stderr, "%s: read(%s): %s\n", argv[0],
				argv[1], strerror(errno));
			exit(1);
		}
		if (bs_size == sizeof(bstream)) {
			fprintf(stderr, "%s: %s: too big\n", argv[0], argv[1]);
			exit(1);
		}
		fclose(f);
	}

	pagesize = getpagesize(); /* can't fail */

	/* /dev/mem for mmap of both gpio and spi1 */
	if ((fdmem = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		fprintf(stderr, "%s: /dev/mem: %s\n", argv[0], strerror(errno));
		exit(1);
	}

	/* map a whole page (4kB, but we called getpagesize to know it) */
	piobase = mmap(0, pagesize, PROT_READ | PROT_WRITE,
		       MAP_SHARED, fdmem,
		       (int)piobase & (~(pagesize-1)));
	if (piobase == MAP_FAILED) {
		fprintf(stderr, "%s: mmap(/dev/mem): %s\n",
			argv[0], strerror(errno));
		exit(1);
	}

	/* ok, now move the pointers within the page, we won't munmap anyways */
	for (i = 0; i < ARRAY_SIZE(pio); i++) {
		pio[i] = piobase + offsets[i];
	}

	/*
	 * all of this stuff is working on gpio so enable pio, out or in
	 */

	/* clock and data are output, low by now */
	pio_set(PIO_PER, TK0);
	pio_set(PIO_CODR, TK0);
	pio_set(PIO_OER, TK0);

	pio_set(PIO_PER, TD0);
	pio_set(PIO_CODR, TD0);
	pio_set(PIO_OER, TD0);

	/* fpga_reset is high */
	pio_set(PIO_PER, FPGA_RESET);
	pio_set(PIO_SODR, FPGA_RESET);
	pio_set(PIO_OER, FPGA_RESET);

	/* program_b is output high: is is pulsed low to start programming */
	pio_set(PIO_PER, PROGRAMB);
	pio_set(PIO_SODR, PROGRAMB);
	pio_set(PIO_OER, PROGRAMB);

	/* init_b is input: it's the fpga telling us it's done initializing */
	pio_set(PIO_PER, INITB);
	pio_set(PIO_ODR, INITB);

	/* done is input */
	pio_set(PIO_PER, DONE);
	pio_set(PIO_ODR, DONE);

	/* flip the leds for 0.1 secs to show we are alive and well */
	pio_set(PIO_PER, LED0);
	pio_set(PIO_OER, LED0);
	pio_set(PIO_CODR, LED0);
	pio_set(PIO_PER, LED1);
	pio_set(PIO_OER, LED1);
	pio_set(PIO_CODR, LED1);
	ud(100*1000);
	pio_set(PIO_SODR, LED0);
	pio_set(PIO_SODR, LED1);

	/*
	 * What follows is based on information from
	 * www.xilinx.com/support/documentation/user_guides/ug360.pdf
	 */

	/* First, turn PROGRAM_B low */
	pio_set(PIO_CODR, PROGRAMB);

	/* Then, wait little and then check init_b must go low */
	for (i = 0; i < 50; i++) {
		ud(10);
		if (!pio_get(INITB))
			break;
	}

	/* check status: initb must be low and done must be low */
	if (pio_get(INITB)) {
		fprintf(stderr, "%s: INIT_B is still high after PROGRAM_B\n",
			argv[0]);
		//exit(1);
	}

	if (!pio_get(DONE)) {
		fprintf(stderr, "%s: DONE is already high after PROGRAM_B\n",
			argv[0]);
		//exit(1);
	}

	/* raise program_b, and initb must go high, too */
	pio_set(PIO_SODR, PROGRAMB);
	for (i = 0; i < 50; i++) {
		ud(10);
		if (pio_get(INITB))
			break;
	}
	if (pio_get(INITB)) {
		fprintf(stderr, "%s: INIT_B is not going back high\n",
			argv[0]);
		//exit(1);
	}


	/* Then write one byte at a time */
	for (i = 0; i < bs_size; i++) {
		int byte = bstream[i];
		int bit;

		if (!(i & 1023)) {
				putchar('.'); fflush(stdout);
		}
		/* msb first */
		for (bit = 0x80; bit > 0; bit >>= 1) {
			/* data on rising edge of clock, which starts low */
			if (byte & bit)
				pio_set(PIO_SODR, TD0);
			else
				pio_set(PIO_CODR, TD0);

			/* so, rising edge, after it's stable */
			asm volatile("nop\n nop\n nop\n nop");
			pio_set(PIO_SODR, TK0);
			pio_set(PIO_CODR, TK0);
		}
		if (0 && /* don't do this check */ !pio_get(DONE)) {
			fprintf(stderr, "%s: DONE is already high after "
				"%i bytes (missing %i)\n", argv[0],
				i, bs_size - i);
			exit(1);
		}
	}
	putchar('\n');

	/* Then, wait a little and then check done must go high */
	for (i = 0; i < 500; i++) {
		ud(100);
		if (!pio_get(DONE))
			break;
	}
	if (pio_get(DONE)) {
		fprintf(stderr, "%s: DONE is not going high after %i bytes\n",
			argv[0], bs_size);
		exit(1);
	}

	/* PA5 must go low then high */
	pio_set(PIO_CODR, FPGA_RESET);
	ud(10);
	pio_set(PIO_SODR, FPGA_RESET);

	exit(0);
}
