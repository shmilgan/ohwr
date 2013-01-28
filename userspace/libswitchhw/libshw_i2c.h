#ifndef LIBSHW_I2C_H
#define LIBSHW_I2C_H

#include <stdint.h>
#include "i2c.h"

#define LINK0_LED_LINK		0x1
#define LINK0_LED_WRMODE	0x2
#define LINK0_SFP_TX_DISABLE	0x4
#define LINK0_LED_SYNCED	0x8

#define LINK1_LED_WRMODE	0x10
#define LINK1_LED_SYNCED	0x20
#define LINK1_LED_LINK		0x40
#define LINK1_SFP_TX_DISABLE	0x80

//command to be sent as a first byte after address
#define PCA9554_CMD_INPUT_REG		0
#define PCA9554_CMD_OUTPUT_REG		1
#define PCA9554_CMD_POLARITY_REG	2
#define PCA9554_CMD_DIR_REG		3

#define I2C_CHIP_PCA9554 1

//output bitmap for each expander (all out)
#define WRSWHW_OUTPUT_BITMAP 		0
//initial state for outputs
#define WRSWHW_INITIAL_OUTPUT_STATE	(LINK1_SFP_TX_DISABLE | LINK0_SFP_TX_DISABLE)

int wrswhw_pca9554_configure(struct i2c_bus* bus, uint32_t address,uint8_t value);
int wrswhw_pca9554_set_output_reg(struct i2c_bus* bus, uint32_t address, uint8_t value);

int wrswhw_pca9554_get_input(struct i2c_bus* bus, uint32_t address);
int wrswhw_pca9554_set_output_bits  (struct i2c_bus* bus, uint32_t address, uint8_t value);
int wrswhw_pca9554_clear_output_bits(struct i2c_bus* bus, uint32_t address, uint8_t value);

#endif //LIBSHW_I2C_H
