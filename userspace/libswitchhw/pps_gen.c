/* PPS Generator driver */

#include <stdio.h>
#include  <string.h>
#include <inttypes.h>
#include <sys/time.h>

#include <hw/fpga_regs.h>
#include <hw/pps_gen_regs.h>

#include <switch_hw.h>
#include <trace.h>

/* Default width (in 8ns units) of the pulses on the PPS output */
#define PPS_WIDTH 100000

int shw_pps_gen_init()
{
	uint32_t cr;

	cr = PPSG_CR_CNT_EN | PPSG_CR_PWIDTH_W(PPS_WIDTH);
  TRACE(TRACE_INFO, "Initializing PPS generator...");

  _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_CR, cr);

  _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_ADJ_SECLO, 0);
  _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_ADJ_SECHI, 0);
  _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_ADJ_NSEC, 0);

  _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_CR, cr | PPSG_CR_CNT_SET);
  _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_CR, cr);
  _fpga_writel(FPGA_BASE_PPS_GEN + 0x1c, 0x6); /* enable PPS output */
}

/* Adjusts the nanosecond (refclk cycle) counter by atomically adding (how_much) cycles. */
int shw_pps_gen_adjust(int counter, int64_t how_much)
{
  uint32_t cr;

  TRACE(TRACE_INFO, "Adjust: counter = %s [%c%lld]", 
  	counter == PPSG_ADJUST_SEC ? "seconds" : "nanoseconds", how_much<0?'-':'+', abs(how_much));

	if(counter == PPSG_ADJUST_NSEC)
	{
 		_fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_ADJ_SECLO, 0);
  	_fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_ADJ_SECHI, 0);
		_fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_ADJ_NSEC, how_much);
	} else {
 		_fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_ADJ_SECLO, (uint32_t ) (how_much & 0xffffffffLL));
  	_fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_ADJ_SECHI, (uint32_t ) (how_much >> 32) & 0xff);
		_fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_ADJ_NSEC, 0);
	}

  _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_CR, _fpga_readl(FPGA_BASE_PPS_GEN + PPSG_REG_CR) | PPSG_CR_CNT_ADJ);
	return 0;
}

/* Returns 1 when the adjustment operation is not yet finished */
int shw_pps_gen_busy()
{
	uint32_t cr = _fpga_readl(FPGA_BASE_PPS_GEN + PPSG_REG_CR);
  return cr& PPSG_CR_CNT_ADJ ? 0 : 1;
}

/* Enables/disables PPS output */
int shw_pps_gen_enable_output(int enable)
{
    uint32_t escr = _fpga_readl(FPGA_BASE_PPS_GEN + PPSG_REG_ESCR);
    if(enable)
        _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_ESCR, escr | PPSG_ESCR_PPS_VALID)
    else
        _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_ESCR, escr & ~PPSG_ESCR_PPS_VALID);

    return 0;
}

void shw_pps_gen_read_time(uint64_t *seconds, uint32_t *nanoseconds)
{
	uint32_t ns_cnt;
	uint64_t sec1, sec2;
	
	do {
		sec1 = (uint64_t)_fpga_readl(FPGA_BASE_PPS_GEN + PPSG_REG_CNTR_SECLO) | (uint64_t)_fpga_readl(FPGA_BASE_PPS_GEN + PPSG_REG_CNTR_SECHI) << 32;
		ns_cnt = _fpga_readl(FPGA_BASE_PPS_GEN + PPSG_REG_CNTR_NSEC);
		sec2 = (uint64_t)_fpga_readl(FPGA_BASE_PPS_GEN + PPSG_REG_CNTR_SECLO) | (uint64_t)_fpga_readl(FPGA_BASE_PPS_GEN + PPSG_REG_CNTR_SECHI) << 32;
	}	while(sec2 != sec1);

	if(seconds) *seconds = sec2;
	if(nanoseconds) *nanoseconds = ns_cnt;
}