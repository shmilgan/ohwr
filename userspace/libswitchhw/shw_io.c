/*
 * pio_pins.c
 *
 * Define the PIO pin available for the CPU & the FPGA.
 *
 *  Created on: Oct 30, 2012
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
#include <pio.h>
#include <trace.h>
#include <string.h>

#include "shw_io.h"

#include "i2c_io.h"
#include "libshw_i2c.h"

//Help to setup the structure.
#define IOARR_SET_IO(_name,_type,_ptr) { 		\
		all_shw_io[_name].ID=_name;				\
		all_shw_io[_name].type=_type;				\
		all_shw_io[_name].name=#_name;	\
		all_shw_io[_name].ptr=(void*)_ptr; }

#define IOARR_SET_GPIO(_name) { 				\
		all_shw_io[_name].ID=_name;				\
		all_shw_io[_name].type=SHW_CPU_PIO;		\
		all_shw_io[_name].name=#_name;	\
		all_shw_io[_name].ptr=(void*)PIN_##_name; }


// definitions of commonly IO pins used with through all the versions (static memory)

// reset signal for main FPGA
//const pio_pin_t PIN_main_fpga_nrst[] = {{ PIOA, 5, PIO_MODE_GPIO, PIO_OUT }, {0}};
const pio_pin_t PIN_shw_io_reset_n[] =  {{PIOE, 1, PIO_MODE_GPIO, PIO_OUT_1}, {0}};
const pio_pin_t PIN_shw_io_box_fan_en[] =  {{PIOB, 20, PIO_MODE_GPIO, PIO_OUT_0}, {0}}; //<3.3 (then used for i2c io)
const pio_pin_t PIN_shw_io_box_fan_tacho[] =  {{PIOE, 7, PIO_MODE_GPIO, PIO_IN}, {0}};
const pio_pin_t PIN_shw_io_fpga_fan_en[]  =  {{PIOB, 24, PIO_MODE_GPIO, PIO_OUT}, {0}};
const pio_pin_t PIN_shw_io_led_cpu1[] =  {{PIOA, 0, PIO_MODE_GPIO, PIO_OUT}, {0}};
const pio_pin_t PIN_shw_io_led_cpu2[] =  {{PIOA, 1, PIO_MODE_GPIO, PIO_OUT}, {0}};
const pio_pin_t PIN_shw_io_arm_boot_sel[]  =  {{PIOC, 7, PIO_MODE_GPIO, PIO_IN}, {0}};
const pio_pin_t PIN_shw_io_arm_gen_but[]  =  {{PIOE, 9, PIO_MODE_GPIO, PIO_IN}, {0}};

const shw_chip_t I2C_pca9554_ver = {(void*)&i2c_io_bus, 0x20, 0x0F, I2C_CHIP_PCA9554};


const shw_io_bus_t I2C_shw_io_led_state_g[] = { {&I2C_pca9554_ver, (1<<4),4}, {0}};
const shw_io_bus_t I2C_shw_io_led_state_o[] = { {&I2C_pca9554_ver, (1<<5),5}, {0}};


//Declaration of the array
const shw_io_t _all_shw_io[NUM_SHW_IO_ID];



int shw_io_init()
{
	int ret, ver;

	//Remove const for writing
	shw_io_t* all_shw_io=(shw_io_t*)_all_shw_io;

	//Map CPU's pin into memory space
	assert_init(shw_pio_mmap_init());

	//then init the i2c (if it was not done)
	if(i2c_io_bus.err==I2C_NULL_PARAM)
		assert_init(shw_i2c_io_init());

	//then obtain the serial number
	ver=shw_get_hw_ver();

	//Finally assigned the input/ouput according to version number.
	if(ver<330)
	{
		IOARR_SET_GPIO(shw_io_reset_n);
		IOARR_SET_GPIO(shw_io_box_fan_en);
		IOARR_SET_GPIO(shw_io_box_fan_tacho);
		IOARR_SET_GPIO(shw_io_fpga_fan_en);
		IOARR_SET_GPIO(shw_io_led_cpu1);
		IOARR_SET_GPIO(shw_io_led_cpu2);
		IOARR_SET_GPIO(shw_io_arm_boot_sel);
		IOARR_SET_GPIO(shw_io_arm_gen_but);
		TRACE(TRACE_INFO, "version=%f %d %d %d",ver, ver>=3.3f, ver<3.3f, ver==3.3f);
	}
	else
	{
		IOARR_SET_GPIO(shw_io_reset_n);
		IOARR_SET_GPIO(shw_io_led_cpu1);
		IOARR_SET_GPIO(shw_io_led_cpu2);
		IOARR_SET_GPIO(shw_io_arm_boot_sel);

		IOARR_SET_IO(shw_io_led_state_g,SHW_I2C,I2C_shw_io_led_state_g);
		IOARR_SET_IO(shw_io_led_state_o,SHW_I2C,I2C_shw_io_led_state_o);

		//Finally setup the orange led state before initiate anything else
		shw_io_write(shw_io_led_state_o,1);
	}

	TRACE(TRACE_INFO, "version=%g",ver);
	return 0;
}

int shw_io_configure_all()
{
	int i;
	const shw_io_bus_t *iobus;
	const shw_io_t* all_io=(shw_io_t*)_all_shw_io;

	for(i=0;i<NUM_SHW_IO_ID;i++)
	{
		const shw_io_t* io=(const shw_io_t*)&_all_shw_io[i];
		switch(io->type)
		{
			case SHW_CPU_PIO:
				shw_pio_configure(all_io[i].ptr);
				break;
			case SHW_I2C:
				iobus = (const shw_io_bus_t *)io->ptr;
				if(iobus->chip && iobus->chip->type==I2C_CHIP_PCA9554)
					wrswhw_pca9554_configure(iobus->chip->bus,iobus->chip->addr,iobus->chip->config);
				break;
			case SHW_UNDEF:
				//Do nothing for undefined type
				break;
			default:
				TRACE(TRACE_INFO,"Config not implemented for type %d for io #%d",io->type,i); break;
		}
	}
	return 0;
}



const pio_pin_t* get_pio_pin(shw_io_id_t id)
{
	const shw_io_t *wrpin=get_shw_io(id);
	if(wrpin && wrpin->type==SHW_CPU_PIO)
		return (wrpin)?(const pio_pin_t*)wrpin->ptr:0;
}

const shw_io_t* get_shw_io(shw_io_id_t id)
{
	if(0< id && id < NUM_SHW_IO_ID)
	{
		if(_all_shw_io[id].ID==id) return &(_all_shw_io[id]);
		else TRACE(TRACE_ERROR,"IO %d does not correspond to its ID %s",id,_all_shw_io[id].name);
	}
	else TRACE(TRACE_ERROR,"IO %d does not exist",id);
	return 0;
}


uint32_t shw_io_read(shw_io_id_t id)
{
	uint32_t ret;
	int32_t i32data;
	uint8_t u8data[2];
	const shw_io_t* io=&_all_shw_io[id];
	const shw_io_bus_t *iobus;

	if(0< id && id < NUM_SHW_IO_ID && io->ID==id && io->ptr)
	{
		switch(io->type)
		{
		case SHW_CPU_PIO:
			return shw_pio_get((const pio_pin_t*)io->ptr);
		case SHW_I2C:
			iobus = (const shw_io_bus_t *)io->ptr;
			if(iobus->chip && iobus->chip->type==I2C_CHIP_PCA9554)
			{
				i32data = wrswhw_pca9554_get_input(iobus->chip->bus,iobus->chip->addr);
				return ((i32data & iobus->mask) >> iobus->shift);
			}
		case SHW_UNDEF:
			TRACE(TRACE_ERROR,"IO #%d is undef",id); break;
		default:
			TRACE(TRACE_ERROR,"Unknow type %d for io #%d",io->type,id); break;
		}
	}
	return ret;
}

int shw_io_write(shw_io_id_t id, uint32_t value)
{
	int ret=-1;
	int32_t i32data;
	uint8_t u8data[2];
	const shw_io_t* io=&_all_shw_io[id];
	const shw_io_bus_t *iobus;

	if(0< id && id < NUM_SHW_IO_ID && io->ID==id && io->ptr)
	{
		switch(io->type)
		{
		case SHW_CPU_PIO:
			shw_pio_set((const pio_pin_t*)io->ptr,value);
			return 0;
		case SHW_I2C:
			iobus = (const shw_io_bus_t *)io->ptr;
			if(iobus->chip && iobus->chip->type==I2C_CHIP_PCA9554)
			{
				i32data = wrswhw_pca9554_get_input(iobus->chip->bus,iobus->chip->addr);
				i32data &= ~iobus->mask;
				value <<= iobus->shift;
				value  &= iobus->mask;
				return wrswhw_pca9554_set_output_reg(iobus->chip->bus,iobus->chip->addr,value | i32data);
			}
		case SHW_UNDEF:
			TRACE(TRACE_ERROR,"Pin #%d is undef",id); break;
		default:
			TRACE(TRACE_ERROR,"Unknow type %d for io #%d",io->type,id); break;
		}
	}
	return -1;
}



const char *get_shw_info(const char cmd)
{
	static char str_hwver[10];
	int tmp;

	switch(cmd)
	{
	case 'p':
		tmp = shw_get_hw_ver();
		snprintf(str_hwver,10,"%d.%d",tmp/100,tmp%100);
		return str_hwver;
	case 'f':
		return (shw_get_fpga_type()==SHW_FPGA_LX240T)?"LX240T":"LX130T";
	}
	return "";
}


