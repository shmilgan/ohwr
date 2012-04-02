#ifndef I2C_SFP_H
#define I2C_SFP_H

//address from AT24C01 datasheet (1k, all address lines shorted to the ground)
#define I2C_SFP_ADDRESS 0x50

i2c_sfp_select_port(struct i2c_bus_t* bus, uint32_t address, uint8_t port);
i2c_sfp_read(struct i2c_bus_t* bus

#endif			//I2C_SFP_H