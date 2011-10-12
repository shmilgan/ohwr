/* PPS Generator driver */

#include <stdio.h>
#include  <string.h>
#include <inttypes.h>
#include <sys/time.h>

#include <hw/clkb_io.h>
#include <hw/pps_gen_regs.h>
#include <hw/switch_hw.h>
#include <hw/trace.h>

/* Default width (in 8ns units) of the pulses on the PPS output */
#define PPS_WIDTH 100000

int shw_pps_gen_init()
{
	uint32_t cr;

	cr = PPSG_CR_CNT_EN | PPSG_CR_PWIDTH_W(PPS_WIDTH);
  TRACE(TRACE_INFO, "Initializing PPS generator...");

  _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_CR, cr);

  _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_ADJ_UTCLO, 1285700840);
  _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_ADJ_UTCHI, 0);
  _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_ADJ_NSEC, 0);

  _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_CR, cr | PPSG_CR_CNT_SET);
  _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_CR, cr);
}

/* Adjusts the nanosecond (125 MHz refclk cycle) counter by atomically adding (how_much). */
int shw_pps_gen_adjust_nsec(int32_t how_much)
{
  uint32_t cr;

  TRACE(TRACE_INFO, "AdjustPPS: %d nanoseconds", how_much);
  _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_ADJ_UTCLO, 0);
  _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_ADJ_UTCHI, 0);

  _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_ADJ_NSEC, ( how_much / 8 ));

  _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_CR,   PPSG_CR_CNT_EN | PPSG_CR_PWIDTH_W(PPS_WIDTH) | PPSG_CR_CNT_ADJ);
}

/* Adjusts the seconds (UTC/TAI) counter by adding (how_much) in an atomic way */
int shw_pps_gen_adjust_utc(int64_t how_much)
{
  uint32_t cr;

  TRACE(TRACE_INFO, "AdjustUTC: %lld seconds", how_much);
  _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_ADJ_UTCLO, (uint32_t) (how_much & 0xffffffffLL));
  _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_ADJ_UTCHI, (uint32_t) (how_much >> 32));
  _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_ADJ_NSEC, 0);
  _fpga_writel(FPGA_BASE_PPS_GEN + PPSG_REG_CR,   PPSG_CR_CNT_EN | PPSG_CR_PWIDTH_W(PPS_WIDTH) | PPSG_CR_CNT_ADJ);
}

/* Returns 1 when the adjustment operation is not yet finished */
int shw_pps_gen_busy()
{
  return _fpga_readl(FPGA_BASE_PPS_GEN + PPSG_REG_CR) & PPSG_CR_CNT_ADJ ? 0 : 1;
}
