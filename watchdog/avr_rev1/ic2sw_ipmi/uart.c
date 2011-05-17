#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "uart.h"
#include "cmdline.h"
#include "string.h"

#define MAX_CHARS 10

// enable and initialize the uart
void uartInit(void)
{
    // enable RxD/TxD and interrupts
	UCSR1B |= _BV(RXEN1)|_BV(TXEN1)|_BV(RXCIE);

	// enable interrupts
   //sei();
}
void printfr(const prog_char str[])
{

	// print a string stored in program memory
	register char c;

	// check to make sure we have a good pointer
	if (!str) return;

	// print the string until the null-terminator
	while((c = pgm_read_byte(str++)))
		uart_putc(c);
}

// set the uart baud rate
void uartSetBaudRate(uint8_t baudrate)
{
	// calculate division factor for requested baud rate, and set it
	/*
	  14.4k 34 -0.8%
	  19.2k 25 0.2%
	  28.8k 16 2.1%
	  38.4k 12 0.2%
	  57.6k 8 -3.5%
      76.8k 6 -7.0%
     115.2k 3 8.5%
     230.4k 1 8.5%
     250  k 1 0.0%
	 */
	UBRR1H= 0;
	UBRR1L= baudrate;

}

// transmits a byte over the uart
void uart_putc(uint8_t c)
{
	while (!(UCSR1A & (1<<UDRE1)));
	UDR1 = c;
}

void uart_putstr(uint8_t *str) {
	while(*str) {
		uart_putc(*str++);
	}
}

uint8_t uart_getc(void)
{
	while (!(UCSR1A & (1<<RXC1)));
	return UDR1;
}

uint8_t uart_getc_noloop(void)
{
	if ((UCSR1A & (1<<RXC1)))
	{
		return UDR1;
	}
	else
		return 0x00;
}

// UART Receive Complete Interrupt Handler
SIGNAL(SIG_UART1_RECV)
{
	uint8_t c;

	// get received char
	c = UDR1;

	cmdlineInputFunc(c);

	if (c==0xd)
	{
		uart_putc('\n');
	}
	uart_putc(c);

}



