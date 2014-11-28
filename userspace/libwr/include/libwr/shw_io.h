/*
 * shw_io.h
 *
 * Generic definition of IO available for the CPU & the FPGA.
 *
 *  Created on: Jan 20, 2013
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

#ifndef _LIBWR_SHW_IO_H_
#define _LIBWR_SHW_IO_H_

#include <stdint.h>

#include <libwr/shw_io.h>
#include <libwr/pio.h>

#define assert_init(proc) { int ret; if((ret = proc) < 0) return ret; }

/**
 * List of type of IO.
 */
typedef enum {
	SHW_UNDEF = 0,		//undefined type
	SHW_CPU_PIO,
	SHW_I2C,
	SHW_WB_PIO,
	SHW_WB_SYSM
} shw_io_type_t;

/**
 * List of ID for the various IO.
 */
typedef enum {
	shw_io_reset_n = 1,	//start at 1 (0 <=> undef)
	shw_io_box_fan_en,	//(< v3.3)
	shw_io_box_fan_tacho,	//(< v3.3)
	shw_io_fpga_fan_en,	//(< v3.3)
	shw_io_led_cpu1,
	shw_io_led_cpu2,
	shw_io_arm_boot_sel,	//(>= v3.2)
	shw_io_led_state_g,	//(>= v3.3)
	shw_io_led_state_o,	//(>= v3.3)
	shw_io_arm_gen_but,	//(>= v3.3)
	NUM_SHW_IO_ID
} shw_io_id_t;

/**
 * Structure to setup name and ID
 */
typedef struct {
	uint8_t ID;
	uint8_t type;
	const char *name;
	void *ptr;
} shw_io_t;

/*
 * Parameters to a chip
 */
typedef struct {
	void *bus;
	uint32_t addr;
	uint32_t config;
	uint32_t type;
} shw_chip_t;

/*
 * Structure to fake an I/O using on a bus
 */
typedef struct {
	const shw_chip_t *chip;
	uint8_t mask;		//Mask of the interesting bit
	uint8_t shift;		//Shift masking bit
} shw_io_bus_t;

/**
 * Exported structure to use them (Same size as enum)
 */
extern const shw_io_t _all_shw_io[];

//Functions
int shw_io_init();
int shw_io_configure_all();
const shw_io_t *get_shw_io(shw_io_id_t id);
const pio_pin_t *get_pio_pin(shw_io_id_t id);
uint32_t shw_io_read(shw_io_id_t id);
int shw_io_write(shw_io_id_t, uint32_t value);

const char *get_shw_info(const char cmd);

#endif /* _LIBWR_SHW_IO_H_ */
