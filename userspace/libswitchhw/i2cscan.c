/*
 * i2cscan.c
 * CERN 2012 B.Bielawski
 */

#include <stdio.h>
#include <stdlib.h>

#include "i2c.h"
#include "pio.h"

#include "i2c_cpu_bb.h"
#include "i2c_fpga_reg.h"

#include "libshw_i2c.h"


//I2C for reading downlink SFPs via mux
static const pio_pin_t bus0_sda = {PIOB, 20, PIO_MODE_GPIO, PIO_OUT_1};
static const pio_pin_t bus0_scl = {PIOB, 21, PIO_MODE_GPIO, PIO_OUT_1};

static const pio_pin_t bus1_sda = {PIOB, 22, PIO_MODE_GPIO, PIO_OUT_1};
static const pio_pin_t bus1_scl = {PIOB, 23, PIO_MODE_GPIO, PIO_OUT_1};

static const pio_pin_t bus2_sda = {PIOB, 24, PIO_MODE_GPIO, PIO_OUT_1};
//static const pio_pin_t bus2_scl = {PIOB, 25, PIO_MODE_GPIO, PIO_OUT_1};


//PB20
//PB21
//PB22
//PB22
//PB23
//PB24
//PB25
//PB26
//PB27


static const pio_pin_t fan_box =  {PIOB, 20, PIO_MODE_GPIO, PIO_OUT_1};
static const pio_pin_t fan_fpga = {PIOB, 24, PIO_MODE_GPIO, PIO_OUT_1};


static const pio_pin_t led1 = {PIOA, 0, PIO_MODE_GPIO, PIO_OUT_1};
static const pio_pin_t led2 = {PIOA, 1, PIO_MODE_GPIO, PIO_OUT_1};

static const pio_pin_t mux_scl = {PIOB, 25, PIO_MODE_GPIO, PIO_OUT_1};
static const pio_pin_t mux_sda = {PIOB, 27, PIO_MODE_GPIO, PIO_OUT_1};

int main()
{
    printf("Initing HW...\n");

    shw_init();

    

    shw_pio_configure(&led1);
    shw_pio_configure(&led2);

    printf("Connecting to a bus...\n");

    //struct i2c_bus_t* bus1 = i2c_cpu_bb_new_bus(&mux_scl, &mux_sda);
    struct i2c_bus_t* bus1 = i2c_fpga_reg_new_bus(FPGA_I2C1_ADDRESS, 50000);
    
    /*
    if (!bus1)
    {
	printf("Failed to create bus!\n");
	exit(1);
    }
    
    
    unsigned char detected_devices_bitmap[16];
    
    int detected_devices  = i2c_scan(bus1, detected_devices_bitmap);
    
    printf("Detected %d devices\n", detected_devices);
    if (detected_devices > 0)
    {
	int i;
	for (i = 0; i < 16; i++)
	{
	    printf("%02X ", detected_devices_bitmap[i] % 0xFF);
	}
	printf("\n");
    }
    */
    /*
    unsigned char data[16] = {1};
    
    printf("Send+Rcvd: %d\n", i2c_transfer(bus1, 0x70, 1, 0, data));
    printf("Send+Rcvd: %d\n", i2c_transfer(bus1, 0x71, 1, 0, data));
    printf("Send+Rcvd: %d\n", i2c_transfer(bus1, 0x72, 1, 0, data));
    printf("Send+Rcvd: %d\n", i2c_transfer(bus1, 0x73, 1, 0, data));
    */
    
    int i = 0;
    for (i = 0; i < 7; i++)
    {
	wrswhw_pca9554_configure(bus1, 0x20 | i);
	wrswhw_pca9554_set_output_reg(bus1, 0x20 | i, ~(WRSWHW_INITIAL_OUTPUT_STATE));
    }
    
    
    i2c_free(bus1);
    return 0;
}
