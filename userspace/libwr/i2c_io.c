/*
 * i2c_cpu.c
 *
 * Access to the PCA9554PW chip to retrieve scb version and status LED.
 *
 *  Created on: 20 Jan 2013
 *  Authors:
 * 		- Benoit RAT
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License...
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <libwr/pio.h>
#include <trace.h>

#include "i2c.h"
#include "i2c_io.h"
#include "i2c_bitbang.h"
#include "libshw_i2c.h"

#define ARRAY_SIZE(a)                               \
  (sizeof(a) / sizeof(*(a)))

#define I2C_SCB_VER_ADDR 0x20


//Connected to miniBP to PB6>PB4>PB24
pio_pin_t wr_i2c_io_sda = {
	.port = PIOB,
	.pin = 24,
	.mode = PIO_MODE_GPIO, //PullUp by i2c when miniBP >v3.3, PullDown in miniBP v3.2
	.dir = PIO_OUT_0,
};
//Connected to miniBP to PB7>PB0>PB20
pio_pin_t wr_i2c_io_scl = {
	.port = PIOB,
	.pin = 20,
	.mode = PIO_MODE_GPIO,	//PullUp by i2c when miniBP >v3.3, PullDown in miniBP v3.2
	.dir = PIO_OUT_0,
};
struct i2c_bitbang wr_i2c_io_reg = {
	.scl = &wr_i2c_io_scl,
	.sda = &wr_i2c_io_sda,
};


struct i2c_bus i2c_io_bus = {
		.name = "wr_scb_ver",
		.type = I2C_TYPE_BITBANG,
		.type_specific = &wr_i2c_io_reg,
		.err  = I2C_NULL_PARAM,
	};


int shw_i2c_io_init(void)
{
	TRACE(TRACE_INFO, "Initializing IO I2C bus...%s",__TIME__);
	if (i2c_init_bus(&i2c_io_bus) < 0) {
		TRACE(TRACE_ERROR,"init failed: %s", i2c_io_bus.name);
		return -1;
	}

	TRACE(TRACE_INFO,"init: success: %s", i2c_io_bus.name);
	return 0;
}



int shw_i2c_io_scan(uint8_t *dev_map)
{
	int i;
	int detect;


	if (i2c_io_bus.err)
			return -1;

	detect  = i2c_scan(&i2c_io_bus, dev_map);
	printf("\ni2c_bus: %s: %d devices\n", i2c_io_bus.name, detect);
	for (i = 0; i < 128; i++)
		if (dev_map[i/8] & (1 << (i%8)))
			printf("device at: 0x%02X\n", i);

	return detect;
}


int shw_get_hw_ver()
{
	uint8_t ret;
	struct i2c_bus *bus= &i2c_io_bus;

	//Check if i2c module exists (>=3.3)
	if(bus && bus->scan(bus,I2C_SCB_VER_ADDR))
	{
		//The 0b00001110 bits are used for SCB HW version
		ret= wrswhw_pca9554_get_input(bus,I2C_SCB_VER_ADDR);
		switch((ret >> 1) & 0x7)
		{
			case 0: return 330;
			case 1: return 340; //version is not available
			case 2: return 341;
			default:
				TRACE(TRACE_FATAL,"Unknown HW version (0x%x), check the DIP switch under the SCB",(ret >> 1) & 0x7);
				return -1;


		}

	}
	else
	{
		return 320;

	}
}

uint8_t shw_get_fpga_type()
{
	struct i2c_bus *bus= &i2c_io_bus;

	if(bus && bus->scan(bus,I2C_SCB_VER_ADDR))
	{
	//The 0b00000001 bit is used for FPGA type
		if(wrswhw_pca9554_get_input(bus,I2C_SCB_VER_ADDR) & 0x1)
			return SHW_FPGA_LX240T;
		else
			return SHW_FPGA_LX130T;
	}
	//HACK: Check if file exists. This enable v3.2 miniBP and v3.3 SCB with LX240T
	if(access("/wr/etc/lx240t.conf", F_OK) == 0) return SHW_FPGA_LX240T;
	return SHW_FPGA_LX130T;
}
