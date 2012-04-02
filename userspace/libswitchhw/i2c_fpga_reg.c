/*
 * i2c_cpu_bb.c
 */

#include <stdlib.h>
#include <string.h>
#include <hw/util.h> //for shw_udelay();

#include "i2c_fpga_reg.h"


i2c_bus_t*	i2c_fpga_reg_new_bus(uint32_t address, uint32_t prescaler)
{
    i2c_bus_t* bus = (i2c_bus_t*)shw_malloc(sizeof(i2c_bus_t));
    if (!bus)
        return NULL;
        
    memset((void*) bus, 0, sizeof(i2c_bus_t));
        
    i2c_fpga_reg_t* type_specific = (i2c_fpga_reg_t*)shw_malloc(sizeof(i2c_fpga_reg_t));
        
    if (!type_specific)
    {
	shw_free(bus);
	return NULL;
    }
    
    type_specific->base_address = address;
    
    bus->type_specific = (void*)type_specific;
    bus->type = I2C_BUS_TYPE_FPGA_REG;
    
    bus->transfer = i2c_fpga_reg_transfer;
    bus->scan = i2c_fpga_reg_scan;
    
    _fpga_writel(address + FPGA_I2C_REG_CTR, 0);
    _fpga_writel(address + FPGA_I2C_REG_PREL, prescaler & 0xFF);
    _fpga_writel(address + FPGA_I2C_REG_PREH, prescaler >> 8);
    _fpga_writel(address + FPGA_I2C_REG_CTR, CTR_EN);
    
    if (!(_fpga_readl(address + FPGA_I2C_REG_CTR) & CTR_EN))
    {
	i2c_free(bus);		//free bus if we did not manage to connect to it
	return NULL;
    }
    return bus;
}


static void mi2c_wait_busy(uint32_t fpga_address)
{
    while (_fpga_readl(fpga_address + FPGA_I2C_REG_SR) & SR_TIP);		//read from status register transfer in progress flag
}


//@return: 1 - ACK, 0 - NAK
static int mi2c_start(uint32_t fpga_address, uint32_t i2c_address, uint32_t mode)
{
    i2c_address <<= 1;
    if (mode == I2C_READ)
	i2c_address |= 1;

    _fpga_writel(fpga_address + FPGA_I2C_REG_TXR, i2c_address);		//set address
    _fpga_writel(fpga_address + FPGA_I2C_REG_CR, CR_STA | CR_WR);	//write to control register start write
    mi2c_wait_busy(fpga_address);				//wait till transfer is completed
    
    int result = _fpga_readl(fpga_address + FPGA_I2C_REG_SR) & SR_RXACK;
    
    printf("Address: %02X, Result: %d\n", i2c_address >> 1, result);
    
    return 1;
    //return result ? 1: 0;
}


//0 - nak, 1 - ack
static int mi2c_write_byte(uint32_t fpga_address, uint8_t data, uint32_t last)
{
    uint32_t cmd = CR_WR;
    if (last)
	cmd |= CR_STO;		//if it's last byte issue stop condition after sending byte
    
    _fpga_writel(fpga_address + FPGA_I2C_REG_TXR, data);	//write into txd register the byte
    _fpga_writel(fpga_address + FPGA_I2C_REG_CR, cmd);		//write command
    
    mi2c_wait_busy(fpga_address);
    
    return 1;
    //return !(_fpga_readl(fpga_address + FPGA_I2C_REG_SR) & SR_RXACK);
}

static int mi2c_read_byte(uint32_t fpga_address, uint32_t last)
{
    uint32_t cmd = CR_RD;
    if (last)
	cmd |= CR_STO | CR_ACK;		//CR_ACK means DO NOT SEND ACK

    _fpga_writel(fpga_address + FPGA_I2C_REG_CR, cmd);
    mi2c_wait_busy(fpga_address);
    
    return _fpga_readl(fpga_address + FPGA_I2C_REG_RXR);		//read byte from rx register
}


int32_t		i2c_fpga_reg_scan(struct i2c_bus_t* bus, uint32_t i2c_address)
{
    if (!bus)
        return I2C_NULL_PARAM;
        
    if (bus->type != I2C_BUS_TYPE_FPGA_REG)
	return I2C_BUS_MISMATCH;
    
    i2c_fpga_reg_t* ts = (i2c_fpga_reg_t*)bus->type_specific;
    uint32_t fpga_address = ts->base_address;
    
    printf("I2C address: %08X\n", fpga_address);
    
    //start condition and wait for result
    int ack = mi2c_start(fpga_address, i2c_address, I2C_READ);

    // stop condition and wait for result
    uint32_t cmd = CR_STO;
    _fpga_writel(fpga_address + FPGA_I2C_REG_CR, cmd);
    mi2c_wait_busy(fpga_address);				//todo: check if we need to wait
    
    return 1;
}

int32_t		i2c_fpga_reg_transfer(struct i2c_bus_t* bus, uint32_t i2c_address,  uint32_t to_write, uint32_t to_read, uint8_t* data)
{
    if (!bus)
	return I2C_NULL_PARAM;
    if (bus->type != I2C_BUS_TYPE_FPGA_REG)
	return I2C_BUS_MISMATCH;
	
    i2c_fpga_reg_t* ts = (i2c_fpga_reg_t*)bus->type_specific;
    uint32_t fpga_address = ts->base_address;

    int ack = 0;
    int sent = 0;
    int rcvd = 0;
    int b;
	
    if (to_write)
    {
    	ack = mi2c_start(fpga_address, i2c_address, I2C_WRITE);
    	if (!ack)		//NAK
    	    return I2C_DEV_NOT_FOUND;
    	
    	printf("To write: %d\n", to_write);
    	
    	int last = to_write-1;
    	for (b = 0; b < to_write; b++)
    	{
    	    ack = mi2c_write_byte(fpga_address, data[b], b == last);
    	    if (!ack)
    		return I2C_NO_ACK_RCVD;
    	    sent++; 
    	}
    }
    
    if (to_read)
    {
	ack = mi2c_start(fpga_address, i2c_address, I2C_READ);
	if (!ack)		//NAK
    	    return I2C_DEV_NOT_FOUND;
    	
    	int last = to_read-1;
    	for (b = 0; b < to_read; b++)
    	{
    	    data[b] = mi2c_read_byte(fpga_address, b == last);
    	    rcvd++;
    	}
    }
    return sent + rcvd;
}

