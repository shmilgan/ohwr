/*
 *
 *
 *
 */

#include <avr/io.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include "uart.h"
#include "ipmi.h"
#include "i2csw.h"
#include "ws.h"
#include "global.h"





// Standard I2C bit rates are:
// 100KHz for slow speed
// 400KHz for high speed

//#define QDEL	delay(5)		// i2c quarter-bit delay
//#define HDEL	delay(10)		// i2c half-bit delay

// i2c quarter-bit delay
//#define QDEL	asm volatile("nop"); asm volatile("nop"); asm volatile("nop"); asm volatile("nop"); asm volatile("nop");
// i2c half-bit delay
//#define HDEL	asm volatile("nop"); asm volatile("nop"); asm volatile("nop"); asm volatile("nop"); asm volatile("nop"); asm volatile("nop"); asm volatile("nop"); asm volatile("nop"); asm volatile("nop"); asm volatile("nop");

void i2cInitA(I2C_DATA* i2c_bus);
void i2cInitB(I2C_DATA* i2c_bus);
void i2cTemp(I2C_DATA* i2c_bus);


static I2C_DATA i2c_context[4];
uint8_t sda_pin,scl_pin;
IPMI_WS IPMI0_A_ws;


inline __attribute__((gnu_inline)) void clk_delay(uint8_t delay)
{
	while(delay--)
		asm volatile("nop");
}


#define QDEL clk_delay(5)
#define HDEL clk_delay(9)

#define I2C_SDA_IN      cbi( *i2c_bus->pins.sda_ddr, i2c_bus->pins.sda)
#define I2C_SDA_OUT     sbi( *i2c_bus->pins.sda_ddr, i2c_bus->pins.sda)

#define I2C_SCL_IN      cbi( *i2c_bus->pins.scl_ddr, i2c_bus->pins.scl)
#define I2C_SCL_OUT     sbi( *i2c_bus->pins.scl_ddr, i2c_bus->pins.scl)

#define I2C_SDA_LO      cbi( *i2c_bus->pins.sda_port, i2c_bus->pins.sda)
#define I2C_SDA_HI      sbi( *i2c_bus->pins.sda_port, i2c_bus->pins.sda)

#define I2C_SCL_LO      cbi( *i2c_bus->pins.scl_port, i2c_bus->pins.scl);
#define I2C_SCL_HI      sbi( *i2c_bus->pins.scl_port, i2c_bus->pins.scl);I2C_SCL_IN;while(((*i2c_bus->pins.scl_pin) & (1<<i2c_bus->pins.scl))==0);I2C_SCL_OUT

#define I2C_SCL_TOGGLE  QDEL;I2C_SCL_HI;HDEL;I2C_SCL_LO
#define I2C_START       I2C_SCL_HI;I2C_SDA_HI;QDEL;I2C_SDA_LO;QDEL;I2C_SCL_LO
#define I2C_STOP        HDEL;I2C_SCL_HI; QDEL; I2C_SDA_HI


#define SDADDR SDADDR_A
#define SDAPIN SDAPIN_A
#define SDAPORT SDAPORT_A

#define SCLPIN SCLPIN_A
#define SCLPORT SCLPORT_A





uint8_t i2cPutbyte(I2C_DATA* i2c_bus,uint8_t b)
{
	int8_t i;

	for (i=7;i>=0;i--)
	{
		if ( ((b) & (1<<i)) )
		{
			I2C_SDA_HI;
		}
		else
		{
			I2C_SDA_LO;			// address bit
		}

		I2C_SCL_TOGGLE;		// clock HI, delay, then LO
	}
	I2C_SDA_LO;					// leave SDA HI
	//Get Ack from slave
	I2C_SDA_IN;			// change direction to input on SDA line (may not be needed)

	clk_delay(8);

	I2C_SCL_HI;					// clock back up
  	b = (*i2c_bus->pins.sda_pin) & (1<<i2c_bus->pins.sda);	// get the ACK bit

	HDEL;
	I2C_SCL_LO;					// not really ??
	I2C_SDA_OUT;		// change direction back to output
	QDEL;
	return (b == 0);			// return ACK value
}


uint8_t i2cGetbyte(I2C_DATA* i2c_bus,uint8_t last)
{
	int8_t i;
	int8_t c,b = 0;

	I2C_SDA_HI;		// make sure pullups are ativated
	I2C_SDA_IN;		// change direction to input on SDA line (may not be needed)

	for(i=7;i>=0;i--)
	{
		HDEL;
		I2C_SCL_HI;				// clock HI
	  	c = (*i2c_bus->pins.sda_pin) & (_BV(i2c_bus->pins.sda));
		b <<= 1;
		if(c) b |= 1;
		HDEL;
    	I2C_SCL_LO;				// clock LO
	}

	I2C_SDA_OUT;		// change direction to output on SDA line

	if (last)
		I2C_SDA_HI;				// set NAK
	else
		I2C_SDA_LO;				// set ACK

	I2C_SCL_TOGGLE;				// clock pulse
	I2C_SDA_HI;					// leave with SDA HI
	return b;					// return received byte
}


//************************
//* I2C public functions *
//************************
//! Initialize I2C communication
void i2c_bus_init(void)
{
	i2cTemp(&i2c_context[I2C_TEMP]);
	i2cInitA(&i2c_context[I2C_IPMB0_A]);
	i2cInitB(&i2c_context[I2C_IPMB0_B]);
}

I2C_DATA* getBus(uint8_t i2c_bus)
{
	return &i2c_context[i2c_bus];
}

//
void i2cInitA(I2C_DATA* i2c_bus)
{
	i2c_bus->pins.scl=SCL_A;

	i2c_bus->pins.scl_ddr=&SCLDDR_A;
	i2c_bus->pins.scl_pin=&SCLPIN_A;
	i2c_bus->pins.scl_port=&SCLPORT_A;

	i2c_bus->pins.sda=SDA_A;

	i2c_bus->pins.sda_ddr=&SDADDR_A;
	i2c_bus->pins.sda_pin=&SDAPIN_A;
	i2c_bus->pins.sda_port=&SDAPORT_A;

	I2C_SDA_OUT;// set SDA as output
	I2C_SCL_OUT;// set SCL as output

	I2C_SDA_HI;					// set I/O state and pull-ups
	I2C_SCL_HI;					// set I/O state and pull-ups

	i2c_bus->I2C_state=I2C_INIT;
}
void i2cInitB(I2C_DATA* i2c_bus)
{
	i2c_bus->pins.scl=SCL_B;

	i2c_bus->pins.scl_ddr=&SCLDDR_B;
	i2c_bus->pins.scl_pin=&SCLPIN_B;
	i2c_bus->pins.scl_port=&SCLPORT_B;

	i2c_bus->pins.sda=SDA_B;

	i2c_bus->pins.sda_ddr=&SDADDR_B;
	i2c_bus->pins.sda_pin=&SDAPIN_B;
	i2c_bus->pins.sda_port=&SDAPORT_B;

	I2C_SDA_OUT;// set SDA as output
	I2C_SCL_OUT;// set SCL as output

	I2C_SDA_HI;					// set I/O state and pull-ups
	I2C_SCL_HI;					// set I/O state and pull-ups

	i2c_bus->I2C_state=I2C_INIT;
}

void i2cTemp(I2C_DATA* i2c_bus)
{
	i2c_bus->pins.scl=SCL_TEMP;

	i2c_bus->pins.scl_ddr=&SCLDDR_TEMP;
	i2c_bus->pins.scl_pin=&SCLPIN_TEMP;;
	i2c_bus->pins.scl_port=&SCLPORT_TEMP;;

	i2c_bus->pins.sda=SDA_TEMP;;

	i2c_bus->pins.sda_ddr=&SDADDR_TEMP;;
	i2c_bus->pins.sda_pin=&SDAPIN_TEMP;;
	i2c_bus->pins.sda_port=&SDAPORT_TEMP;;

	I2C_SDA_OUT;// set SDA as output
	I2C_SCL_OUT;// set SCL as output

	I2C_SDA_HI;					// set I/O state and pull-ups
	I2C_SCL_HI;					// set I/O state and pull-ups

	i2c_bus->I2C_state=I2C_INIT;


}

uint8_t i2cMasterReceive(I2C_DATA* i2c_bus,uint8_t slave_addr,uint8_t wr_len, uint8_t* wr_data,uint8_t rd_len, uint8_t* rd_data)
{
	uint8_t ret;
	int j = rd_len;
	uint8_t *p = rd_data;

	I2C_START;					// do start transition
	ret=i2cPutbyte(i2c_bus,slave_addr & 0xFE);			// send DEVICE address
	if (ret==0)
		{
			//not acknowledged
			I2C_SDA_LO;					// clear data line and
			I2C_STOP;					// send STOP transition
			return ret;
		}
	while (wr_len--)
			i2cPutbyte(i2c_bus,*wr_data++);

	HDEL;
	I2C_SCL_HI;      			// do a repeated START
	I2C_START;					// transition

	i2cPutbyte(i2c_bus,slave_addr | READ);	// resend DEVICE, with READ bit set

	// receive data bytes
	while (j--)
		*p++ = i2cGetbyte(i2c_bus,j == 0);

	I2C_SDA_LO;					// clear data line and
	I2C_STOP;					// send STOP transition

	return ret;

}

uint8_t i2c_Send(I2C_DATA* i2c_bus,uint8_t slave_addr, uint8_t wr_len, uint8_t* wr_data)
{
	uint8_t ret,i;
	printf("\n\ri2c_Send\n\raddr %X LEN: 0x%02X - ",slave_addr,wr_len);
	for(i=0;i<wr_len;i++)
	{
		 printf(" 0x%02X ",wr_data[i]);
	}
	 printf("\n\r");


	I2C_SDA_OUT;
	I2C_SCL_OUT;
	I2C_START;					// do start transition
	ret=i2cPutbyte(i2c_bus,slave_addr);			// send DEVICE address
	if (ret==0)
	{
		//not acknowledged
		I2C_SDA_LO;					// clear data line and
		I2C_STOP;					// send STOP transition

		printf("Error NACK 1 slave_addr %02X",slave_addr);
		return ret;
	}

	while (wr_len--)
	{
		ret=i2cPutbyte(i2c_bus,*wr_data++);
		//not acknowledged
		if (ret==0)
		{
    			I2C_SDA_LO;					// clear data line and
				I2C_STOP;					// send STOP transition
				printf("Error NACK Data slave_addr %02X",slave_addr);
				return ret;
		}
	}

	I2C_SDA_LO;					// clear data line and
	I2C_STOP;					// send STOP transition

	return ret;
}

uint8_t i2c_Read(I2C_DATA* i2c_bus,uint8_t slave_addr, uint8_t rd_len, uint8_t* rd_data)
{
	uint8_t ret;
	int j = rd_len;
	uint8_t *p = rd_data;

	printf("\n\ri2c_Read\n\raddr %X LEN: 0x%02X - \n\r",slave_addr,rd_len);

	I2C_SDA_OUT;
	I2C_SCL_OUT;

	I2C_START;					// do start transition
	ret=i2cPutbyte(i2c_bus,slave_addr | READ);			// send DEVICE address
	if (ret==0)
	{
		//not acknowledged
		I2C_SDA_LO;					// clear data line and
		I2C_STOP;					// send STOP transition

		printf("Error NACK 1 slave_addr %02X",slave_addr);
		return ret;
	}

// receive data bytes
	while (j--)
	{
		*p++ = i2cGetbyte(i2c_bus,j == 0);
	}

	I2C_SDA_LO;					// clear data line and
	I2C_STOP;					// send STOP transition

	return ret;
}

IPMI_WS* IPMB0_A_READ(I2C_DATA*IPMB)
{
	cbi(*IPMB->pins.sda_ddr,IPMB->pins.sda);//define as inputs
	cbi(*IPMB->pins.scl_ddr,IPMB->pins.scl);//define as inputs

	IPMI0_A_ws.i2c_byte=0;
	IPMI0_A_ws.i2c_sda=0;
	IPMI0_A_ws.i2c_bit=0;

	IPMB->I2C_state=I2C_INIT;
	 while(1)
		  {
		        toogle_gpio(PORTB,MCH_LED_YEL);
	    		switch (IPMB->I2C_state)
					{
					case I2C_INIT://is everything up? for some time
					{
						if ( ((SDAPIN & _BV(SDA_A))!=0) && ((SCLPIN & _BV(SCL_A))!=0))
						{
							IPMI0_A_ws.delivery_attempts++;
							if (IPMI0_A_ws.delivery_attempts>15)
							{
								IPMI0_A_ws.i2c_byte=0;
								IPMI0_A_ws.i2c_bit=0;
								IPMI0_A_ws.len_in=0;
								IPMB->I2C_state=I2C_START_BIT;
								IPMI0_A_ws.delivery_attempts=0;
							}
						}
						//toogle_gpio(PORTB,MCH_LED_YEL);
						//IPMI0_A_ws.delivery_attempts=0;

						break;
					}

					case I2C_START_BIT: //START BIT
					{
						if ( ((SCLPIN & _BV(SCL_A))!=0) )
						{
							if (((SDAPIN & _BV(SDA_A))==0))
							{
								IPMB->I2C_state=I2C_WAIT_SCL;
								IPMI0_A_ws.incoming_protocol=IPMI_CH_PROTOCOL_IPMB;
								sbi(PORTB,MCH_LED_RED);
								IPMI0_A_ws.delivery_attempts=0;
								break;
							}
							IPMI0_A_ws.delivery_attempts++;
							if ((IPMI0_A_ws.delivery_attempts>640000)) IPMB->I2C_state=I2C_TIMEOUT_I2C;
						}
						if ((SCLPIN & _BV(SCL_A))==0)
						{
							IPMB->I2C_state=I2C_INIT;

							break;

						}
						//toogle_gpio(PORTB,MCH_LED_YEL);
						break;
					}
					case I2C_WAIT_SCL:
					{
						sda_pin=SDAPIN & _BV(SDA_A);
						scl_pin=SCLPIN & _BV(SCL_A);

						if ((scl_pin )==0)
						{
							IPMB->I2C_state=I2C_GET_BIT;
							break;
						}

						if ((scl_pin)!=0)
						{
							if ((( sda_pin )!=0)&&(IPMI0_A_ws.i2c_sda==0))
							{
								cbi(PORTB,MCH_LED_RED);//stop bit;
								//ws_set_state( &ws, WS_ACTIVE_MASTER_READ );
								IPMB->I2C_state=I2C_STOP_BIT;
							}

						}

						//up++;
						//if (up>45000) goto begin_loop;
						//toogle_gpio(PORTB,MCH_LED_YEL);
						break;
					}
					case I2C_GET_BIT:
					{
						if ((SCLPIN & _BV(SCL_A))!=0)
						{
							//toogle_gpio(PORTB,MCH_LED_YEL);
							IPMI0_A_ws.i2c_sda = (SDAPIN) & _BV(SDA_A);

							IPMI0_A_ws.i2c_byte <<= 1;
							if(IPMI0_A_ws.i2c_sda) IPMI0_A_ws.i2c_byte |=0x01;
							IPMI0_A_ws.i2c_bit++;

							if (IPMI0_A_ws.i2c_bit>7)
							{
								IPMB->I2C_state=I2C_WAIT_SCL;
								IPMI0_A_ws.pkt_in[IPMI0_A_ws.len_in++]=IPMI0_A_ws.i2c_byte;

								if (IPMI0_A_ws.len_in==0)
								{
									if (IPMI0_A_ws.i2c_byte!=0x20)
									{
										printfr(PSTR("WRG ADDRS\n\r"));
										IPMB->I2C_state=I2C_WRG_ADDR; //not a frame for this machine, go and wait for a new start sign
									}
								}


							//send ack during the next SCL cycle
								IPMI0_A_ws.sclbit_timeout=0;
								while(((SCLPIN & _BV(SCL_A)))!=0)
								{
									IPMI0_A_ws.sclbit_timeout++;
									if (IPMI0_A_ws.sclbit_timeout>300) {
									//	toogle_gpio(PORTB,MCH_LED_YEL);
										IPMB->I2C_state=I2C_TIMEOUT_STOP;
										goto i2c_timeout_stop;
									}
								};

								sbi(SDADDR, SDA_A);//define as output
								cbi(SDAPORT,SDA_A); //Send ACK
								IPMI0_A_ws.sclbit_timeout=0;

								while(((SCLPIN & _BV(SCL_A)))==0)
								{
									IPMI0_A_ws.sclbit_timeout++;
									if (IPMI0_A_ws.sclbit_timeout>300) {
									//	toogle_gpio(PORTB,MCH_LED_YEL);
										IPMB->I2C_state=I2C_TIMEOUT_STOP;
										goto i2c_timeout_stop;
									}
								};// wait for next down edge

								IPMI0_A_ws.sclbit_timeout=0;
								while(((SCLPIN & _BV(SCL_A)))!=0)
								{
									IPMI0_A_ws.sclbit_timeout++;
									if (IPMI0_A_ws.sclbit_timeout>300)
									{
									//	toogle_gpio(PORTB,MCH_LED_YEL);
										IPMB->I2C_state=I2C_TIMEOUT_STOP;
										goto i2c_timeout_stop;
									}
								};// wait for next up edge

								cbi(SDADDR, SDA_A);//define as inputs
								sbi(SDAPORT,SDA_A);

								IPMI0_A_ws.i2c_byte=0;
								IPMI0_A_ws.i2c_bit=0;

							}
							else
							{

								IPMB->I2C_state=I2C_WAIT_SCL;

							}



						}
					//	toogle_gpio(PORTB,MCH_LED_YEL);
					}
						break;

		           case I2C_TIMEOUT_I2C:
		        	   printfr(PSTR("timeout"));
						IPMI0_A_ws.delivery_attempts=0;
						return 0;
						break;

	i2c_timeout_stop: case I2C_TIMEOUT_STOP:
						printfr(PSTR("timeout stop"));
						IPMI0_A_ws.sclbit_timeout=0;
						IPMB->I2C_state=I2C_INIT;
						break;

					case I2C_WRG_ADDR:
						return 0;
						break;
					case I2C_STOP_BIT:
						printfr(PSTR("stop_bit"));
						return &IPMI0_A_ws;

						break;
					default:
					{
						printfr(PSTR("Default"));
						IPMB->I2C_state=I2C_INIT;
						break;
					}
				}


			}

	 return &IPMI0_A_ws;

}

/*
 * i2c_master_write()
 * 	Gets called by ws_process_work_list() when we find a ws with state
 * 	WS_ACTIVE_MASTER_WRITE_PENDING in the work list.
 */
void i2c_master_write( IPMI_WS *ws )
{
	uint8_t ret;
	ret=i2c_Send((I2C_DATA*)ws->I2C_BUS,ws->pkt_out[0],ws->len_out, &ws->pkt_out[1]);
	if (ret==0)
	{
		ws->delivery_attempts++;
		ws_set_state( ws, WS_ACTIVE_MASTER_WRITE);
	}
	else
		ws_set_state( ws, WS_ACTIVE_IN);

}


void i2c_master_read( IPMI_WS *ws )
{

    uint8_t ret,i;
    uint8_t data_wr[0];
    data_wr[0]=0x00;


	ret=i2c_Read((I2C_DATA*)ws->I2C_BUS,ws->addr_out,ws->len_rcv, &ws->pkt_in[0]);
	//ret=i2cMasterReceive((I2C_DATA*)ws->I2C_BUS,ws->addr_out,1, data_wr,ws->len_rcv, &ws->pkt_in[0]);

	if (ret==0)
	{
		printf("retry again\n\r");
		ws->delivery_attempts++;
		ws_set_state( ws, WS_ACTIVE_MASTER_READ);
	}
	else
	{
		printf("ws->pkt_in ");
		for(i=0;i<ws->len_rcv;i++)
		{
				 printf(" 0x%02X ",ws->pkt_in[i]);
		}
		printf("\n\r");

		ws_set_state( ws, WS_ACTIVE_IN);
	}

}
