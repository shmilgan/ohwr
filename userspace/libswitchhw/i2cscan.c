/*
 * i2cscan.c
 * CERN 2012 B.Bielawski
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "i2c.h"
#include "pio.h"

#include "i2c_bitbang.h"
#include "i2c_fpga_reg.h"

#include "i2c_sfp.h"

#include "libshw_i2c.h"

#define ARRAY_SIZE(a)                               \
  (sizeof(a) / sizeof(*(a)))

int main()
{
    int ret;
    uint32_t i;
    unsigned char dev_map[16];
    int detect;
    struct shw_sfp_header head;

    printf("Initing HW...\n");
    memset(&head, 0, sizeof(head));

    shw_init();
    if (shw_sfp_buses_init() < 0) {
        printf("Failed to initialize buses\n");
        return 1;
    }

    shw_sfp_read_db("sfpdb.lua");

    shw_sfp_bus_scan(WR_FPGA_BUS0, dev_map);
    shw_sfp_bus_scan(WR_FPGA_BUS1, dev_map);
    shw_sfp_bus_scan(WR_MUX_BUS, dev_map);
    shw_sfp_bus_scan(WR_SFP0_BUS, dev_map);
    shw_sfp_bus_scan(WR_SFP1_BUS, dev_map);

#if 0
    shw_sfp_gpio_init();
    pio_pin_t *sda = ((struct i2c_bitbang *)i2c_buses[WR_MUX_BUS].type_specific)->sda;
    while (1) {
        mi2c_pin_out(sda, 1);
        mi2c_pin_out(sda, 0);
	    for (i = 0; i < 18; i++) {
		    shw_sfp_set_led_link(i, 1);
	    }
	    usleep(100);
	    for (i = 0; i < 18; i++) {
		    shw_sfp_set_led_link(i, 0);
	    }
		    usleep(100);
    }
#endif

    struct shw_sfp_caldata *d;
    printf("\nScanning SFPs:\n");
    for (i = 0; i < 18; i++) {
        memset(&head, 0, sizeof(struct shw_sfp_header));
        ret = shw_sfp_read(i, 0x50, 0x0, sizeof(head), (uint8_t *)&head);
        if (ret == I2C_DEV_NOT_FOUND || ret < 0) {
            printf("SFP %d: NOT PRESENT\n", i);
            continue;
        }
        printf("SFP %d: PRESENT\n", i);
        shw_sfp_header_dump(&head);
        shw_sfp_print_header(&head);
	d = NULL;
	d = shw_sfp_get_cal_data(i);
	if (d)
		printf("Callibration (%s): alpha = %d, dtx = %d, drx = %d\n",
			(d->flags & SFP_FLAG_CLASS_DATA) ? "CLASS" : "DEVICE",
			d->alpha, d->delta_tx, d->delta_rx);
        printf("\n");
    }


#if 0
    for (i = 0; i < 18; i++) {
        memset(&head, 0, sizeof(struct shw_sfp_header));
	printf("SFP_SCAN: %08x\n", shw_sfp_scan());
        ret = shw_sfp_read(i, 0x50, 0x0, sizeof(head), (uint8_t *)&head);
        if (ret == I2C_DEV_NOT_FOUND) {
            printf("SFP %d: NOT PRESENT\n", i);
            continue;
        }
        printf("SFP %d: PRESENT\n", i);
        printf("\n");
        shw_sfp_header_dump(&head);
        shw_sfp_print_header(&head);
    }
#endif

    return 0;
}
