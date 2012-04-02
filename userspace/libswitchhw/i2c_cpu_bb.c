/*
 * i2c_cpu_bb.c
 */

#include <stdlib.h>
#include <string.h>
#include <hw/util.h> //for shw_udelay();

#include "i2c_cpu_bb.h"


 
 
struct i2c_bus_t*		i2c_cpu_bb_new_bus(pio_pin_t* scl_pin, pio_pin_t* sda_pin)
{
	 //alloc structures
	i2c_bus_t* bus = (i2c_bus_t*)shw_malloc(sizeof(i2c_bus_t));
	if (!bus)
	    return NULL;
	
	memset((void*)bus, 0, sizeof(i2c_bus_t));
	
	i2c_cpu_bb_t* type_specific = (i2c_cpu_bb_t*)shw_malloc(sizeof(i2c_cpu_bb_t));
	if (!type_specific)
		return NULL;
	memset(type_specific, 0, sizeof(type_specific));
	
	//setup pins
	type_specific->scl = scl_pin;
	type_specific->sda = sda_pin;

	shw_pio_setdir(scl_pin, 0);
	shw_pio_setdir(sda_pin, 0);

	bus->type_specific = type_specific;
	
	//setup bus type
	bus->type = I2C_BUS_TYPE_CPU_BB;
	
	//assign functions
	bus->transfer = i2c_cpu_bb_transfer;
	bus->scan = i2c_cpu_bb_scan;
	return bus;
 }
 
 #define I2C_DELAY 100
 
 static void mi2c_pin_out(pio_pin_t* pin, int state)
 {
	shw_pio_setdir(pin, state ? 0 : 1);
	shw_udelay(I2C_DELAY);
 }

 static void mi2c_start(i2c_cpu_bb_t* bus)
 {
	mi2c_pin_out(bus->sda, 0);
	mi2c_pin_out(bus->scl, 0);
 }
 
 static void mi2c_restart(i2c_cpu_bb_t* bus)
 {
	mi2c_pin_out(bus->sda, 1);
	mi2c_pin_out(bus->scl, 1);
	mi2c_pin_out(bus->sda, 0);
	mi2c_pin_out(bus->scl, 0);
 }
 
 
 static void mi2c_stop(i2c_cpu_bb_t* bus)
 {
	 mi2c_pin_out(bus->sda,0);
	 mi2c_pin_out(bus->scl,1);
	 mi2c_pin_out(bus->sda,1);
 }
 
 
 static int mi2c_write_byte(i2c_cpu_bb_t* bus, uint8_t data)
 {
	int ack = 0;
	int b;
	 
	for (b = 0; b < 8; b++)
	{
		mi2c_pin_out(bus->sda, data & 0x80);	//set MSB to SDA
		mi2c_pin_out(bus->scl, 1);			//toggle clock
		mi2c_pin_out(bus->scl, 0);
		data <<= 1;
	}
	 
	mi2c_pin_out(bus->sda, 1);	//go high
	mi2c_pin_out(bus->scl, 1);	//toggle clock
	 
	shw_pio_setdir(bus->sda, PIO_IN);		//let SDA float
	shw_udelay(I2C_DELAY);
	ack = shw_pio_get(bus->sda);
	 
	mi2c_pin_out(bus->scl, 0);
	shw_pio_setdir(bus->sda, PIO_OUT_1);	//turn on output
	shw_udelay(I2C_DELAY);
	 
	return (ack == 0) ? 1: 0;
 }
 
 
 
static uint8_t mi2c_get_byte(i2c_cpu_bb_t *bus, int ack)
{
	int i;
	unsigned char result = 0;

	mi2c_pin_out(bus->scl, 0);
	shw_pio_setdir(bus->sda, PIO_IN);		//let SDA float so we can read it
	
	for (i=0;i<8;i++)
	{
		mi2c_pin_out(bus->scl, 1);
		result <<= 1;
		if (shw_pio_get(bus->sda))
			result |= 0x01;
		mi2c_pin_out(bus->scl, 0);
	}

	//send ACK or NAK
	shw_pio_setdir(bus->sda, ack ? PIO_OUT_0: PIO_OUT_1);	
	shw_udelay(I2C_DELAY);
			
	mi2c_pin_out(bus->scl, 1);		//generate SCL pulse for slave to read ACK/NACK
	mi2c_pin_out(bus->scl, 0);
	
	return result;

}

int32_t i2c_cpu_bb_scan(struct i2c_bus_t* bus, uint32_t address)
{
    if (!bus)
	return I2C_NULL_PARAM;

    i2c_cpu_bb_t* ts = (i2c_cpu_bb_t*)bus->type_specific;

    mi2c_start(ts);
    int   ack = mi2c_write_byte(ts, (address << 1) | 0);		//add R bit at the end of address

    printf("Scan: %02X, Result: %d\n", address, ack);
    
    mi2c_stop(ts);
    
    return ack ? 1: 0;
}
 
int	i2c_cpu_bb_transfer(i2c_bus_t* bus, uint32_t address, uint32_t to_write, uint32_t to_read, uint8_t* data)
{
	if (!bus)
		return I2C_NULL_PARAM;
	
	if (bus->type != I2C_BUS_TYPE_CPU_BB)
		return I2C_BUS_MISMATCH;
	
	i2c_cpu_bb_t* ts = (i2c_cpu_bb_t*)bus->type_specific;
	
	int sent = 0;
	int rcvd = 0;
	int ack = 0;
	
	int i;
	
	if (to_write > 0)
	{
		mi2c_start(ts);
		ack = mi2c_write_byte(ts, address << 1);		//add W bit at the end of address
		if (!ack)
		{
			mi2c_stop(ts);
			return I2C_DEV_NOT_FOUND;				//the device did not ack it's address
		}		
		for (i=0; i < to_write; i++)
		{
			ack = mi2c_write_byte(ts, data[i]);		//write data
			if (!ack)
			{
				mi2c_stop(ts);
				return I2C_NO_ACK_RCVD;
			}
			sent++;
		}
		mi2c_stop(ts);
	}
	
	if (to_read)
	{
		mi2c_start(ts);
		ack = mi2c_write_byte(ts, (address << 1) | 1);		//add R bit at the end of address
		if (!ack)
		{
			mi2c_stop(ts);
			return I2C_DEV_NOT_FOUND;				//the device did not ack it's address
		}

		int last_byte = to_read - 1;
		
		for (i=0; i < to_read; i++)
		{
			data[i] = mi2c_get_byte(ts, i != last_byte);		//read data, ack until the last byte
			rcvd++;
		}
		mi2c_stop(ts);
	}
	
	return sent+rcvd;
}

