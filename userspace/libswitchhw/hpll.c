/* Helper PLL driver */
/* To be removed and replaced by the SoftPLL in V3 */
#include <stdio.h>
#include  <string.h>
#include <inttypes.h>
#include <sys/time.h>

#include <hw/clkb_io.h>
#include <hw/switch_hw.h>

#define FREQ_ERROR_FRACBITS 7

/* Default values for some HPLL parameters:
   Phase detector gating = 16384 fbck cycles */
#define DEFAULT_PD_GATE_SEL HPLL_PD_GATE_16K 
/*  Frequency measure ment gating: 131072 fbck cycles */
#define DEFAULT_FD_GATE_SEL 3 

/* Lock detection thresholds */
#define DEFAULT_LD_SAMPLES 250
#define DEFAULT_LD_THRESHOLD 100

/* Macros for converting floating point PI gains to the fixed point format
   used internally by the HPLL */
#define FLOAT_TO_FB_COEF(x) ((int)((x) * (32.0 * 16.0 )))
#define FLOAT_TO_PB_COEF(x) ((int)((x) * (32.0 * 16.0 )))

static const hpll_params_t default_hpll_params = {
  20.0 , 8.0, // freq Ki/Kp
  0.256, 6.40, // target phase Ki/Kp
  100, // gain steps
  200000ULL, // gain step delay
  16384, // N
  1, // delta
  HPLL_FD_GATE_128K, // freq detector gating
  HPLL_PD_GATE_16K,   // phase detector gating
  HPLL_REFSEL_LOCAL, // reference clock select: local reference clock
  0, // don't stay in the freq mode after lock
  0, 0
};

/* current state of the HPLL */
static hpll_params_t cur_params;

/* Wrappers for accessing HPLL regs */
static inline uint32_t hpll_read(uint32_t reg)
{
  return shw_clkb_read_reg(CLKB_BASE_HPLL + reg);
}

static inline void hpll_write(uint32_t reg, uint32_t value)
{
  shw_clkb_write_reg(CLKB_BASE_HPLL + reg, value);
}


/* Linear interpolation at (step) step out of (nsteps) between (start) and (end) */
static double interpolate(double start, double end, int step, int nsteps)
{
	double k = (double)step / (double)(nsteps - 1);
	return start * (1.0-k) + k * end;
}


/* Ramps the HPLL gain to a new value. It doesn't like step changes int the coefficients,
   so they have to be slowly adjusted until they reach the required values. */
void shw_hpll_ramp_gain(double kp_new, double ki_new)
{
	uint64_t init_tics = shw_get_tics();
	int step = 0;

	cur_params.kp_phase = kp_new;
	cur_params.ki_phase = ki_new;


	for(step = 0; step < cur_params.phase_gain_steps; step++)
	{
				double kp, ki;

				kp = interpolate(cur_params.kp_phase_cur,cur_params.kp_phase, step, cur_params.phase_gain_steps);
				ki = interpolate(cur_params.ki_phase_cur,cur_params.ki_phase, step, cur_params.phase_gain_steps);

	      hpll_write(HPLL_REG_PBGR,
					 HPLL_PBGR_P_KP_W(FLOAT_TO_FB_COEF(kp)) |
					 HPLL_PBGR_P_KI_W(FLOAT_TO_FB_COEF(ki)));
				usleep(1000);
	}

	cur_params.kp_phase_cur = cur_params.kp_phase;
	cur_params.ki_phase_cur = cur_params.ki_phase;
}


/* Loads the state of the HPLL from (params) structure */
void shw_hpll_load_regs(const hpll_params_t *params)
{
  int target_freq_err;
  double freq_gating;

  hpll_write(HPLL_REG_PCR, 0); // disable the PLL

  hpll_write(HPLL_REG_DIVR,    // select the division ratio
	     HPLL_DIVR_DIV_FB_W(params->N + params->delta) |
	     HPLL_DIVR_DIV_REF_W(params->N));


  TRACE(TRACE_INFO,"DIV_REF = %d, DIV_FB = %d", params->N, params->N + params->delta);

  hpll_write(HPLL_REG_FBGR, // set frequency detector gain
	     HPLL_FBGR_F_KP_W(FLOAT_TO_FB_COEF(params->kp_freq)) |
	     HPLL_FBGR_F_KI_W(FLOAT_TO_FB_COEF(params->ki_freq)));

  TRACE(TRACE_INFO,"Freq: KP = %d, KI = %d", FLOAT_TO_FB_COEF(params->kp_freq), FLOAT_TO_FB_COEF(params->ki_freq));

  hpll_write(HPLL_REG_PBGR, // set phase detector gain (initial value)
	     HPLL_PBGR_P_KP_W(FLOAT_TO_FB_COEF(params->kp_phase)) |
	     HPLL_PBGR_P_KI_W(FLOAT_TO_FB_COEF(params->ki_phase)));

//	params->kp_phase_cur = params->kp_phase;
//	params->ki_phase_cur = params->ki_phase;

  TRACE(TRACE_INFO,"Phase: KP = %d, KI = %d", FLOAT_TO_FB_COEF(params->kp_phase), FLOAT_TO_FB_COEF(params->ki_phase));

  hpll_write(HPLL_REG_LDCR, // lock detection samples & threshold
	     HPLL_LDCR_LD_SAMP_W(DEFAULT_LD_SAMPLES) |
 	     HPLL_LDCR_LD_THR_W(DEFAULT_LD_THRESHOLD));

  TRACE(TRACE_INFO,"LockDet: SAMP = %d, THR = %d", DEFAULT_LD_SAMPLES, DEFAULT_LD_THRESHOLD);

  // calculate target frequency error

  freq_gating = (double) (1 << (params->freq_gating + 14));

  target_freq_err = (int) ((freq_gating - (freq_gating * (double)(params->N + params->delta) / (double)(params->N))) * (double)(1<<FREQ_ERROR_FRACBITS)) ;

  hpll_write(HPLL_REG_FBCR, // frequency branch control
	     HPLL_FBCR_FERR_SET_W(target_freq_err) | 									 HPLL_FBCR_FD_GATE_W(params->freq_gating));

  TRACE(TRACE_INFO,"TargetFreqErr = %d, FreqGating = %d PhaseGating = %d", target_freq_err, params->freq_gating, params->phase_gating);



  // enable the PLL, set DAC clock, reference clock, etc.

  hpll_write(HPLL_REG_PCR,
	     // 			 HPLL_PCR_FORCE_F |
	     HPLL_PCR_ENABLE |                          // enable the PLL
	     HPLL_PCR_SWRST |                           // force software reset
	     HPLL_PCR_PD_GATE_W(params->phase_gating) |  // phase detector gating
	     HPLL_PCR_DAC_CLKSEL_W(2) |                 // dac clock = system clock / 32
	     HPLL_PCR_REFSEL_W(params->ref_sel));        // reference select


  memcpy(&cur_params, params, sizeof(hpll_params_t));

  cur_params.kp_phase_cur = params->kp_phase;
  cur_params.ki_phase_cur = params->ki_phase;


  shw_hpll_ramp_gain(0.0384,   0.00384); /* fixme */
}

void shw_hpll_reset()
{
  uint32_t pcr_val = hpll_read(HPLL_REG_PCR);
  pcr_val |= HPLL_PCR_SWRST;
  hpll_write(HPLL_REG_PCR, pcr_val);
}

int shw_hpll_check_lock()
{
  uint32_t psr;

  psr= hpll_read(HPLL_REG_PSR);

  if(psr & HPLL_PSR_LOCK_LOST) {
    return 0;
    hpll_write(HPLL_REG_PSR,  HPLL_PSR_LOCK_LOST); // clear loss-of-lock bit
  }

  return (psr & HPLL_PSR_FREQ_LK) && (psr & HPLL_PSR_PHASE_LK);
}

int shw_hpll_get_divider()
{
	return cur_params.N;
}

int shw_hpll_init()
{
  TRACE(TRACE_INFO, "Initializing the DMTD Helper PLL");

  shw_hpll_load_regs(&default_hpll_params);

  while(!shw_hpll_check_lock()) usleep(100000);
}

/* Sets the HPLL reference input to network I/F (if_name) and re-initializes the HPLL. */
int shw_hpll_switch_reference(const char *if_name)
{
	hpll_params_t my_params;
  TRACE(TRACE_INFO, "HPLL: Set reference input to: %s", if_name);

 	memcpy(&my_params, &default_hpll_params, sizeof(hpll_params_t));

 	if(!strcmp(if_name, "wru0"))
 		my_params.ref_sel = HPLL_REFSEL_UP0_RBCLK;
 	else if(!strcmp(if_name, "wru1"))
 		my_params.ref_sel = HPLL_REFSEL_UP1_RBCLK;
 	else if(!strcmp(if_name, "local"))
 		my_params.ref_sel = HPLL_REFSEL_LOCAL;
  else
 	  TRACE(TRACE_FATAL, "unrecognized HPLL reference clock: %s", if_name);

  shw_hpll_load_regs(&my_params);
}
