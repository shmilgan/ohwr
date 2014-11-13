/*
	i2c_fpga_reg.h
	2012 CERN B.Bielawski 
*/

#ifndef I2C_FPGA_REG_H
#define I2C_FPGA_REG_H

#include <fpga_io.h>
#include "i2c.h"

#define FPGA_I2C_REG_PREL	0x00
#define FPGA_I2C_REG_PREH	0x04
#define FPGA_I2C_REG_CTR	0x08
#define FPGA_I2C_REG_TXR	0x0C
#define FPGA_I2C_REG_RXR	0x0C
#define FPGA_I2C_REG_CR		0x10
#define FPGA_I2C_REG_SR		0x10
#define FPGA_I2C_REG_IFS  0x14

#define CTR_EN 			(1<<7)
#define CR_STA			(1<<7)
#define CR_STO			(1<<6)
#define CR_RD			(1<<5)
#define CR_WR			(1<<4)
#define CR_ACK			(1<<3)
#define SR_RXACK		(1<<7)
#define SR_TIP			(1<<1)
#define IFS_BUSY    (1<<7)

#define FPGA_I2C_ADDRESS 0x54000
#define FPGA_I2C0_IFNUM 0
#define FPGA_I2C1_IFNUM 1
#define FPGA_I2C_SENSORS_IFNUM 2

typedef struct
{
    uint32_t base_address;
    uint32_t if_num;
    uint32_t prescaler;
} i2c_fpga_reg_t;

int		i2c_fpga_reg_init_bus(struct i2c_bus *bus);

#endif //I2C_FPGA_REG_H
