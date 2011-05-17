/*
 *
 *
 *
 *
 */
#ifndef I2CSW_H
#define I2CSW_H

#include "global.h"

// include project-dependent settings
#include "i2cswconf.h"

// defines and constants
#define READ		0x01	// I2C READ bit

#define I2C_ADDRESS_LOCAL	0
#define I2C_ADDRESS_REMOTE	1

#define I2C_TEMP     0
#define I2C_IPMB0_A  1
#define I2C_IPMB0_B  2
#define I2C_IPMBL_1  3

// functions
enum i2c_state{
	I2C_INIT,
	I2C_START_BIT,
	I2C_WAIT_SCL,
	I2C_GET_BIT,
	I2C_STOP_BIT,
	I2C_ACK,
	I2C_NACK,
	I2C_TIMEOUT_I2C,
	I2C_WRG_ADDR,
	I2C_TIMEOUT_STOP,
} ;

typedef struct i2c_pins{
	 volatile uint8_t* sda_ddr;
	 volatile uint8_t* sda_port;
	 volatile uint8_t* sda_pin;
	 uint8_t sda;

	 volatile uint8_t* scl_ddr;
	 volatile uint8_t* scl_port;
	 volatile uint8_t* scl_pin;
	 volatile uint8_t scl;

	uint8_t slave_addr;

	uint8_t pkt_in[35];
	uint8_t len_in;

}I2C_PINS;
typedef struct i2c_data{

	I2C_PINS pins;
	enum i2c_state I2C_state;

}I2C_DATA;

// initialize I2C interface pins
void i2c_bus_init(void);
I2C_DATA* getBus(uint8_t i2c_bus);


//uint8_t i2c_Receive(I2C_DATA* i2c);

uint8_t i2c_Send(I2C_DATA* i2c_bus,uint8_t slave_addr, uint8_t wr_len, uint8_t* wr_data);

uint8_t i2c_Read(I2C_DATA* i2c_bus,uint8_t slave_addr, uint8_t rd_len, uint8_t* rd_data);

uint8_t i2cMasterReceive(I2C_DATA* i2c_bus,uint8_t slave_addr,uint8_t wr_len, uint8_t* wr_data,uint8_t rd_len, uint8_t* rd_data);

uint8_t read_ipmb_a(I2C_DATA* IPMB,IPMI_WS* ws);

void i2c_master_write( IPMI_WS *ws );

void i2c_master_read( IPMI_WS *ws );

IPMI_WS* IPMB0_A_READ(I2C_DATA*IPMB);

#endif
