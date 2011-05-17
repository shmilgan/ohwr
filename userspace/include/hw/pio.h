#ifndef __CPU_IO_H
#define __CPU_IO_H

#include <sys/types.h>
#include <inttypes.h>

#include <at91/at91sam9263.h>
#include <at91/at91_pio.h>
#include <at91/at91_pmc.h>

#include <hw/trace.h>

#define NUM_PIO_BANKS 6

#define PIOA 1
#define PIOB 2
#define PIOC 3
#define PIOD 4
#define PIOE 5
#define PIO_FPGA 6

#define PIO_MODE_GPIO 0
#define PIO_MODE_PERIPH_A 1
#define PIO_MODE_PERIPH_B 2
#define PIO_MODE_PULLUP 0x80

#define PIO_OUT 1
#define PIO_OUT_0 1
#define PIO_OUT_1 2
#define PIO_IN 0

#define _writel(reg, val) *(volatile uint32_t *)(reg) = (val)
#define _readl(reg) (*(volatile uint32_t *)(reg))

typedef struct pio_pin
{
	int port;
	int pin;
	int mode;
	int dir;
} pio_pin_t;

#define REG_BASE 0
#define REG_CODR 1
#define REG_SODR 2
#define REG_PDSR 3

extern volatile uint8_t *_sys_base;
extern volatile uint8_t *_pio_base[4][NUM_PIO_BANKS+1];

int shw_pio_init();
void shw_pio_configure(const pio_pin_t *pin);
void shw_pio_configure_pins(const pio_pin_t *pins);

int shw_clock_out_enable(int pck_num, int prescaler, int source);
volatile uint8_t *shw_pio_get_sys_base();
volatile uint8_t *shw_pio_get_port_base(int port);
void shw_set_fp_led(int led, int state);



static inline void shw_pio_set(const pio_pin_t *pin, int state)
{

  if(state)
	_writel(_pio_base[REG_SODR][pin->port], (1<<pin->pin));
  else
  	_writel(_pio_base[REG_CODR][pin->port], (1<<pin->pin));
}

static inline void shw_pio_set1(const pio_pin_t *pin)
{

//TRACE(TRACE_INFO,"pio_set1 %x\n", _pio_base[REG_SODR][pin->port]);
	_writel(_pio_base[REG_SODR][pin->port], (1<<pin->pin));
}

static inline void shw_pio_set0(const pio_pin_t *pin)
{
	_writel(_pio_base[REG_CODR][pin->port], (1<<pin->pin));
}

static inline int shw_pio_get(const pio_pin_t *pin)
{
    return (_readl(_pio_base[REG_PDSR][pin->port]) & (1<<pin->pin)) ? 1 : 0;
}

static inline int shw_pio_setdir(const pio_pin_t *pin, int dir)
{
  if(dir == PIO_OUT)
	_writel((_pio_base[REG_BASE][pin->port] + PIO_OER), (1<<pin->pin));
  else
  	_writel((_pio_base[REG_BASE][pin->port] + PIO_ODR), (1<<pin->pin));

	return 0;
}

#include "pio_pins.h"

#endif
