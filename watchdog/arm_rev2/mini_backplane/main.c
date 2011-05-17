#include <board.h>
#include <pio.h>
#include <dbgu.h>
#include <aic.h>
#include <trace.h>
#include <adc.h>
#include <twi.h>

#include <inttypes.h>

#include <stdio.h>
#include "spi_slave.h"
#include "mi2c.h"


#define PWRON_NORMAL 0
#define PWRON_SAMBA 1
#define PWRON_MINI_BACKPLANE 2

#define PIN_LED_LINK (1<<0)
#define PIN_LED_ACT (1<<1)
#define PIN_SFP_TX_DISABLE (1<<2)
#define PIN_SFP_TX_FAULT (1<<3)
#define PIN_EN_FB_TX (1<<4)
#define PIN_EN_FB_RX (1<<5)
#define PIN_SFP_LOS (1<<6)
#define PIN_SFP_DETECT (1<<7)

static const Pin PIN_led_utca0 = {1 << 30, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};
static const Pin PIN_led_utca1 = {1 << 26, AT91C_BASE_PIOB, AT91C_ID_PIOB, PIO_OUTPUT_0, PIO_DEFAULT};
static const Pin PIN_utca_pwron = {1 << 21, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}; //was 0
static const Pin PIN_utca_enable = {1 << 11, AT91C_BASE_PIOB, AT91C_ID_PIOB, PIO_INPUT, PIO_DEFAULT};
static const Pin PIN_flash_serial_sel =	{1 <<  6, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}; // was 1
static const Pin PIN_main_nrst = {1 <<  1, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}; // was 0
static const Pin PIN_dtxd_bootsel = {1 << 28, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_INPUT, PIO_PULLUP};

#define LED_LINK 0
#define LED_ACT 1

#define LED_OFF 0
#define LED_ON 1
#define LED_BLINK_SLOW 2
#define LED_BLINK_FAST 3

#define FEEDBACK_OFF 0
#define FEEDBACK_RX 1
#define FEEDBACK_TX 2


#define PORT_DISABLED 0
#define PORT_DETECT_SFP 1
#define PORT_READY 2
#define PORT_FAULT 3


typedef struct {
  uint8_t addr;
  uint8_t sfp_id_mem[256];
  uint8_t led_link;
  uint8_t led_act;
  uint8_t feedback;
  uint8_t sfp_changed;
  uint8_t sfp_present;
  uint8_t sfp_los;
  uint8_t sfp_tx_enable;
  uint8_t sfp_tx_fault;
  uint8_t updated;
  uint8_t enabled;
  uint8_t prev_pio;
  uint8_t pio;
  uint32_t led_counter;
  int state;
	uint8_t pio_out;

} port_state_t;

#define NUM_PORTS 8

port_state_t ports[NUM_PORTS];


#define PCA9534_REG_IN 0
#define PCA9534_REG_OUT 1
#define PCA9534_REG_POL 2
#define PCA9534_REG_DIR 3


void pca9534_out(uint8_t addr, uint8_t out)
{
  uint8_t nack;
  mi2c_start();
  nack|=mi2c_put_byte(addr);
  nack|=mi2c_put_byte(PCA9534_REG_OUT);
  nack|=mi2c_put_byte(out);
  mi2c_stop();
	if(nack)
	printf("NO ack from: 0x%x\n\r", addr);
}


void pca9534_set_dir(uint8_t addr, uint8_t dir)
{
  mi2c_start();
  mi2c_put_byte(addr);
  mi2c_put_byte(PCA9534_REG_DIR);
  mi2c_put_byte(~dir);
  mi2c_stop();
}

uint8_t pca9534_in(uint8_t addr)
{
  uint8_t ret;
  mi2c_start();
  mi2c_put_byte(addr);
  mi2c_put_byte(PCA9534_REG_IN);
  mi2c_repeat_start();
  mi2c_put_byte(addr|1);
  mi2c_get_byte(&ret);
  mi2c_stop();
  return ret;
}

void port_init(port_state_t *port, uint8_t addr)
{
 port->addr = addr;
 port->led_link = LED_OFF;
 port->led_act = LED_OFF;
 pca9534_set_dir ( addr, PIN_LED_LINK | PIN_LED_ACT | PIN_EN_FB_TX | PIN_EN_FB_RX | PIN_SFP_TX_DISABLE );
 pca9534_out ( addr, 0 );

 port->pio_out = 0;
 port->prev_pio = PIN_SFP_DETECT | PIN_SFP_TX_FAULT;
 port->updated = 1;
 port->state = PORT_DETECT_SFP; // was SFP_DISABLED
 port->led_counter = 0;
}


static void delay(int howmuch)
{
  volatile int i;

  while(howmuch--)
    {
      for(i=0;i<10000;i++) asm volatile("nop");
    }
}

void io_init()
{
  PIO_Configure(&PIN_led_utca0, 1);
  PIO_Configure(&PIN_led_utca1, 1);
  PIO_Configure(&PIN_utca_pwron, 1);
  PIO_Configure(&PIN_flash_serial_sel, 1);
  PIO_Configure(&PIN_main_nrst, 1);
  PIO_Configure(&PIN_dtxd_bootsel, 1);
}

#define CHIP_GPIO 1
#define CHIP_EXPANDER 2

int check_mini_backplane()
{

}

void power_on(int mode)
{
  switch (mode)
    {
    case PWRON_SAMBA:
      PIO_Set(&PIN_flash_serial_sel);
      PIO_Set(&PIN_utca_pwron);
      delay(20);
      PIO_Set(&PIN_main_nrst);
      delay(200);
      PIO_Clear(&PIN_flash_serial_sel);
      break;

    case PWRON_NORMAL:
      PIO_Clear(&PIN_flash_serial_sel);
      PIO_Set(&PIN_utca_pwron);
      delay(20);
      PIO_Set(&PIN_main_nrst);
      break;

    case PWRON_MINI_BACKPLANE:
      break;
    }
}

#define PERIOD_FAST 16
#define PERIOD_SLOW 128


void port_handle_leds(port_state_t *p)
{
	uint8_t led_state = 0;



	switch(p->led_link)
	{
		case LED_OFF:break;
		case LED_ON: led_state |= PIN_LED_LINK;break;
		case LED_BLINK_FAST:if(p->led_counter&PERIOD_FAST) led_state |= PIN_LED_LINK; else led_state &= ~PIN_LED_LINK; break;
		case LED_BLINK_SLOW:if(p->led_counter&PERIOD_SLOW) led_state |= PIN_LED_LINK; else led_state &= ~PIN_LED_LINK; break;
	}

	switch(p->led_act)
	{
		case LED_OFF:break;
		case LED_ON: led_state |= PIN_LED_ACT;break;
		case LED_BLINK_FAST:if(p->led_counter&PERIOD_FAST) led_state |= PIN_LED_ACT; else led_state &= ~PIN_LED_ACT; break;
		case LED_BLINK_SLOW:if(p->led_counter&PERIOD_SLOW) led_state |= PIN_LED_ACT; else led_state &= ~PIN_LED_ACT; break;
	}

	p->pio_out &= ~(PIN_LED_ACT | PIN_LED_LINK);
	p->pio_out |= led_state;

	p->led_counter++;
}


void port_fsm()
{
  port_state_t *p;

  int i;
  for(i=0;i<NUM_PORTS;i++)
    {
      uint8_t in_val;

      p = &ports[i];

      switch(p->state)
	{
	case PORT_DISABLED:

	  break;

	case PORT_DETECT_SFP:
	  p->pio = pca9534_in(p->addr);

//	  printf("pio_in %x\n\r", p->pio);

	  if((p->prev_pio & PIN_SFP_DETECT) && !(p->pio & PIN_SFP_DETECT))
	    {
	      printf("[port-fsm] detected SFP insertion on port %d\n\r", i);
	      p->state = PORT_READY;
	    }

	  p->prev_pio = p->pio;

	  break;

	case PORT_READY:
	  port_handle_leds(p);
	  pca9534_out(p->addr, p->pio_out);
	  break;
	}
    }

}

int mi2c_scan()
{
	int i;

	for(i=0;i<256;i+=2)
	{
		mi2c_start();
		if(!mi2c_put_byte(i)) printf("Found:%x\n\r", i);
		mi2c_stop();
	}
}

#define WD_CMD_SET_LED 1
#define WD_CMD_CALIB_CTL 2

#define WD_CALIB_TX_ON 1
#define WD_CALIB_RX_ON 2
#define WD_CALIB_OFF 3


#define LV2_MUX_ADDR 0x70

void calib_setup(int port, int cmd)
{
	unsigned char lv2_mux = (port >> 1);


	switch(cmd)
	{
		case WD_CALIB_TX_ON:
			printf("Enable TX feedback on port %d mask %x\n\r", port, (1<<lv2_mux));
			ports[port].pio_out |= PIN_EN_FB_TX;
			pca9534_out(ports[port].addr, ports[port].pio_out);
			pca9534_set_dir(LV2_MUX_ADDR, 0xff);
			pca9534_out(LV2_MUX_ADDR, (1<<lv2_mux));
			break;
		case WD_CALIB_RX_ON:
			printf("Enable RX feedback on port %d mask %x\n\r", port, (1<<lv2_mux));
			ports[port].pio_out |= PIN_EN_FB_RX;
			pca9534_out(ports[port].addr, ports[port].pio_out);
			pca9534_set_dir(LV2_MUX_ADDR, 0xff);
			pca9534_out(LV2_MUX_ADDR, (1<<lv2_mux));
			break;
		case WD_CALIB_OFF:
			printf("Disable feedback on port %d\n\r", port);
			ports[port].pio_out &= ~(PIN_EN_FB_RX | PIN_EN_FB_TX);
			pca9534_out(ports[port].addr, ports[port].pio_out);
			pca9534_out(LV2_MUX_ADDR, 0);
			break;
	}
}

void handle_spi()
{
	unsigned char cmd_buf[4];
	unsigned char cmd, led, port, txrx, enable;

	if(spi_slave_read(cmd_buf, 4) == 4)
	{
		cmd = cmd_buf[0];
		switch(cmd)
		{
			case WD_CMD_SET_LED:
				port = cmd_buf[1];
				led = cmd_buf[2];
				if(port >= 0 && port <= 7)
				{
					if(led==LED_LINK)
						ports[port].led_link = cmd_buf[3];
					else if(led==LED_ACT)
						ports[port].led_act = cmd_buf[3];
				}
				break;

			case WD_CMD_CALIB_CTL:
				port = cmd_buf[1];
				txrx= cmd_buf[2];
				if(port >= 0 && port <= 7)
					calib_setup(port, txrx);

				break;
		}
	}

}

int main(void)
{
  int force_samba = 0;

	AT91S_RSTC * rstc = AT91C_BASE_RSTC;

	rstc->RSTC_RMR = 0xa5000000 | 1;

  io_init();

  force_samba = !PIO_Get(&PIN_dtxd_bootsel);

  TRACE_CONFIGURE(DBGU_STANDARD, 115200, BOARD_MCK);

  printf("force_samba: %d\n\r", force_samba);





  mi2c_init();
	spi_slave_init();

	mi2c_scan();

port_init(&ports[0],0x40);
port_init(&ports[1],0x42);
port_init(&ports[2],0x44);
port_init(&ports[3],0x46);
port_init(&ports[4],0x48);
port_init(&ports[5],0x4a);
port_init(&ports[6],0x4c);
port_init(&ports[7],0x4e);

  //power_on(PWRON_NORMAL);

	unsigned char tmp[32];

  for(;;)
  {
  	port_fsm();

		if(spi_slave_poll())
			handle_spi();
  }
}

