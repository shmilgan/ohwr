/*
 * i2c_cpu_bb.c
 */

#include <stdlib.h>
#include <string.h>
#include <hw/util.h> //for shw_udelay();

#include "i2c_fpga_reg.h"


int i2c_fpga_reg_init_bus(struct i2c_bus *bus)
{
    i2c_fpga_reg_t *priv;

    if (!bus->type_specific) {
    	printf("no type specific structure provided\n");
    	return -1;
    }
    if (bus->type != I2C_BUS_TYPE_FPGA_REG) {
    	printf("type doesn't match I2C_BUS_TYPE_FPGA_REG(%d): %d\n", I2C_BUS_TYPE_FPGA_REG, bus->type);
	return -1;
    }

    priv = (i2c_fpga_reg_t *)bus->type_specific;
    
    bus->transfer = i2c_fpga_reg_transfer;
    bus->scan = i2c_fpga_reg_scan;
    
    _fpga_writel(priv->base_address + FPGA_I2C_REG_CTR, 0);
    _fpga_writel(priv->base_address + FPGA_I2C_REG_PREL, priv->prescaler & 0xFF);
    _fpga_writel(priv->base_address + FPGA_I2C_REG_PREH, priv->prescaler >> 8);
    _fpga_writel(priv->base_address + FPGA_I2C_REG_CTR, CTR_EN);
    
    if (!(_fpga_readl(priv->base_address + FPGA_I2C_REG_CTR) & CTR_EN)) {
    	printf("failed to read from control register\n");
	return -1;
    }

    return 0;
}


static void mi2c_wait_busy(uint32_t fpga_address)
{
    while (_fpga_readl(fpga_address + FPGA_I2C_REG_SR) & SR_TIP);		//read from status register transfer in progress flag
}


//@return: 1 - ACK, 0 - NAK
static int mi2c_start(uint32_t fpga_address, uint32_t i2c_address, uint32_t mode)
{
    i2c_address = (i2c_address << 1) & 0xFF;
    if (mode == I2C_READ)
	i2c_address |= 1;

    _fpga_writel(fpga_address + FPGA_I2C_REG_TXR, i2c_address);		//set address
    _fpga_writel(fpga_address + FPGA_I2C_REG_CR, CR_STA | CR_WR);	//write to control register start write
    mi2c_wait_busy(fpga_address);				//wait till transfer is completed
    
    uint32_t result = _fpga_readl(fpga_address + FPGA_I2C_REG_SR);
   // printf("addr = %02x, result = %02x\n", i2c_address>>1, result);
    
    return (result & SR_RXACK) ? 0 : 1;
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
    
//    return 1;
    return !(_fpga_readl(fpga_address + FPGA_I2C_REG_SR) & SR_RXACK);
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


int32_t		i2c_fpga_reg_scan(struct i2c_bus* bus, uint32_t i2c_address)
{
    if (!bus)
        return I2C_NULL_PARAM;
        
    if (bus->type != I2C_BUS_TYPE_FPGA_REG)
	return I2C_BUS_MISMATCH;
    
    i2c_fpga_reg_t* ts = (i2c_fpga_reg_t*)bus->type_specific;
    uint32_t fpga_address = ts->base_address;
    
    //start condition and wait for result
    int ack = mi2c_start(fpga_address, i2c_address, I2C_READ);

    if (ack) {
        // stop condition and wait for result
        _fpga_writel(fpga_address + FPGA_I2C_REG_TXR, 0);
        _fpga_writel(fpga_address + FPGA_I2C_REG_CR, CR_STO | CR_WR);
        mi2c_wait_busy(fpga_address);
//        printf("address: %02X has a device\n", i2c_address);
       return 1;
    }

    return 0;
}

int32_t		i2c_fpga_reg_transfer(struct i2c_bus* bus, uint32_t i2c_address,  uint32_t to_write, uint32_t to_read, uint8_t* data)
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
