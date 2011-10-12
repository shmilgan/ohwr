/* Routines for accessing the timing FPGA */

#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>

#include <hw/switch_hw.h>
#include <hw/trace.h>
#include <hw/clkb_io.h>
#include <hw/pio.h>

#define SPI_CLKDIV_VAL 50
#define CLKB_IDCODE 0xdeadbeef

/* Some protos */
int shw_clkb_write_reg(uint32_t reg, uint32_t val);
uint32_t shw_clkb_read_reg(uint32_t reg);

/* Initializes the SPI link between the main FPGA and the timing FPGA. 
	 clkdiv = SPI SCLK division ratio. */
static void cmi_init(int clkdiv)
{
  uint32_t val = SPICTL_ENABLE(1) | SPICTL_CLKDIV(clkdiv);
  _fpga_writel(FPGA_BASE_SPIM+SPI_REG_SPICTL, val);
}

/* Returns non-zero if the SPI controller is busy sending/receiving data */
static inline int cmi_busy()
{
  uint32_t ctl;
  ctl = _fpga_readl(FPGA_BASE_SPIM+SPI_REG_SPICTL);
  return SPICTL_BUSY(ctl);
}

/* Enables (cs=1) or disables (cs=1) the SPI chip-select line */
static inline int cmi_cs(int cs)
{
  uint32_t ctl;
  ctl = _fpga_readl(FPGA_BASE_SPIM+SPI_REG_SPICTL);
  if(cs) ctl |= SPICTL_CSEN(1);
  else ctl |= SPICTL_CSDIS(1);
  _fpga_writel(FPGA_BASE_SPIM+SPI_REG_SPICTL, ctl);
}

/* Sends/receives a number (n) of 32-bit words (d_in) to/from the timing
   FPGA. Received words are stored in (d_out). */
static void cmi_transfer(uint32_t *d_in, uint32_t *d_out, int n)
{
  int i;
  uint32_t ctl, tmp;

  while(cmi_busy());

  cmi_cs(1);

  for(i=0;i<n;i++)
  {
	_fpga_writel(FPGA_BASE_SPIM+SPI_REG_SPITX, d_in[i]);

    while(cmi_busy());

    tmp = _fpga_readl(FPGA_BASE_SPIM+SPI_REG_SPIRX);
    if(d_out) d_out[i] = tmp;
  }

  cmi_cs(0);
}

/* Initializes the Inter-FPGA link (just the link, not the timing FPGA). */
int shw_clkb_init_cmi()
{
	TRACE(TRACE_INFO, "initializing Clocking Mezzanine Interface");
  cmi_init(SPI_CLKDIV_VAL);
	return 0;
}


/* Initializes the Timing FPGA. Currently the initialization is just 
   resetting the chip. */
int shw_clkb_init()
{
    shw_pio_configure(PIN_clkb_fpga_nrst);
    shw_pio_set0(PIN_clkb_fpga_nrst);
    shw_udelay(1000);
    shw_pio_set1(PIN_clkb_fpga_nrst);
    shw_udelay(1000);
}

/* Writes a 32-bit value (val) to the register (reg) in the Timing FPGA.
   Returns 0 on success. */
int shw_clkb_write_reg(uint32_t reg, uint32_t val)
{
  uint32_t rx[2], tx[2];
  tx[0] = (reg >> 2) & 0x7fffffff;
  tx[1] = val;
  cmi_transfer(tx,rx,2);

  if(rx[1]!=0xaa) {
    TRACE(TRACE_ERROR, "invalid ack (0x%08x)", rx[1]);
	return -1;
  }
  return 0;
}

/* Reads the value of a 32-bit register (reg) from the Timing FPGA and
   returns it. */
uint32_t shw_clkb_read_reg(uint32_t reg)
{
  uint32_t rx[2], tx[2];

  tx[0]=(reg >> 2) | 0x80000000;
  tx[1]= 0xffffffff;
  cmi_transfer(tx,rx,2);
  return rx[1];
}
