/*
 * main.c
 *
 *  Created on: 16-Jun-2009
 *      Author: poliveir
 */


#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include "uart.h"
#include "ipmi.h"
#include "i2csw.h"
#include "ws.h"
#include "module.h"
#include "cmdline.h"
#include "global.h"
#include "timer.h"
#include "tm100.h"

void pwrFunction(void);
void tooglepwr(void);



typedef struct led{
	volatile uint8_t* ddr;
	volatile uint8_t* port;
	volatile uint8_t* pin;
	volatile uint8_t io;
}LED;


LED leds[2];


char cmd[32];

uint8_t print_buffer[100];

uint8_t IMPI_ADDR[] ={0xA8,0xAA,0xC2,0xC4,0xC8};

#define MAX_IPMI_BUFF_SIZE       31

IPMI_WS ws;
IPMI_WS* wsp;
uint8_t send_cmd;
//for stdout
int put(char c, FILE * foo){
	uart_putc(c);
	return 0;
}

//for stdout
int get( FILE * foo){
	return uart_getc();

}

void initLeds(void)
{


	leds[0].ddr=&DDRB;
	leds[0].port=&PORTB;
	leds[0].pin=&PINB;
	leds[0].io=PB4;

	leds[1].ddr=&DDRB;
	leds[1].port=&PORTB;
	leds[1].pin=&PINB;
	leds[1].io=PB5;
}

void boot_FPGA(void)
{
#define Flash_Main_SEL      PB6
#define Main_nRST           PG1
#define Flash_Boot_SEL      PG0

#define UTCA_HS_SWITCH      PC0
#define UTCA_ENABLE	        PC5
/*
 * THE MCH can the assert its PWR_O signal when its ejector switch is closed.
 * This active high signal is detected by the PM with autonomous mode enabled, which then enables Payload Power
 * to the respective MCH.
 */



#define UTCA_PWRON PC4


	DDRB |= _BV(Flash_Main_SEL); //Flash_Boot_SEL PB6 output
	DDRC |= _BV(UTCA_PWRON); //UTCA_PWRON PC4 output
	DDRG |= _BV( Main_nRST); //UTCA_PWRON PC4 output
	DDRG |= _BV( Flash_Boot_SEL); //UTCA_PWRON PC4 output

	DDRC &= ~_BV(UTCA_ENABLE); //  UTCA_ENABLE PC5 input
	PORTC|= _BV(UTCA_ENABLE); //   Pull-up


	PORTB|= _BV(Flash_Main_SEL); //Flash_Main_Sel
	PORTG|= _BV(Flash_Boot_SEL); //Flash_Main_Sel



	_delay_ms(500);

	PORTC|= _BV(UTCA_PWRON); //Utca_Pwron;
	_delay_ms(1000);

	while((PINC & _BV(UTCA_ENABLE))!=0);

	PORTB&= ~_BV(Flash_Main_SEL); //Flash_Main_Sel

	//PORTB&= ~_BV(Flash_Boot_SEL); //Flash_Main_Sel

	_delay_ms(500);

	PORTG|= _BV(Main_nRST);

	_delay_ms(1000);

}

uint8_t scl_pin,sda_pin;

int main(void)
{
	uint8_t delay,ret,i;


	uint16_t sample;
	uint8_t Seq;

	timerInit();

	uartInit();
	initLeds();
	//set baudrate to 38.4k  -- 12
	//set baudrate to 115.2k -- 3
	//set baudrate to 230400 -- 1
	uartSetBaudRate(12);//38.4k/(8e6)/((16)*(4800))-1




	//sei();

	fdevopen(put, get);



	printf("\x1B[2JLast Compilation %s %s\n\r",__DATE__,__TIME__);

	printfr(PSTR("Houston, we have take off! i2csw ipmi prot\n\r"));


	sbi(*leds[0].ddr,leds[0].io); //  Output

	sbi(*leds[1].ddr,leds[1].io); //  Output

	sbi(*leds[0].port,leds[0].io); //  Output

	cbi(*leds[1].port,leds[1].io); //  Output

	delay=3;
	sample=0;

	while(delay--)
	{
		_delay_ms(1000);
	    printf("Press F2 for bootloader %02d sec\r",delay);// F1 = 0x1B 0x4f 0x50
	                                                       // F2 = 0x1B 0x4f 0x51
	}

	printfr(PSTR("\nMain Flash Booting\n\r"));
	boot_FPGA();

	_delay_ms(1000);

	printfr(PSTR("I2C Software\r"));
	ws_init();
	i2c_bus_init();
	i=0;
    Seq=0;

    cmdlineAddCommand("help",		helpFunction);
    cmdlineAddCommand("pwr",		pwrFunction);
    cmdlineAddCommand("send_fru",		helpFunction);
    cmdlineAddCommand("toogle_pwr",		tooglepwr);

    printfr(PSTR("IPMI Initialization\n\r"));
  //  IMPI_ADDR[0]=0x00;
    _delay_ms(2000);

    printf("Enable Interrupts\n\r");
    sei();


    printf("Module Init\n\r");
    module_init();

	while(1)
	{

		fflush( stdin );
		sample++;
		toogle_gpio(*leds[1].port,leds[1].io );
		toogle_gpio(*leds[0].port,leds[0].io );
//		printf("sample %05d \n\r",sample);

		//printf("sample: %04d \n\r",sample);
		goto begin_loop;
ret_start:


       _delay_ms(500);

		//tm100_update_sensor(GetTM100SensorInfo(0));


#define NONE
		uint8_t dev_addr;

		dev_addr = IMPI_ADDR[2];

		cmdlineMainLoop();

		ws_process_work_list();

		if (send_cmd)
		{
			goto send_cmd;
		}
		else
		{
			goto begin_loop;
		}

send_cmd:
		send_cmd=0;
		printf("\n\rOUT LEN: 0x%02X - ",ws.len_out);
		for(i=0;i<ws.len_out;i++)
		{
			printf(" 0x%02X ",ws.pkt_out[i]);
		}
		printfr(PSTR("\n\rSending IPMI MSG\n\r"));


		ret=i2c_Send(getBus(I2C_IPMB0_A),ws.pkt_out[0],ws.len_out, &ws.pkt_out[1]);

			if(ret==0)
			{
				printf("Complete msg %02Xh: Nack: %03d \n\r",ws.pkt_out[0], ret);
				_delay_ms(100);
				goto ret_start;
			}
			else
			{
				printf("Complete msg %02Xh: ack: %03d \n\r",ws.pkt_out[0], ret);

			}


		//ret=i2cMasterReceive(IMPI_ADDR[0],localTxBufferLength, localTxBuffer,0x10,localRxBuffer);
begin_loop:

		printfr(PSTR("\n\rpolling I2C\n\r"));


		wsp=&ws;

#define SLAVE_I2C

#define NR_ATTEMPS   65000
#ifdef SLAVE_I2C
		ws.delivery_attempts=0;


#define SDADDR SDADDR_A
#define SDAPIN SDAPIN_A
#define SDAPORT SDAPORT_A

#define SCLPIN SCLPIN_A
#define SCLPORT SCLPORT_A

		wsp=IPMB0_A_READ(getBus(I2C_IPMB0_A));

     	if (wsp==0)
			 goto ret_start;


		printf("\n\rIN LEN: 0x%02X - ",wsp->len_in);
		for(i=0;i<wsp->len_in;i++)
		{
			printf(" 0x%02X ",wsp->pkt_in[i]);
		}


	   ipmi_process_pkt(wsp);

	   if (wsp->ws_state==WS_ACTIVE_MASTER_WRITE)//we have a response here!
	   {


			uint8_t retries=0;

new_tras:	ret=i2c_Send(getBus(I2C_IPMB0_A),wsp->pkt_out[0],wsp->len_out, &wsp->pkt_out[1]);

			if(ret==0)
			{
				printf("\n\r0x%02Xh: Nack: %03d \n\r",wsp->pkt_out[0], ret);
				retries++;
				if (retries<8)
					goto new_tras;
			}
			else
			{
				printf("\n\r0x%02Xh: ack: %03d goto ret_stard\n\r",ws.pkt_out[0], ret);

				goto begin_loop;

			}
		goto begin_loop;
	  }
	  goto begin_loop;

#endif



			/*
			for(i=0;i<10;i++)
			{
				printf("%02X : %02Xh\n\r",i,localRxBuffer[i]);

			}*/
			//
			// format buffer
			//localBuffer[0x04] = 0;

			//i++;
			//if (i>7) i=0;
	}
	return 0;
}

void tooglepwr(void)
{
	printf("\n\rtoogle_gpio %X\n\r",(PINC & _BV(UTCA_PWRON))!=0);
	toogle_gpio(PORTC,UTCA_PWRON); //Utca_Pwron;

}

void pwrFunction(void)
{

	uint8_t dev_addr;
	uint8_t channel,ch_count;


	dev_addr=cmdlineGetArgInt(1);
	channel=cmdlineGetArgInt(2);
	ch_count=cmdlineGetArgInt(3);
	//printf("pwrFunction");
	printf("send_pwr_ctrl_ch %d %d %d",dev_addr,channel,ch_count);

	send_pwr_ctrl_ch(&ws,0,dev_addr,channel,ch_count,0,cmd_complete);
	send_cmd=1;

}
