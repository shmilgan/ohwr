#ifndef __PHY_CALIBRATION_H
#define __PHY_CALIBRATION_H


#define PHY_CALIBRATE_TX 0
#define PHY_CALIBRATE_RX 1

int shw_cal_init();
int shw_cal_enable_feedback(const char *if_name, int enable, int lane);
int shw_cal_measure(uint32_t *phase); // picoseconds!
int shw_cal_enable_pattern(const char *if_name, int enable);
int shw_poll_dmtd(const char *if_name, uint32_t *phase_ps);


void xpoint_cal_feedback(int on, int port, int txrx);
int xpoint_configure();

#endif


