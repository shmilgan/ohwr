/*
	i2c_bitbang.h
	2012 CERN B.Bielawski 
*/

#ifndef I2C_CPU_BB_H
#define I2C_CPU_BB_H

#include <pio.h>
#include "i2c.h"

struct i2c_bitbang {
	pio_pin_t*		scl;
	pio_pin_t*		sda;
	int udelay;
	int timeout;
};

int		i2c_bitbang_init_bus(struct i2c_bus *bus);
int32_t		i2c_bitbang_transfer(struct i2c_bus* bus, uint32_t address,  uint32_t to_write, uint32_t to_read, uint8_t* data);
int32_t		i2c_bitbang_scan(struct i2c_bus* bus, uint32_t address);

#endif //I2C_CPU_BB_H
