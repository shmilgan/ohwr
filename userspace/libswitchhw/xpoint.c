#include <stdio.h>
#include <stdlib.h>

#include <sys/time.h>

#include <hw/switch_hw.h>
#include <hw/trace.h>
#include <hw/clkb_io.h>
#include <hw/pio.h>

// The I2C bus which goes to ADN4600 is currently connected to the GPIO pins of CLKB FPGA.

#define MASK_SCL_OUT (1<<0)
#define MASK_SDA_OUT (1<<1)

#define MASK_SCL_IN (1<<2)
#define MASK_SDA_IN (1<<3)

#define M_SDA_OUT(x) {							\
    if(x)								\
      shw_clkb_write_reg(CLKB_BASE_GPIO + GPIO_REG_SODR, MASK_SDA_OUT);	\
    else								\
      shw_clkb_write_reg(CLKB_BASE_GPIO + GPIO_REG_CODR, MASK_SDA_OUT);	\
  }

#define M_SCL_OUT(x) {							\
    if(x)								\
      shw_clkb_write_reg(CLKB_BASE_GPIO + GPIO_REG_SODR, MASK_SCL_OUT);	\
    else								\
      shw_clkb_write_reg(CLKB_BASE_GPIO + GPIO_REG_CODR, MASK_SCL_OUT);	\
  }

#define M_SDA_IN (shw_clkb_read_reg(CLKB_BASE_GPIO + GPIO_REG_PSR) & MASK_SDA_IN)


// ADN4600 register addresses

#define ADN4600_ADDR 0x90

#define ADN4600_RESET 0x0
#define ADN4600_XPT_CONFIG 0x40
#define ADN4600_XPT_UPDATE 0x41

#define ADN4600_RX_CONFIG(channel) (0x80 + channel * 8)
#define ADN4600_TX_CONFIG(channel) (((channel<4)?(0xc0 + channel * 8) : (0xe0 + (7-channel)*8)))



// crosspoint I/O map:
// up0 tx -> IN1 inverted
// up1 tx -> IN0 inverted
// sfp0 tx -> IN2 inverted
// sfp1 tx -> IN3 inverted
// up0 rx <- OUT0
// up1 rx <- OUT1
// sfp0 rx <- OUT7
// sfp1 rx <- OUT6
// feedback <- OUT3

#define XPT_UP0_TX 1
#define XPT_UP1_TX 0
#define XPT_SFP0_TX 2
#define XPT_SFP1_TX 3

#define XPT_UP0_RX 0
#define XPT_UP1_RX 1
#define XPT_SFP0_RX  7
#define XPT_SFP1_RX  6
#define XPT_FEEDBACK  3

#define RX_CONFIG_PNSWAP 0x40
#define RX_CONFIG_RXEN 0x10
#define RX_CONFIG_EQBY 0x20
#define RX_CONFIG_RXEQ(eq) (eq&0x7)

#define TX_CONFIG_TXEN 0x20
#define TX_CONFIG_TXDR 0x10
#define TX_CONFIG_TXPE(pe) (pe&0x7)


static void mi2c_start()
{
  M_SDA_OUT(0);
  M_SCL_OUT(0);
}

static void mi2c_repeat_start()
{
  M_SDA_OUT(1);
  M_SCL_OUT(1);
  M_SDA_OUT(0);
  M_SCL_OUT(0);
}

static void mi2c_stop()
{
  M_SDA_OUT(0);
  M_SCL_OUT(1);
  M_SDA_OUT(1);
}

static unsigned char mi2c_put_byte(unsigned char data)
{
  char i;
  unsigned char ack;

  for (i=0;i<8;i++, data<<=1)
    {
      M_SDA_OUT(data&0x80);
      M_SCL_OUT(1);
      M_SCL_OUT(0);
    }

  M_SDA_OUT(1);
  M_SCL_OUT(1);

  ack = M_SDA_IN;	/* ack: sda is pulled low ->success.	 */

  M_SCL_OUT(0);
  M_SDA_OUT(0);

  return ack!=0;
}

static void mi2c_get_byte(unsigned char *data)
{

  int i;
  unsigned char indata = 0;

  /* assert: scl is low */
  M_SCL_OUT(0);

  for (i=0;i<8;i++)
    {
      M_SCL_OUT(1);
      indata <<= 1;
      if ( M_SDA_IN ) indata |= 0x01;
      M_SCL_OUT(0);
    }

  M_SDA_OUT(1);
  *data= indata;
}

static void mi2c_init()
{
  shw_clkb_write_reg(CLKB_BASE_GPIO + GPIO_REG_DDR, MASK_SCL_OUT | MASK_SDA_OUT);

  M_SCL_OUT(1);
  M_SDA_OUT(1);
}

static uint8_t adn4600_read_reg(uint8_t reg)
{
  uint8_t rval;

  mi2c_start();
  if(mi2c_put_byte(ADN4600_ADDR)) goto rxerr;
  if(mi2c_put_byte(reg)) goto rxerr;

  mi2c_repeat_start();
  if(mi2c_put_byte(ADN4600_ADDR|1)) goto rxerr;
  mi2c_get_byte(&rval);
  mi2c_stop();

  return rval;

 rxerr:
  TRACE(TRACE_ERROR, "No I2C ack from ADN4600!");
  mi2c_stop();
  return 0;
}

static void adn4600_write_reg(uint8_t reg, uint8_t value)
{
  uint8_t rval;


  mi2c_start();

  if(mi2c_put_byte(ADN4600_ADDR)) goto txerr;

  if(mi2c_put_byte(reg)) goto txerr;

  if(mi2c_put_byte(value)) goto txerr;

  mi2c_stop();

 txerr:
  TRACE(TRACE_FATAL, "No I2C ack from ADN4600!");
  mi2c_stop();
}


void xpoint_route(int in, int out)
{
  adn4600_write_reg(ADN4600_XPT_CONFIG, (in<<4) | out);
  adn4600_write_reg(ADN4600_XPT_UPDATE, 1);
}

int xpoint_configure()
{
  TRACE(TRACE_INFO, "Initializing ADN4600 crosspoint");
  mi2c_init();

  adn4600_write_reg(ADN4600_RESET, 0x1);

// Uplink 1 TX
  adn4600_write_reg(ADN4600_RX_CONFIG(0), RX_CONFIG_PNSWAP | RX_CONFIG_RXEN | RX_CONFIG_RXEQ(0));
  adn4600_write_reg(ADN4600_RX_CONFIG(1), RX_CONFIG_PNSWAP | RX_CONFIG_RXEN | RX_CONFIG_RXEQ(0));
  adn4600_write_reg(ADN4600_RX_CONFIG(2), RX_CONFIG_PNSWAP | RX_CONFIG_RXEN | RX_CONFIG_RXEQ(0));
  adn4600_write_reg(ADN4600_RX_CONFIG(3), RX_CONFIG_PNSWAP | RX_CONFIG_RXEN | RX_CONFIG_RXEQ(0));
  adn4600_write_reg(ADN4600_RX_CONFIG(4), 0);
  adn4600_write_reg(ADN4600_RX_CONFIG(5), 0);
  adn4600_write_reg(ADN4600_RX_CONFIG(6), 0);
  adn4600_write_reg(ADN4600_RX_CONFIG(7), 0);

  adn4600_write_reg(ADN4600_TX_CONFIG(0), TX_CONFIG_TXEN | TX_CONFIG_TXPE(0));
  adn4600_write_reg(ADN4600_TX_CONFIG(1), TX_CONFIG_TXEN | TX_CONFIG_TXPE(0));
  adn4600_write_reg(ADN4600_TX_CONFIG(6), TX_CONFIG_TXEN | TX_CONFIG_TXPE(0));
  adn4600_write_reg(ADN4600_TX_CONFIG(7), TX_CONFIG_TXEN | TX_CONFIG_TXPE(0));
  adn4600_write_reg(ADN4600_TX_CONFIG(XPT_FEEDBACK), TX_CONFIG_TXPE(0));

  xpoint_route(XPT_UP1_TX, XPT_SFP1_RX);
  xpoint_route(XPT_UP0_TX, XPT_SFP0_RX);

  xpoint_route(XPT_SFP1_TX, XPT_UP1_RX);
  xpoint_route(XPT_SFP0_TX, XPT_UP0_RX);

  return 0;
}


// port: 0 = uplink0, 1 = uplink 1
// txrx: 0 = tx calibration, 1 = rx calibration
// on: 0 = disable, 1= enable

void xpoint_cal_feedback(int on, int port, int txrx)
{
  int port_map[] = { XPT_UP0_TX,
		     XPT_SFP0_TX,
		     XPT_UP1_TX,
		     XPT_SFP1_TX };

  int index = (port * 2 + (txrx ? 1 : 0));

  if(!on)
    {
     adn4600_write_reg(ADN4600_TX_CONFIG(XPT_FEEDBACK), TX_CONFIG_TXPE(0));
   } else {
    xpoint_route(port_map[index], XPT_FEEDBACK);
    adn4600_write_reg(ADN4600_TX_CONFIG(XPT_FEEDBACK), TX_CONFIG_TXEN | TX_CONFIG_TXPE(0));
  }
}

