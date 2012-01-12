/*
 * Trivial pll programmer using an spi controoler.
 * PLL is AD9516, SPI is opencores
 * Tomasz Wlostowski, Alessandro Rubini, 2011, for CERN.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

#define WRS3_SPI_BASE 0x10000000 /* FIXME */

static void ud(int usecs) /* horrible udelay thing without scheduling */
{
	struct timeval tv1, tv2;
	gettimeofday(&tv1, NULL);
	do
		gettimeofday(&tv2, NULL);
	while ((tv2.tv_sec - tv1.tv_sec) * 1000*1000
	       + tv2.tv_usec - tv1.tv_usec < usecs);
}

/*
 * SPI stuff, used by later code
 */

#define SPI_REG_RX0	0
#define SPI_REG_TX0	0
#define SPI_REG_RX1	4
#define SPI_REG_TX1	4
#define SPI_REG_RX2	8
#define SPI_REG_TX2	8
#define SPI_REG_RX3	12
#define SPI_REG_TX3	12

#define SPI_REG_CTRL	16
#define SPI_REG_DIVIDER	20
#define SPI_REG_SS	24

#define SPI_CTRL_ASS		(1<<13)
#define SPI_CTRL_IE		(1<<12)
#define SPI_CTRL_LSB		(1<<11)
#define SPI_CTRL_TXNEG		(1<<10)
#define SPI_CTRL_RXNEG		(1<<9)
#define SPI_CTRL_GO_BSY		(1<<8)
#define SPI_CTRL_CHAR_LEN(x)	((x) & 0x7f)

static int oc_spi_base;

static inline void ocspi_write(int addr, uint32_t val)
{
	if (!oc_spi_base)
		return;
	*(uint32_t *)(oc_spi_base + addr) = val;
	printf("%08x := %08x\n", oc_spi_base + addr, val);
}

static inline uint32_t ocspi_read(int addr)
{
	uint32_t val;
	if (!oc_spi_base)
		return -1;
	val =*(uint32_t *)(oc_spi_base + addr);
	printf("%08x  = %08x\n", oc_spi_base + addr, val);
	return val;
}

int oc_spi_init(char *prgname, uint32_t base_addr)
{
	int pagesize = getpagesize(); /* can't fail */
	int fdmem;
	void *addr;

	/* /dev/mem for mmap of both gpio and spi1 */
	if ((fdmem = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		fprintf(stderr, "%s: /dev/mem: %s\n",
			prgname, strerror(errno));
		return -1;
	}

	/* map a whole page (4kB, but we called getpagesize to know it) */
	addr = mmap(0, pagesize, PROT_READ | PROT_WRITE,
		       MAP_SHARED, fdmem,
		       base_addr & (~(pagesize-1)));
	if (addr == MAP_FAILED) {
		fprintf(stderr, "%s: mmap(/dev/mem): %s\n",
			prgname, strerror(errno));
		return -1;
	}
	close(fdmem);
	oc_spi_base = (int)addr + (base_addr & (pagesize-1));
	ocspi_write(SPI_REG_DIVIDER, 1000);
	return 0;
}

int oc_spi_txrx(int ss, int nbits, uint32_t in, uint32_t *out)
{
	uint32_t rval;

	if (!out)
		out = &rval;

	ocspi_write( SPI_REG_CTRL,
		     SPI_CTRL_ASS | SPI_CTRL_CHAR_LEN(nbits)
		     | SPI_CTRL_TXNEG);
	ocspi_write( SPI_REG_TX0, in);
	ocspi_write( SPI_REG_SS, (1 << ss));
	ocspi_write( SPI_REG_CTRL,
		     SPI_CTRL_ASS | SPI_CTRL_CHAR_LEN(nbits)
		     | SPI_CTRL_TXNEG | SPI_CTRL_GO_BSY);

	while(ocspi_read(SPI_REG_CTRL) & SPI_CTRL_GO_BSY)
		;

	*out = ocspi_read(SPI_REG_RX0);
	return 0;
}

#define CS_PLL	0 /* AD9516 on SPI CS0 */

/*
 * AD9516 stuff, using SPI, used by later code.
 * "reg" is 12 bits, "val" is 8 bits, but both are better used as int
 */

static void ad9516_write_reg(int reg, int val)
{
	oc_spi_txrx(CS_PLL, 24, (reg << 8) | val, NULL);
}

static int ad9516_read_reg(int reg)
{
	uint32_t rval;
	oc_spi_txrx(CS_PLL, 24, (reg << 8) | (1 << 23), &rval);
	return rval & 0xff;
}

/* FIXME: table of registers .... */
struct adregval {
	int reg;
	int val;
};

static struct adregval ad9516_regs[0];


static int ad9516_init(FILE *fout)
{
	int i;

	if (fout)
		fprintf(fout, "Initializing AD9516 PLL...\n");

	ad9516_write_reg(0x000, 0x99);
	ad9516_write_reg(0x232, 0x01);

	if (ad9516_read_reg(0x3) != 0xc3) {
		fprintf(stderr,"Error: AD9516 PLL not responding.\n");
		return -1;
	}

	for (i=0; i < ARRAY_SIZE(ad9516_regs); i++)
		ad9516_write_reg (ad9516_regs[i].reg, ad9516_regs[i].val);

	while ((ad9516_read_reg(0x1f) & 1) == 0)
		ud(100);

	// sync channels
	ad9516_write_reg(0x230, 1);
	ad9516_write_reg(0x232, 1);
	ad9516_write_reg(0x230, 0);
	ad9516_write_reg(0x232, 1);

	if (fout)
		fprintf(fout, "AD9516 locked.\n");

	return 0;
}

/*
 * That's it. Here's our main function
 */

int main(int argc, char **argv)
{
	if (oc_spi_init(argv[0], WRS3_SPI_BASE) < 0)
		exit(1);

	if (ad9516_init(stdout) < 0)
		exit(1);

	/* Wow! Nothing to do! */
	exit(0);
}
