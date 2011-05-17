#ifndef __PIO_PINS_H
#define __PIO_PINS_H

#include <stdio.h>

#define LED_OFF 0
#define LED_RED 1
#define LED_GREEN 2
#define LED_YELLOW 3


// reset signal for main FPGA
extern const pio_pin_t PIN_main_fpga_nrst[];
extern const pio_pin_t PIN_fled0[];
extern const pio_pin_t PIN_fled1[];
extern const pio_pin_t PIN_fled2[];
extern const pio_pin_t PIN_fled3[];
extern const pio_pin_t PIN_fled4[];

extern const pio_pin_t PIN_fled5[];
extern const pio_pin_t PIN_fled6[];
extern const pio_pin_t PIN_fled7[];

// uTCA front panel LEDs
extern const pio_pin_t PIN_uled0[];
extern const pio_pin_t PIN_uled1[];
extern const pio_pin_t PIN_uled2[];
extern const pio_pin_t PIN_uled3[];

// AD9516 PLL control signals
extern const pio_pin_t PIN_ad9516_cs[];
extern const pio_pin_t PIN_ad9516_sclk[];
extern const pio_pin_t PIN_ad9516_sdio[];
extern const pio_pin_t PIN_ad9516_sdo[];
extern const pio_pin_t PIN_ad9516_refsel[];
extern const pio_pin_t PIN_ad9516_nrst[];

// uTCA Hotswap switch
extern const pio_pin_t PIN_hotswap_switch[];

// main FPGA passive serial configuration signals
extern const pio_pin_t PIN_main_fpga_dclk[];
extern const pio_pin_t PIN_main_fpga_nconfig[];
extern const pio_pin_t PIN_main_fpga_data[];
extern const pio_pin_t PIN_main_fpga_nstatus[];
extern const pio_pin_t PIN_main_fpga_confdone[];

// clocking board FPGA passive serial configuration signals
extern const pio_pin_t PIN_clkb_fpga_dclk[];
extern const pio_pin_t PIN_clkb_fpga_nconfig[];
extern const pio_pin_t PIN_clkb_fpga_data[];
extern const pio_pin_t PIN_clkb_fpga_nstatus[];
extern const pio_pin_t PIN_clkb_fpga_confdone[];


extern const pio_pin_t PIN_clkb_fpga_nrst[];

extern const pio_pin_t PIN_up0_sfp_sda[];
extern const pio_pin_t PIN_up0_sfp_scl[];
extern const pio_pin_t PIN_up0_sfp_los[];
extern const pio_pin_t PIN_up0_sfp_tx_fault[];
extern const pio_pin_t PIN_up0_sfp_tx_disable[];
extern const pio_pin_t PIN_up0_sfp_detect[];

extern const pio_pin_t PIN_up1_sfp_sda[];
extern const pio_pin_t PIN_up1_sfp_scl[];
extern const pio_pin_t PIN_up1_sfp_los[];
extern const pio_pin_t PIN_up1_sfp_tx_fault[];
extern const pio_pin_t PIN_up1_sfp_tx_disable[];
extern const pio_pin_t PIN_up1_sfp_detect[];


extern const pio_pin_t * _all_cpu_gpio_pins[];
extern const pio_pin_t * _all_fpga_gpio_pins[];
extern const pio_pin_t * _fp_leds[];

#endif

