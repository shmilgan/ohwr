#ifndef __WATCHDOG_H
#define __WATCHDOG_H

#include <stdio.h>

#include <hw/switch_hw.h>
#include <hw/pio.h>

#define MBL_LED_LINK 0
#define MBL_LED_ACT 1
 
#define MBL_LED_OFF 0
#define MBL_LED_ON 1
#define MBL_LED_BLINK_SLOW 2
#define MBL_LED_BLINK_FAST 3
   
#define MBL_FEEDBACK_TX 1
#define MBL_FEEDBACK_RX 2
#define MBL_FEEDBACK_OFF 3

void shw_mbl_set_leds(int port, int led, int mode);
void shw_mbl_cal_feedback(int port, int cmd);
int shw_watchdog_init();


#endif
