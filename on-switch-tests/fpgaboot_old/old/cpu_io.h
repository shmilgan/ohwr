#ifndef __CPU_IO_H
#define __CPU_IO_H

#include <sys/types.h>
#include <inttypes.h>

#include <at91/at91sam9263.h>
#include <at91/at91_pio.h>
#include <at91/at91_pmc.h>

#define NUM_PIO_BANKS 5

#define PIOA 0
#define PIOB 1
#define PIOC 2
#define PIOD 3
#define PIOE 4

#define PIN_MODE_GPIO 0
#define PIN_MODE_PERIPH_A 1
#define PIN_MODE_PERIPH_B 2

#define PIN_MODE_PULLUP 0x80

#define _writel(reg, val){ *(volatile uint32_t *)(reg) = (val); }
#define _readl(reg) (*(volatile uint32_t *)(reg))

int io_init();
void pio_set_state(int port, int pin, int state);
void pio_set_mode(int port, int pin, int mode, int dir);
int pck_enable(int pck_num, int prescaler, int source);
int pio_get_state(int port, int pin);
volatile uint8_t *io_get_sys_base();
volatile uint8_t *pio_get_port_addr(int port);

#endif
