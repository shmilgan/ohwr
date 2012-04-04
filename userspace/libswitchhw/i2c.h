/*
	i2c.h
	B.Bielawski CERN 2012
*/

#ifndef I2C_H
#define I2C_H

#include <stdint.h>

#define I2C_OK			  0
#define I2C_DEV_NOT_FOUND	-1
#define I2C_NO_ACK_RCVD	-2
#define I2C_ALLOC_ERROR	-0x10
#define I2C_NULL_PARAM	-0x11
#define I2C_BUS_MISMATCH	-0x12

#define I2C_TYPE_BITBANG	0
#define I2C_BUS_TYPE_FPGA_REG	1

#define I2C_WRITE	0
#define I2C_READ	1


typedef struct i2c_bus
{
	const char	*name;
	int 		type;
	void*		type_specific;
	int32_t		(*transfer)(struct i2c_bus* bus, uint32_t address,  uint32_t to_write, uint32_t to_read, uint8_t* data);
	int32_t		(*scan)(struct i2c_bus* bus, uint32_t address);
	int		err;
} i2c_bus_t;

int i2c_init_bus(struct i2c_bus *bus);

int32_t i2c_transfer(struct i2c_bus* bus, uint32_t address, uint32_t to_write, uint32_t to_read, uint8_t* data);

int32_t i2c_write(struct i2c_bus* bus, uint32_t address, uint32_t to_write, uint8_t* data);
int32_t i2c_read (struct i2c_bus* bus, uint32_t address, uint32_t to_read,  uint8_t* data);

int32_t i2c_scan(struct i2c_bus* bus, uint8_t* data);



#endif	//I2C_H

