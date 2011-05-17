// definitions for DMTD helper PLL

#ifndef __HPLL_H
#define __HPLL_H

#include <inttypes.h>
#include <hw/hpll_regs.h>

// Phase detector gating:
#define HPLL_PD_GATE_512 0
#define HPLL_PD_GATE_1K 1
#define HPLL_PD_GATE_2K 2
#define HPLL_PD_GATE_4K 3
#define HPLL_PD_GATE_8K 4
#define HPLL_PD_GATE_16K 5
#define HPLL_PD_GATE_32K 6
#define HPLL_PD_GATE_64K 7

// Frequency detector gating:
#define HPLL_FD_GATE_16K 0
#define HPLL_FD_GATE_32K 1
#define HPLL_FD_GATE_64K 2
#define HPLL_FD_GATE_128K 3
#define HPLL_FD_GATE_256K 4
#define HPLL_FD_GATE_512K 5
#define HPLL_FD_GATE_1M 6
#define HPLL_FD_GATE_2M 7

#define HPLL_REFSEL_UP0_RBCLK 2
#define HPLL_REFSEL_UP1_RBCLK 1
#define HPLL_REFSEL_LOCAL 0


typedef struct {
  float ki_freq, kp_freq;		 // Kp/Ki for the frequency branch
  float ki_phase, kp_phase;  // Phase gain (target)

  int phase_gain_steps;			 // number of phase gain (start to end) transition steps
  uint64_t phase_gain_step_delay;        // step delay for phase gain adjustment (in microseconds)

  int N, delta;                          // divider settings: output_freq = input_freq * (N / (N+delta))

  int freq_gating;                       // frequency detector gating
  int phase_gating;                      // phase detector gating

  int ref_sel;                           // reference clock select
  int force_freqmode;                    // force frequency lock mode

	double ki_phase_cur, kp_phase_cur; // Phase gain (current)

} hpll_params_t;

int shw_hpll_init();
//void shw_hpll_update();
int shw_hpll_check_lock();
//void shw_hpll_set_reference(int ref_clk);
void shw_hpll_reset();
void shw_hpll_load_regs(const hpll_params_t *params);
int shw_hpll_switch_reference(const char *if_name);
int shw_hpll_get_divider();

//void shw_hpll_start_recording(uint32_t *buffer, int params);

#endif
