
#include <string.h>

#include "i2c.h"


int32_t i2c_transfer(struct i2c_bus_t* bus, uint32_t address, uint32_t to_write, uint32_t to_read, uint8_t* data)
{
	return bus->transfer(bus, address, to_write, to_read, data);
}


void i2c_free(struct i2c_bus_t* bus)
{
    if (!bus)
	return;
	
    if (bus->type_specific)
	shw_free(bus->type_specific);

    shw_free(bus);
}


int32_t i2c_write(struct i2c_bus_t* bus, uint32_t address, uint32_t to_write, uint8_t* data)
{
    return bus->transfer(bus, address, to_write, 0, data);
}


int32_t i2c_read (struct i2c_bus_t* bus, uint32_t address, uint32_t to_read,  uint8_t* data)
{
    return bus->transfer(bus, address, 0, to_read, data);
}


int32_t i2c_scan(struct i2c_bus_t* bus, uint8_t* data)
{
    if (!bus)
        return I2C_NULL_PARAM;
        
    const int devices = 128;
    
    int address;
    
    const int first_valid_address = 0x20;
    const int last_valid_address = 0x20;

    memset((void*)data, 0, 16);		//16 bytes * 8 addresses per byte == 128 addresses
    
    int found = 0;
    
    for (address = first_valid_address; address <= last_valid_address; address++)
    {
	int res = bus->scan(bus, address);
	if (res)		//device present
	{
	    int offset = address >> 3;			//choose proper byte
	    int bit = 1 << (7-(address & 0x7));		//choose proper bit
	    data[offset] |= bit;
	    found++;
	}
    }
    
    return found;
}
