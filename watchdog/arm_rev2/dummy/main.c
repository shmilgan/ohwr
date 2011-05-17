#include <board.h>
#include <pio/pio.h>
#include <dbgu/dbgu.h>
#include <aic/aic.h>
#include <utility/trace.h>
#include <adc/adc.h>

#include <stdio.h>

static const Pin PIN_led_utca0 = 				{1 << 30, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};
static const Pin PIN_led_utca1 = 				{1 << 26, AT91C_BASE_PIOB, AT91C_ID_PIOB, PIO_OUTPUT_0, PIO_DEFAULT};

static const Pin PIN_utca_pwron = 			{1 << 21, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};
static const Pin PIN_flash_serial_sel = 		{1 <<  6, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT};
static const Pin PIN_main_nrst = 				{1 <<  1, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};

static const Pin PIN_dtxd_bootsel = 			{1 << 28, AT91C_BASE_PIOA, AT91C_ID_PIOA, PIO_INPUT, PIO_PULLUP};

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

int main(void)
{
	int force_samba = 0;
	
	io_init();
	
	force_samba = !PIO_Get(&PIN_dtxd_bootsel);
	
	
  TRACE_CONFIGURE(DBGU_STANDARD, 115200, BOARD_MCK);

    printf("force_samba: %d\n", force_samba);


	if(force_samba)
	{
		PIO_Set(&PIN_flash_serial_sel);
		PIO_Set(&PIN_utca_pwron);
		delay(20);
		PIO_Set(&PIN_main_nrst);
		delay(200);
		PIO_Clear(&PIN_flash_serial_sel);
	} else {
	
		PIO_Clear(&PIN_flash_serial_sel);
		PIO_Set(&PIN_utca_pwron);
		delay(20);
		PIO_Set(&PIN_main_nrst);
	
	}


	for(;;)
	{
		printf(",");
 		PIO_Set(&PIN_led_utca0);
		delay(10);
		PIO_Clear(&PIN_led_utca0);
		delay(10);	
 		PIO_Set(&PIN_led_utca1);
		delay(10);
		PIO_Clear(&PIN_led_utca1);
		delay(10);	

	}
}

