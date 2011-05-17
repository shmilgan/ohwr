#include <hw/pio.h>

#define LED_OFF 0
#define LED_RED 1
#define LED_GREEN 2
#define LED_YELLOW 3

// definitions of commonly used pins


// reset signal for main FPGA
const pio_pin_t PIN_main_fpga_nrst[] = {{ PIOB, 20, PIO_MODE_GPIO, PIO_OUT }, {0}};


// Front panel LEDs
const pio_pin_t PIN_fled0[] = {{PIOC, 5, PIO_MODE_GPIO, PIO_OUT_1},
    {PIOC, 12, PIO_MODE_GPIO, PIO_OUT_1},
    {0}
};

const pio_pin_t PIN_fled1[] = {{PIOC, 7, PIO_MODE_GPIO, PIO_OUT_1},
    {PIOC, 19, PIO_MODE_GPIO, PIO_OUT_1},
    {0}
};

const pio_pin_t PIN_fled2[] = {{PIOC, 9, PIO_MODE_GPIO, PIO_OUT_1},
    {PIOC, 17, PIO_MODE_GPIO, PIO_OUT_1},
    {0}
};

const pio_pin_t PIN_fled3[] = {{PIOC, 14, PIO_MODE_GPIO, PIO_OUT_1},
    {PIOC, 6, PIO_MODE_GPIO, PIO_OUT_1},
    {0}
};

const pio_pin_t PIN_fled4[] = {{PIO_FPGA, 0, PIO_MODE_GPIO, PIO_OUT_1},
    {PIO_FPGA, 1, PIO_MODE_GPIO, PIO_OUT_1},
    {0}
};

const pio_pin_t PIN_fled5[] = {{PIO_FPGA, 2, PIO_MODE_GPIO, PIO_OUT_1},
    {PIO_FPGA, 3, PIO_MODE_GPIO, PIO_OUT_1},
    {0}
};

const pio_pin_t PIN_fled6[] = {{PIO_FPGA, 4, PIO_MODE_GPIO, PIO_OUT_1},
    {PIO_FPGA, 5, PIO_MODE_GPIO, PIO_OUT_1},
    {0}
};

const pio_pin_t PIN_fled7[] = {{PIO_FPGA, 6, PIO_MODE_GPIO, PIO_OUT_1},
    {PIO_FPGA, 7, PIO_MODE_GPIO, PIO_OUT_1},
    {0}
};

// uTCA front panel LEDs
const pio_pin_t PIN_uled0[] = {{PIOC, 11, PIO_MODE_GPIO, PIO_OUT}, {0}};
const pio_pin_t PIN_uled1[] = {{PIOC, 15, PIO_MODE_GPIO, PIO_OUT}, {0}};
const pio_pin_t PIN_uled2[] = {{PIOC, 18, PIO_MODE_GPIO, PIO_OUT}, {0}};
const pio_pin_t PIN_uled3[] = {{PIOC, 13, PIO_MODE_GPIO, PIO_OUT}, {0}};

// AD9516 PLL control signals
const pio_pin_t PIN_ad9516_cs[] =      {{PIOC, 10, PIO_MODE_GPIO, PIO_OUT_1  }, {0} }; // clkb_cpupin3 -> pc10
const pio_pin_t PIN_ad9516_sclk[] =    {{PIOC, 16, PIO_MODE_GPIO, PIO_OUT_1  }, {0} }; // clkb_cpupin1 -> pc16
const pio_pin_t PIN_ad9516_sdio[] =     {{PIOB, 30, PIO_MODE_GPIO, PIO_OUT_1  }, {0} }; // clkb_cpupin5 -> pb30
const pio_pin_t PIN_ad9516_sdo[] =     {{PIOC, 8,  PIO_MODE_GPIO, PIO_IN     }, {0} }; // clkb_cpupin0 -> pc8
const pio_pin_t PIN_ad9516_refsel[] =  {{PIOB, 26, PIO_MODE_GPIO, PIO_OUT_0  }, {0} }; // clkb_cpupin4 -> pb26
const pio_pin_t PIN_ad9516_nrst[] =    {{PIOC, 3, PIO_MODE_GPIO, PIO_OUT_1  }, {0} }; // clkb_cpupin2 -> pc3

// uTCA Hotswap switch
const pio_pin_t PIN_hotswap_switch[] =    {{PIOC, 1, PIO_MODE_GPIO, PIO_IN }, {0} };

// main FPGA passive serial configuration signals
const pio_pin_t PIN_main_fpga_dclk[] =        {{ PIOB, 1,    PIO_MODE_GPIO, PIO_OUT_1}, {0}};
const pio_pin_t PIN_main_fpga_nconfig[] =     {{ PIOB, 22,   PIO_MODE_GPIO, PIO_OUT_1}, {0}};
const pio_pin_t PIN_main_fpga_data[] =        {{ PIOB, 2,    PIO_MODE_GPIO, PIO_OUT_1}, {0}};
const pio_pin_t PIN_main_fpga_nstatus[] =     {{ PIOB, 31,   PIO_MODE_GPIO, PIO_IN}, {0}};
const pio_pin_t PIN_main_fpga_confdone[] =    {{ PIOB, 18,   PIO_MODE_GPIO, PIO_IN}, {0}}; // INVERTED!

// clocking board FPGA passive serial configuration signals
const pio_pin_t PIN_clkb_fpga_dclk[] =        {{ PIOB, 7,    PIO_MODE_GPIO, PIO_OUT_1}, {0}};
const pio_pin_t PIN_clkb_fpga_nconfig[] =     {{ PIOB, 23,   PIO_MODE_GPIO, PIO_OUT_1}, {0}};
const pio_pin_t PIN_clkb_fpga_data[] =        {{ PIOB, 8,    PIO_MODE_GPIO, PIO_OUT_1}, {0}};
const pio_pin_t PIN_clkb_fpga_nstatus[] =     {{ PIOB, 28,   PIO_MODE_GPIO, PIO_IN}, {0}};
const pio_pin_t PIN_clkb_fpga_confdone[] =    {{ PIOB, 24,   PIO_MODE_GPIO, PIO_IN}, {0}}; // INVERTED!


const pio_pin_t PIN_clkb_fpga_nrst[] =    {{ PIOB, 25,   PIO_MODE_GPIO, PIO_OUT_0}, {0}};

const pio_pin_t PIN_up0_sfp_sda[] = {{PIO_FPGA, 8, PIO_MODE_GPIO, PIO_OUT_1}, {0}};
const pio_pin_t PIN_up0_sfp_scl[] = {{PIO_FPGA, 9, PIO_MODE_GPIO, PIO_OUT_1}, {0}};
const pio_pin_t PIN_up0_sfp_los[] = {{PIO_FPGA, 10, PIO_MODE_GPIO, PIO_IN}, {0}};
const pio_pin_t PIN_up0_sfp_tx_fault[] = {{PIO_FPGA, 11, PIO_MODE_GPIO, PIO_IN}, {0}};
const pio_pin_t PIN_up0_sfp_tx_disable[] = {{PIO_FPGA, 12, PIO_MODE_GPIO, PIO_OUT_0}, {0}};
const pio_pin_t PIN_up0_sfp_detect[] = {{PIO_FPGA, 13, PIO_MODE_GPIO, PIO_IN}, {0}};

const pio_pin_t PIN_up1_sfp_sda[] = {{PIO_FPGA, 18, PIO_MODE_GPIO, PIO_OUT_1}, {0}};
const pio_pin_t PIN_up1_sfp_scl[] = {{PIO_FPGA, 19, PIO_MODE_GPIO, PIO_OUT_1}, {0}};
const pio_pin_t PIN_up1_sfp_los[] = {{PIO_FPGA, 20, PIO_MODE_GPIO, PIO_IN}, {0}};
const pio_pin_t PIN_up1_sfp_tx_fault[] = {{PIO_FPGA, 21, PIO_MODE_GPIO, PIO_IN}, {0}};
const pio_pin_t PIN_up1_sfp_tx_disable[] = {{PIO_FPGA, 22, PIO_MODE_GPIO, PIO_OUT_0}, {0}};
const pio_pin_t PIN_up1_sfp_detect[] = {{PIO_FPGA, 23, PIO_MODE_GPIO, PIO_IN}, {0}};

const pio_pin_t * _all_cpu_gpio_pins[] =
{
    PIN_fled0,
    PIN_fled1,
    PIN_fled2,
    PIN_fled3,

    PIN_uled0,
    PIN_uled1,
    PIN_uled2,
    PIN_uled3,

    PIN_hotswap_switch,

    NULL
};

const pio_pin_t * _all_fpga_gpio_pins[] =
{
    PIN_fled4,
    PIN_fled5,
    PIN_fled6,
    PIN_fled7,

    PIN_up0_sfp_sda,
    PIN_up0_sfp_scl,
    PIN_up0_sfp_los,
    PIN_up0_sfp_tx_fault,
    PIN_up0_sfp_tx_disable,
    PIN_up0_sfp_detect,


    PIN_up1_sfp_sda,
    PIN_up1_sfp_scl,
    PIN_up1_sfp_los,
    PIN_up1_sfp_tx_fault,
    PIN_up1_sfp_tx_disable,
    PIN_up1_sfp_detect,

    NULL
};

const pio_pin_t * _fp_leds[] =
{

    PIN_fled0,
    PIN_fled1,
    PIN_fled2,
    PIN_fled3,
    PIN_fled4,
    PIN_fled5,
    PIN_fled6,
    PIN_fled7

};
