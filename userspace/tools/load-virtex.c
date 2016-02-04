/*
 * Trivial gpio bit banger.
 * Alessandro Rubini, 2011, for CERN.
 * Turned into and spi user-space driver
 * Tomasz Wlostowski, 2012, for CERN.
 *
 * Released to the public domain.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <mach/at91sam9g45.h>

#include <mach/at91_pio.h>
#include <mach/at91_ssc.h>
#include <mach/at91_pmc.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

static unsigned char *bstream;

/* The address and size of the entire AT91 I/O reg space */
#define BASE_IOREGS 0xfff78000
#define SIZE_IOREGS 0x88000

enum {
	PIOA = 0,
	PIOB = 1,
	PIOC = 2,
	PIOD = 3,
	PIOE = 4
};

static void *ioregs;


#define AT91_PIOx(port) (AT91_PIOA + AT91_BASE_SYS + 0x200 * port)

/* macros to access 32-bit registers of various peripherals */
#define __PIO(port, regname) (*(volatile uint32_t *) \
  (ioregs + AT91_PIOx(port) - BASE_IOREGS + regname))

#define __SSC(regname) (*(volatile uint32_t *) \
  (ioregs + (AT91SAM9G45_BASE_SSC0 - BASE_IOREGS) + regname))

#define __PMC(regname) \
  (*(volatile uint32_t *)(ioregs + (AT91_BASE_SYS - BASE_IOREGS) + regname))

/* Missing SSC reg fields */
#define     AT91_SSC_CKO_DURING_XFER   (2 << 2)

/* This sets a bit. To clear output we set ODR (output disable register) */
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


static void ud(int usecs) /* horrible udelay thing without scheduling */
{
	struct timeval tv1, tv2;
	gettimeofday(&tv1, NULL);
	do
		gettimeofday(&tv2, NULL);
	while ((tv2.tv_sec - tv1.tv_sec) * 1000*1000
	       + tv2.tv_usec - tv1.tv_usec < usecs);
}


/* being a lazy bastard, I fork a process to avoid free, munmap etc */
static int load_fpga_child(char *fname)
{
	int bs_size, i;
	int fdmem;

	{
		/* read the bisstream file */
		FILE *f = fopen(fname, "r");

		if (!f) {
			fprintf(stderr, "%s: %s: %s\n", __func__, fname,
				strerror(errno));
			exit(1);
		}

		fseek(f, 0, SEEK_END);
		bs_size = ftell(f);
		fseek(f, 0, SEEK_SET);

		bstream = malloc(bs_size);

		if (bstream == NULL)
		{
			fprintf(stderr, "malloc failed\n");
			exit(1);
		}


		if (fread(bstream, 1, bs_size, f) != bs_size) {
			fprintf(stderr, "%s: read(%s): %s\n", __func__,
				fname, strerror(errno));
			exit(1);
		}

		fclose(f);
	}


	/* /dev/mem for mmap of both gpio and spi1 */
	if ((fdmem = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		fprintf(stderr, "%s: /dev/mem: %s\n", __func__,
			strerror(errno));
		exit(1);
	}

	/* map a whole page (4kB, but we called getpagesize to know it) */
	ioregs = mmap(0, SIZE_IOREGS, PROT_READ | PROT_WRITE,
		      MAP_SHARED, fdmem,
		      BASE_IOREGS);

	if (ioregs == MAP_FAILED) {
		fprintf(stderr, "%s: mmap(/dev/mem): %s\n", __func__,
			strerror(errno));
		exit(1);
	}


	/*
	 * all of this stuff is working on gpio so enable pio, out or in
	 */

	/* clock and data are output, connected to SSC, low by now */

	pio_set(PIO_PDR, TK0);
	pio_set(PIO_ASR, TK0);
	pio_set(PIO_OER, TK0);

	pio_set(PIO_PDR, TD0);
	pio_set(PIO_ASR, TD0);
	pio_set(PIO_OER, TD0);

	/* enable SSC controller clock */
	__PMC(AT91_PMC_PCER) = 1<<AT91SAM9G45_ID_SSC0;

	__SSC(AT91_SSC_CR) = AT91_SSC_SWRST;
	__SSC(AT91_SSC_CR) = 0;

	/* Config clock rate = MCK / 3 */
	__SSC(AT91_SSC_CMR) = 2 & AT91_SSC_CMR_DIV;

	/* Clock source = divided master clock,
	   active only during data transfer, extra 2 cycles of delay
	   between subsequent bytes */
	__SSC(AT91_SSC_TCMR) =
		AT91_SSC_CKS_DIV | AT91_SSC_CKO_DURING_XFER | (2<<16);

	/* 8 bits/xfer, MSB first */
	__SSC(AT91_SSC_TFMR) = (7 & AT91_SSC_DATALEN) | AT91_SSC_MSBF;
	__SSC(AT91_SSC_CR) = AT91_SSC_TXEN;

	/* fpga_reset is high */
	pio_set(PIO_PER, FPGA_RESET);
	pio_set(PIO_SODR, FPGA_RESET);
	pio_set(PIO_OER, FPGA_RESET);

	/* program_b is output high: this is pulsed low to start programming */
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
			__func__);
		//exit(1);
	}

	if (pio_get(DONE)) {
		fprintf(stderr, "%s: DONE is already high after PROGRAM_B\n",
			__func__);
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
			__func__);
		//exit(1);
	}

	ud(1000); /* wait for a short while before commencing the configuration - otherwise
		     the FPGA might not assert the DONE flag correctly */
	/* Then write one byte at a time */

	printf("Booting FPGA:");
	for (i = 0; i < bs_size; i++) {
		if (!(i & 32767)) {
			putchar('.');
			fflush(stdout);
		}

		while(! (__SSC(AT91_SSC_SR) & AT91_SSC_TXEMPTY))
			;

		__SSC(AT91_SSC_THR) = bstream[i];

		if (pio_get(DONE)) {
			fprintf(stderr, "%s: DONE is already high after "
				"%i bytes (missing %i)\n", __func__,
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
			__func__, bs_size);
		exit(1);
	}
	printf("FPGA image loaded\n");

	/* PA5 must go low then high */
	pio_set(PIO_CODR, FPGA_RESET);
	ud(10);
	pio_set(PIO_SODR, FPGA_RESET);
	exit(0);
}

int load_fpga_main(char *fname)
{
	int pid = fork();
	int status;

	switch(pid) {
	case -1:
		fprintf(stderr, "fork(): %s\n", strerror(errno));
		return -1;
	case 0: /* child */
		if (load_fpga_child(fname))
			exit(1);
		exit(0);
	default: /* parent */
		waitpid(pid, &status, 0);
		/* check if exited normally and returned 0 */
		if (WIFEXITED(status)) {
			if (!WEXITSTATUS(status)) {
				return 0; /* success */
			} else {
				/* process returned not 0 */
				fprintf(stderr, "load-virtex: Error, child "
					"process returned %d\n",
					WEXITSTATUS(status));
				return -1; /* fail */
			}
		} else {
			/* Child process terminated abnormally */
			fprintf(stderr, "load-virtex: Child process terminated"
					" abnormally\n");
			return -1; /* fail */
		}
	}
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Use: \"%s <filename>\"\n", argv[0]);
	}
	return load_fpga_main(argv[1]);
}
