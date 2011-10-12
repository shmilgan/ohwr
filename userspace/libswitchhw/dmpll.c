/* FIXME: rewrite this _properly_ */
/* Just remove me from the code. V3 will use the SoftPLL. */

#include <stdio.h>
#include <inttypes.h>
#include <sys/time.h>
#include <signal.h>
#include <math.h>

#include <hw/switch_hw.h>
#include <hw/clkb_io.h>
#include <hw/dmpll_regs.h>

/* Indices of the DMPLL channels. The DMPLL can lock to Uplink 0/1 or to an external 125 MHz reference
   (never tested, used AD9516 locked to 10 MHz instead). */
#define DMPLL_CHANNEL_EXT_REF 2
#define DMPLL_CHANNEL_UP0 0
#define DMPLL_CHANNEL_UP1 1

/* Number of fractional bits of the PI controller gain coefficients. */
#define PI_FRACBITS 12 // was 16

/* PLL parameters */
typedef struct {
/* DMTD deglitcher threshold (see Tom's M.Sc. for explanation */
	int deglitch_threshold;

/* PLL loop bandwidth in Hz and damping factor. */
	double f_n;
	double eta;
	
/* Fixed-point PI gain coefficients (calculated from f_n and eta). */
	int ki;
	int kp;
	
/* Current reference channel */
	int channel;
	
/* Phase shifter setpoints for each reference channel */
	double phase_setpoint[4];
} dmpll_params_t;


static dmpll_params_t cur_state;

/* Calculates the gains of the PI controller for a given PLL bandwidth
   and damping factor. */
static void dmpll_calc_gain(double f_n, double eta, int *ki, int *kp)
{
	const double Kd = 2.6076e+03; /* phase detector gain */
	const double Kvco =   0.1816; /* VCO gain */
	const double Fs = 7.6294e+03; /* sampling freq (depends on the parameters of the Helper PLL */

	double omega_n = 2*M_PI*f_n;

	*kp = (int)((2*eta*omega_n/(Kd*Kvco)) * (double)(1<<PI_FRACBITS));
	*ki = (int)((omega_n*omega_n / (Kd*Kvco) / Fs) * (double)(1<<PI_FRACBITS));
	TRACE(TRACE_INFO,"kp = %d ki = %d\n", *kp, *ki);
}


/* Sets the deglitcher threshold on a particular channel to thr. */
static void dmpll_set_deglitch_threshold(uint32_t reg, int thr)
{
	TRACE(TRACE_INFO, "threshold %d", thr);
  shw_clkb_write_reg(CLKB_BASE_DMPLL + reg, DMPLL_DGCR_IN0_THR_LO_W(thr), DMPLL_DGCR_IN0_THR_HI_W(thr));

}

/* Sets the phase shfit on a given channel to ps_shift. */
static void dmpll_set_phase_shift(int channel, int ps_shift)
{
	switch(channel)
	{

	 case DMPLL_CHANNEL_UP0:
		shw_clkb_write_reg(CLKB_BASE_DMPLL + DMPLL_REG_PSCR_IN0, ps_shift);
		break;

	 case DMPLL_CHANNEL_UP1:
		shw_clkb_write_reg(CLKB_BASE_DMPLL + DMPLL_REG_PSCR_IN1, ps_shift);
		break;

	}
}

/* Checks if the DMPLL is locked. */
int shw_dmpll_check_lock()
{
  uint32_t psr;

  psr= shw_clkb_read_reg(CLKB_BASE_DMPLL + DMPLL_REG_PSR);

  if(psr & DMPLL_PSR_LOCK_LOST) {
/* Clear loss-of-lock bit so it can be updated again by the DMPLL */
    shw_clkb_write_reg(CLKB_BASE_DMPLL + DMPLL_REG_PSR,  DMPLL_PSR_LOCK_LOST); 
    return 0;
  }

//  printf("DPSR %x\n",psr & (DMPLL_PSR_FREQ_LK | DMPLL_PSR_PHASE_LK));
  return (psr & DMPLL_PSR_FREQ_LK) && (psr & DMPLL_PSR_PHASE_LK); 
}

/* Returns non-zero if the phase shifting operation is in progress (i.e. if the
   shifter has not reached the given setpoint yet. */
static int dmpll_shifter_busy(int channel)
{
	switch(channel)
	{

	 case DMPLL_CHANNEL_UP0:
		return shw_clkb_read_reg(CLKB_BASE_DMPLL + DMPLL_REG_PSCR_IN0) & DMPLL_PSCR_IN0_BUSY;

	 case DMPLL_CHANNEL_UP1:
		return shw_clkb_read_reg(CLKB_BASE_DMPLL + DMPLL_REG_PSCR_IN1) & DMPLL_PSCR_IN1_BUSY;
	}
 return 0;
}


/* Returns the index of the DMPLL channel corresponding to a given network interface. */
static int iface_to_channel(const char *source)
{
	if(!strcmp(source,"wru0"))
		return DMPLL_CHANNEL_UP0;
	else if(!strcmp(source,"wru1"))
		return DMPLL_CHANNEL_UP1;

	return -1;
}

/* Locks the DMPLL to a given reference source (network interface). */
int shw_dmpll_lock(const char *source)
{
	int ref_clk = iface_to_channel(source);

	if(ref_clk < 0) {
		TRACE(TRACE_ERROR, "Unknown clock source...");
		return -1;
	}

  TRACE(TRACE_INFO,"DMPLL: Set refence input to: %s", source);

/* Disable the DMPLL */
	shw_clkb_write_reg(CLKB_BASE_DMPLL + DMPLL_REG_PCR, 0);

/* Set the deglitcher thresholds on all channels (there's no independent setting) */
  dmpll_set_deglitch_threshold(DMPLL_REG_DGCR_IN0, cur_state.deglitch_threshold);
  dmpll_set_deglitch_threshold(DMPLL_REG_DGCR_IN1, cur_state.deglitch_threshold);
  dmpll_set_deglitch_threshold(DMPLL_REG_DGCR_FB,  cur_state.deglitch_threshold);

/* Reset the phase shifters. */  
  shw_clkb_write_reg(CLKB_BASE_DMPLL + DMPLL_REG_PSCR_IN0, 0);
  shw_clkb_write_reg(CLKB_BASE_DMPLL + DMPLL_REG_PSCR_IN1, 0);
  shw_clkb_write_reg(CLKB_BASE_DMPLL + DMPLL_REG_PSCR_IN2, 0);
  shw_clkb_write_reg(CLKB_BASE_DMPLL + DMPLL_REG_PSCR_IN3, 0);


/* Set the frequency branch gain (hardcoded - the freq branch is just to speed up the locking
   so these params don't really need to be phase noise optimized). */
  shw_clkb_write_reg(CLKB_BASE_DMPLL + DMPLL_REG_FBGR, DMPLL_FBGR_F_KP_W(-100) | DMPLL_FBGR_F_KI_W(-600));

/* Calculate the phase branch gains */
	dmpll_calc_gain(cur_state.f_n, cur_state.eta, &cur_state.ki, &cur_state.kp);

/* ... and write them to the PLL */
  shw_clkb_write_reg(CLKB_BASE_DMPLL + DMPLL_REG_PBGR, DMPLL_PBGR_P_KP_W(cur_state.kp) | DMPLL_PBGR_P_KI_W(cur_state.ki));

/* Set the lock detection thresholds, phase shifter speed and enable the channel. */
  shw_clkb_write_reg(CLKB_BASE_DMPLL + DMPLL_REG_LDCR, DMPLL_LDCR_LD_THR_W(2000) | DMPLL_LDCR_LD_SAMP_W(2000));
	shw_clkb_write_reg(CLKB_BASE_DMPLL + DMPLL_REG_PCR, DMPLL_PCR_ENABLE | DMPLL_PCR_REFSEL_W(ref_clk) | DMPLL_PCR_DAC_CLKSEL_W(2) | DMPLL_PCR_PS_SPEED_W(100) | DMPLL_PCR_SWRST);

	cur_state.channel = ref_clk;
	return 0;
}

/* Sets the phase shift between the PLL output clock (REFCLK) and the reference source 
   (Linux network I/F). The phase_shift is given in picoseconds. */
int shw_dmpll_phase_shift(const char *source, int phase_shift)
{
	int ref_clk = iface_to_channel(source);
	double ps;

	if(ref_clk < 0) return -1;

/* Convert to the internal timescale (1 REFCLK cycle = the helper PLL frequency offset / 1 REFCLK). */
	ps = (double)phase_shift / 8000.0 * (double) shw_hpll_get_divider();

	dmpll_set_phase_shift(ref_clk, (int) ps);
	return 0;
}

/* Returns 1 if the DMPLL phase shifter has not yet reached the given setpoint on network I/F (source). */
int shw_dmpll_shifter_busy(const char *source)
{
	int ref_clk = iface_to_channel(source);

	if(ref_clk < 0) return -1;
	int busy = dmpll_shifter_busy(ref_clk) ? 1 :0;

	return  busy;
}

int shw_dmpll_init()
{
	TRACE(TRACE_INFO,"Initializing DMTD main PLL...");

	/* DMPLL is disabled by default. Just make sure it loads the initial DAC value (middle of VCTCXO tuning range) */
	shw_clkb_write_reg(CLKB_BASE_DMPLL + DMPLL_REG_PCR, DMPLL_PCR_DAC_CLKSEL_W(2) | DMPLL_PCR_SWRST);

	/* Set default values for the bandwidth, damping and deglitcher parameters. */
	cur_state.f_n = 15.0; 
	cur_state.eta = 0.8; 
	cur_state.deglitch_threshold = 3000;
	
	return 0;
}
