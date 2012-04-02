/*
	i2c_cpu_bb.h
	2012 CERN B.Bielawski 
*/

#ifndef I2C_CPU_BB_H
#define I2C_CPU_BB_H


#include <hw/pio.h>
#include "i2c.h"

typedef struct
{
	pio_pin_t*		scl;
	pio_pin_t*		sda;
} i2c_cpu_bb_t;


i2c_bus_t*	i2c_cpu_bb_new_bus(pio_pin_t* scl, pio_pin_t* sda);
int32_t		i2c_cpu_bb_transfer(struct i2c_bus_t* bus, uint32_t address,  uint32_t to_write, uint32_t to_read, uint8_t* data);
int32_t		i2c_cpu_bb_scan(struct i2c_bus_t* bus, uint32_t address);

#endif //I2C_CPU_BB_H



