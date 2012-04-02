/* GPIO pin definitions */
#include <hw/pio.h>

#define LED_OFF 0
#define LED_RED 1
#define LED_GREEN 2
#define LED_YELLOW 3

// definitions of commonly used pins


// reset signal for main FPGA
//const pio_pin_t PIN_main_fpga_nrst[] = {{ PIOA, 5, PIO_MODE_GPIO, PIO_OUT }, {0}};


//I2C for uplink 0
const pio_pin_t PIN_up0_sfp_sda[] = {{PIOB, 22, PIO_MODE_GPIO, PIO_OUT_0}, {0}};
const pio_pin_t PIN_up0_sfp_scl[] = {{PIOB, 23, PIO_MODE_GPIO, PIO_OUT_0}, {0}};

//I2C for uplink 1
const pio_pin_t PIN_up1_sfp_sda[] = {{PIOB, 24, PIO_MODE_GPIO, PIO_OUT_0}, {0}};
const pio_pin_t PIN_up1_sfp_scl[] = {{PIOB, 25, PIO_MODE_GPIO, PIO_OUT_0}, {0}};

//I2C for reading downlink SFPs via mux
const pio_pin_t PIN_down_mux_sfp_sda[] = {{PIOB, 20, PIO_MODE_GPIO, PIO_OUT_0}, {0}};
const pio_pin_t PIN_down_mux_sfp_scl[] = {{PIOB, 21, PIO_MODE_GPIO, PIO_OUT_0}, {0}};


//fan controls
const pio_pin_t PIN_fan_fpga[] = {{PIOB, 26, PIO_MODE_GPIO, PIO_OUT_0}, {0}};
const pio_pin_t PIN_fan_box[] =  {{PIOB, 27, PIO_MODE_GPIO, PIO_OUT_0}, {0}};

const pio_pin_t PIN_mbl_reset_n[] =  {{PIOE, 1, PIO_MODE_GPIO, PIO_OUT_1}, {0}};


const pio_pin_t * _all_cpu_gpio_pins[] =
{
//	PIN_main_fpga_nrst,

	PIN_fan_fpga,
	PIN_fan_box,

	PIN_up0_sfp_sda,
	PIN_up0_sfp_scl,
	PIN_up1_sfp_sda,
	PIN_up1_sfp_scl,

	PIN_down_mux_sfp_sda,
	PIN_down_mux_sfp_scl,
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