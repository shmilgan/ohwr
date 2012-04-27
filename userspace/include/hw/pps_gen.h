/* PPS Generator driver */

#ifndef __PPS_GEN_H
#define __PPS_GEN_H

#include <stdio.h>
#include <inttypes.h>

int shw_pps_gen_init();
int shw_pps_gen_adjust_nsec(int32_t how_much);
int shw_pps_gen_adjust_utc(int64_t how_much);
int shw_pps_gen_busy();
int shw_pps_gen_enable_output(int enable);


#endif
