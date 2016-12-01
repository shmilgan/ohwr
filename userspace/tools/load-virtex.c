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

#include <libwr/util.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

static unsigned char *bstream;


/* FIXME definitions from kernel 2.6.39  - read commit message */
#define AT91SAM9G45_PERIPH (0xFFF78000)
#define AT91SAM9G45_SSC0 (0xFFF9C000)
#define AT91_SSC_CR 0x00
#define   AT91_SSC_RXEN (1 <<  0) /* Receive Enable */
#define   AT91_SSC_RXDIS (1 <<  1) /* Receive Disable */
#define   AT91_SSC_TXEN (1 <<  8) /* Transmit Enable */
#define   AT91_SSC_TXDIS (1 <<  9) /* Transmit Disable */
#define   AT91_SSC_SWRST (1 << 15) /* Software Reset */
#define AT91_SSC_CMR 0x04 /* Clock Mode Register */
#define   AT91_SSC_CMR_DIV (0xfff << 0) /* Clock Divider */
#define AT91_SSC_RCMR 0x10 /* Receive Clock Mode Register */
#define   AT91_SSC_CKS (3 <<  0) /* Clock Selection */
#define      AT91_SSC_CKS_DIV (0 << 0)
#define      AT91_SSC_CKS_CLOCK (1 << 0)
#define      AT91_SSC_CKS_PIN (2 << 0)

#define AT91_SSC_RFMR 0x14 /* Receive Frame Mode Register */
#define   AT91_SSC_DATALEN (0x1f <<  0) /* Data Length */
#define   AT91_SSC_LOOP (1    <<  5) /* Loop Mode */
#define   AT91_SSC_MSBF (1    <<  7) /* Most Significant Bit First */

#define AT91_SSC_TCMR 0x18 /* Transmit Clock Mode Register */
#define AT91_SSC_TFMR 0x1c /* Transmit Fram Mode Register */
#define   AT91_SSC_DATDEF (1 <<  5) /* Data Default Value */
#define   AT91_SSC_FSDEN (1 << 23) /* Frame Sync Data Enable */
#define AT91_SSC_THR 0x24 /* Transmit Holding Register */
#define AT91_SSC_SR 0x40 /* Status Register */
#define   AT91_SSC_TXRDY (1 <<  0) /* Transmit Ready */
#define   AT91_SSC_TXEMPTY (1 <<  1) /* Transmit Empty */


#define AT91_SYS (0xFFFFE200)
#define   AT91_SYS_PIOA (0x1000)
#define AT91_SYS_PMC (0x1A00)
#define   AT91_SYS_PMC_PCER (0x10)

/* The address and size of the entire AT91 I/O reg space */
#define BASE_IOREGS AT91SAM9G45_PERIPH
#define SIZE_IOREGS 0x88000

enum {
	PIOA = 0,
	PIOB = 1,
	PIOC = 2,
	PIOD = 3,
	PIOE = 4
};

static void *ioregs;

#define __PERIPH_FIXUP(__addr) (ioregs - BASE_IOREGS + __addr)
#define AT91_PIOx(port) (AT91_SYS + AT91_SYS_PIOA + 0x200 * port)

/* macros to access 32-bit registers of various peripherals */
#define __PIO_ADDR(port, regname) (AT91_PIOx(port) + regname)
#define __SSC_ADDR(regname) (AT91SAM9G45_SSC0 + regname)
#define __PMC_ADDR(regname) (AT91_SYS + AT91_SYS_PMC + regname)


#define __PIO(port, regname) (*(volatile uint32_t *)			\
			      (__PERIPH_FIXUP(__PIO_ADDR(port, regname))))
#define __SSC(regname) (*(volatile uint32_t *)(__PERIPH_FIXUP(__SSC_ADDR(regname))))
#define __PMC(regname) \
	(*(volatile uint32_t *)(__PERIPH_FIXUP(__PMC_ADDR(regname))))

/* Missing SSC reg fields */
#define     AT91_SSC_CKO_DURING_XFER   (2 << 2)

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
	__PMC(AT91_SYS_PMC_PCER) = (1 << AT91SAM9G45_ID_SSC0);

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
	shw_udelay(100*1000);
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
		shw_udelay(10);
		if (!pio_get(INITB))
			break;
	}

	/* check status: initb must be low and done must be low */
	if (pio_get(INITB)) {
		fprintf(stderr, "%s: INIT_B is still high after PROGRAM_B\n",
			__func__);
		//exit(1);
	}

	if (!pio_get(DONE)) {
		fprintf(stderr, "%s: DONE is already high after PROGRAM_B\n",
			__func__);
		//exit(1);
	}

	/* raise program_b, and initb must go high, too */
	pio_set(PIO_SODR, PROGRAMB);
	for (i = 0; i < 50; i++) {
		shw_udelay(10);
		if (pio_get(INITB))
			break;
	}

	if (pio_get(INITB)) {
		fprintf(stderr, "%s: INIT_B is not going back high\n",
			__func__);
		//exit(1);
	}

	shw_udelay(1000); /* wait for a short while before commencing the configuration - otherwise
		     the FPGA might not assert the DONE flag correctly */
	/* Then write one byte at a time */

	printf("Booting FPGA: ");
	for (i = 0; i < bs_size; i++) {
		if (!(i & 32767)) {
			putchar('.');
			fflush(stdout);
		}

		while(! (__SSC(AT91_SSC_SR) & AT91_SSC_TXEMPTY))
			;

		__SSC(AT91_SSC_THR) = bstream[i];

		if (0 && /* don't do this check */ !pio_get(DONE)) {
			fprintf(stderr, "%s: DONE is already high after "
				"%i bytes (missing %i)\n", __func__,
				i, bs_size - i);
			exit(1);
		}
	}
	putchar('\n');

	/* Then, wait a little and then check done must go high */
	for (i = 0; i < 500; i++) {
		shw_udelay(100);
		if (!pio_get(DONE))
			break;
	}
	if (pio_get(DONE)) {
		fprintf(stderr, "%s: DONE is not going high after %i bytes\n",
			__func__, bs_size);
		exit(1);
	}

	/* PA5 must go low then high */
	pio_set(PIO_CODR, FPGA_RESET);
	shw_udelay(10);
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
