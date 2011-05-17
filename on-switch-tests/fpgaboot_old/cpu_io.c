#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <fcntl.h>

#include "cpu_io.h"


static volatile uint8_t *sys_base;
static volatile uint8_t *pio_base[NUM_PIO_BANKS];


int io_init()
{
  int fd = open("/dev/mem", O_RDWR);
  
  if(!fd) return -1;
  sys_base = mmap(NULL, 0x2000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, AT91_BASE_SYS);

  if(sys_base == NULL) 
  {
	perror("mmap()");
	close(fd);
	return -1;
  }
  
//  fprintf(stderr,"AT91_SYS mmapped to: 0x%08x\n", sys_base);

  pio_base[0] = sys_base + AT91_PIOA;
  pio_base[1] = sys_base + AT91_PIOB;
  pio_base[2] = sys_base + AT91_PIOC;
  pio_base[3] = sys_base + AT91_PIOD;
  pio_base[4] = sys_base + AT91_PIOE;
  
  return 0;
}



void pio_set_state(int port, int pin, int state)
{
  if(state)
	_writel(pio_base[port] + PIO_SODR, (1<<pin))
  else
  	_writel(pio_base[port] + PIO_CODR, (1<<pin))
  
}

void pio_set_mode(int port, int pin, int mode, int dir)
{
  _writel(pio_base[port] + PIO_IDR, (1<<pin));	// disable irq

  if(mode & PIN_MODE_PULLUP)
	_writel(pio_base[port] + PIO_PUER, (1<<pin))	// enable pullup
  else
    _writel(pio_base[port] + PIO_PUDR, (1<<pin))	// disable pullup
  	
  switch(mode & 0x3)
  {
	case PIN_MODE_GPIO: 
	  _writel(pio_base[port] + PIO_PER, (1<<pin));	// enable gpio mode
	  break;
	case PIN_MODE_PERIPH_A:
	  _writel(pio_base[port] + PIO_PDR, (1<<pin));	// disable gpio mode
	  _writel(pio_base[port] + PIO_ASR, (1<<pin));	// select peripheral A
	  break;
	case PIN_MODE_PERIPH_B:
	  _writel(pio_base[port] + PIO_PDR, (1<<pin));	// disable gpio mode
	  _writel(pio_base[port] + PIO_BSR, (1<<pin));	// select peripheral B
	  break;
  }

  if(dir)
    _writel(pio_base[port] + PIO_OER, (1<<pin))	// select output
  else
    _writel(pio_base[port] + PIO_ODR, (1<<pin));	// select input

}


static const struct {
  int port;
  int pin;
  int mode;
} pck_port_mapping[] =
{
  {PIOA, 13, PIN_MODE_PERIPH_B},
  {PIOB, 10, PIN_MODE_PERIPH_B},
  {PIOA, 6, PIN_MODE_PERIPH_B},
  {PIOE, 11, PIN_MODE_PERIPH_B}
};

int pck_enable(int pck_num, int prescaler, int source)
{
  if(pck_num > 3) return -1;
  
  pio_set_mode(pck_port_mapping[pck_num].port, pck_port_mapping[pck_num].pin, pck_port_mapping[pck_num].mode, 1);
  
  _writel(sys_base + AT91_PMC_PCKR(pck_num), source | prescaler);
  
// enable the PCK output
  _writel(sys_base + AT91_PMC_SCER, (1<< (8+pck_num)));

  return 0;
}

int pio_get_state(int port, int pin)
{
  return (_readl(pio_base[port] + PIO_PDSR) & (1<<pin)) ? 1: 0;
}


volatile uint8_t *io_get_sys_base()
{
  return sys_base;
}

volatile uint8_t *pio_get_port_addr(int port)
{
  return pio_base[port];
}
