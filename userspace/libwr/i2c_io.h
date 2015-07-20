#ifndef I2C_IO_H
#define I2C_IO_H

#include "i2c.h"

//address from AT24C01 datasheet (1k, all address lines shorted to the ground)
#define I2C_SFP_ADDRESS 0x50

#define SHW_FPGA_LX240T 240
#define SHW_FPGA_LX130T 130

/**
 * Initializes all the buses related to SFP control. These include:
 *     - FPGA buses x 2
 *     - Bitbanged bus to muxes
 *     - Bitbanged buses to SFP link0 and link1
 * @return: 0 on success, -1 on error
 */

extern struct i2c_bus i2c_io_bus;

int shw_i2c_io_init(void);
int shw_i2c_io_scan(uint8_t * dev_map);

int shw_get_hw_ver(void);
uint8_t shw_get_fpga_type(void);

#endif //I2C_IO_H
