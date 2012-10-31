#ifndef __FAN_H
#define __FAN_H

#define SHW_FAN_UPDATETO_DEFAULT 5


int shw_init_fans();
void shw_update_fans();

void shw_pwm_speed(int enable_mask, float rate);
void shw_pwm_update_timeout(int tout_100ms);

int shw_init_i2c_sensors();
float tmp100_read_temp(int dev_addr);
void tmp100_write_reg(int dev_addr, uint8_t reg_addr, uint8_t value);
float tmp100_read_temp(int dev_addr);

#endif
