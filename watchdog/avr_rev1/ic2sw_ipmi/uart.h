#ifndef UART_H
#define UART_H
#include "inttypes.h"

#define UART_BAUD_CALC(UART_BAUD_RATE) ((F_CPU)/((UART_BAUD_RATE)*16L)-1)

void uartSetBaudRate(uint8_t baudrate);
void uartInit(void);
void uart_putc(uint8_t c);
void uart_putstr(uint8_t *str);
uint8_t uart_getc(void);
uint8_t uart_getc_noloop(void);
void printfr(const prog_char str[]);

#endif
//@}


