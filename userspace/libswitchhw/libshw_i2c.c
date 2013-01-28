#include "libshw_i2c.h"

int wrswhw_pca9554_configure(struct i2c_bus* bus, uint32_t address,uint8_t value)
{
    //config initial port dir
    uint8_t config_outputs[2] = {PCA9554_CMD_DIR_REG, value};
    int result = i2c_transfer(bus, address, sizeof(config_outputs), 0, config_outputs);
    if (result < 0)
	return result;
}


int wrswhw_pca9554_set_output_reg(struct i2c_bus* bus, uint32_t address, uint8_t value)
{
    //set initial output state
    uint8_t set_outputs[2] = {PCA9554_CMD_OUTPUT_REG, value};
    return i2c_transfer(bus, address, sizeof(set_outputs), 0, set_outputs);
}

int32_t wrswhw_pca9554_get_input(struct i2c_bus* bus, uint32_t address)
{
    //set read address
    uint8_t r = PCA9554_CMD_INPUT_REG;
    int result = i2c_transfer(bus, address, 1, 0, &r);
    if (result < 0)
	return result;
	
    //read one byte
    result = i2c_transfer(bus,address, 0, 1, &r);
    if (result < 0)
	return result;

    return r;
}

int wrswhw_pca9554_set_output_bits(struct i2c_bus* bus, uint32_t address, uint8_t value)
{
    int result = wrswhw_pca9554_get_input(bus, address);
    if (result < 0)
	return result;
	
    result |= value;	//or input bits
    result &= 0xFF;	//leave only 8 LSBs
        
    return wrswhw_pca9554_set_output_reg(bus, address, result);
}

int wrswhw_pca9554_clear_output_bits(struct i2c_bus* bus, uint32_t address, uint8_t value)
{
    int result = wrswhw_pca9554_get_input(bus, address);
    if (result < 0)
	return result;
	
    result &= ~value;	//remove bits we don't need
    result &= 0xFF;	//leave only 8 LSBs
        
    return wrswhw_pca9554_set_output_reg(bus, address, result);
}

