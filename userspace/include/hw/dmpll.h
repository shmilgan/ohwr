/* FIXME: rewrite this _properly_ */

#ifndef __DMPLL_H
#define __DMPLL_H

#include <hw/dmpll_regs.h>

#define DMPLL_CHANNEL_EXT_REF 2
#define DMPLL_CHANNEL_UP0 0
#define DMPLL_CHANNEL_UP1 1

int shw_dmpll_init();
int shw_dmpll_check_lock();
int shw_dmpll_lock(const char *source);
int shw_dmpll_phase_shift(const char *source, int phase_shift);
int shw_dmpll_shifter_busy(const char *source);

typedef struct {
  int deglitch_threshold;
  double f_n;
  double eta;
  int ki;
  int kp;
  int channel;
  double phase_setpoint[4];
} dmpll_params_t;
              

#endif
