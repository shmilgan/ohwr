#include <board.h>
#include <pio.h>

#include "mi2c.h"

#define MASK_SCL (1<<11)
#define MASK_SDA (1<<10)

#define M_SDA_OUT(x)\
    if(x) PIO->PIO_ODR = MASK_SDA;\
    else  PIO->PIO_OER = MASK_SDA;

#define M_SCL_OUT(x)\
    if(x) PIO->PIO_ODR = MASK_SCL;\
    else  PIO->PIO_OER = MASK_SCL;

#define M_SDA_IN (PIO->PIO_PDSR & MASK_SDA)

#define M_DELAY { int __i; for(__i = 0; __i < 15; __i++) asm volatile("nop"); }

static AT91S_PIO *PIO = (AT91S_PIO*) AT91C_BASE_PIOA;


void mi2c_start()
{
  M_SDA_OUT(0); M_DELAY;
  M_SCL_OUT(0); M_DELAY;
}

void mi2c_repeat_start()
{
  M_SDA_OUT(1); M_DELAY;
  M_SCL_OUT(1); M_DELAY;
  M_SDA_OUT(0); M_DELAY;
  M_SCL_OUT(0); M_DELAY;
}

void mi2c_stop()
{
  M_SDA_OUT(0); M_DELAY;
  M_SCL_OUT(1); M_DELAY;
  M_SDA_OUT(1); M_DELAY;
}

unsigned char mi2c_put_byte(unsigned char data)
{
  char i;
  int ack;

  for (i=0;i<8;i++, data<<=1)
    {
      M_SDA_OUT(data&0x80); M_DELAY;
      M_SCL_OUT(1); M_DELAY;
      M_SCL_OUT(0); M_DELAY;
    }

  M_SDA_OUT(1); M_DELAY;
  M_SCL_OUT(1); M_DELAY;

  ack = M_SDA_IN;	/* ack: sda is pulled low ->success.	 */
 M_DELAY;
  M_SCL_OUT(0); M_DELAY;
  M_SDA_OUT(0); M_DELAY;
  return ack!=0;
}

void mi2c_get_byte(unsigned char *data)
{

  int i;
  unsigned char indata = 0;

  /* assert: scl is low */
  M_SCL_OUT(0); M_DELAY;

  for (i=0;i<8;i++)
    {
      M_SCL_OUT(1); M_DELAY;
      indata <<= 1;
      if ( M_SDA_IN ) indata |= 0x01;
      M_SCL_OUT(0); M_DELAY;
    }

  M_SDA_OUT(1); M_DELAY;
  *data= indata;
}

void mi2c_init()
{
 const Pin PIN_mb_scl = {1<<11, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};
 const Pin PIN_mb_sda = {1<<10, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};
 PIO_Configure(&PIN_mb_scl, 1);
 PIO_Configure(&PIN_mb_sda, 1);
 M_SCL_OUT(1); M_DELAY;
 M_SDA_OUT(1); M_DELAY;
}

