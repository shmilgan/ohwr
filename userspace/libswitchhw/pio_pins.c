/* GPIO pin definitions */
#include <hw/pio.h>

#define LED_OFF 0
#define LED_RED 1
#define LED_GREEN 2
#define LED_YELLOW 3

// definitions of commonly used pins


// reset signal for main FPGA
//const pio_pin_t PIN_main_fpga_nrst[] = {{ PIOA, 5, PIO_MODE_GPIO, PIO_OUT }, {0}};

const pio_pin_t PIN_mbl_reset_n[] =  {{PIOE, 1, PIO_MODE_GPIO, PIO_OUT_1}, {0}};

const pio_pin_t * _all_cpu_gpio_pins[] =
{
	PIN_mbl_reset_n,
	0
};



/*const pio_pin_t * _all_fpga_gpio_pins[] =
{
	PIN_up_ctrl_sda,
	PIN_up_ctrl_scl,
	PIN_down_ctrl_sda,
	PIN_down_ctrl_scl,
	0
};



*/
