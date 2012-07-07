#include "sfp.h"

#define MUX_ADDR(sel)	((0xE << 3) | (sel & 0x7))

static const pio_pin_t mux_sda = {PIOB, 0, PIO_MODE_GPIO, PIO_OUT_1};
static const pio_pin_t mux_scl = {PIOB, 1, PIO_MODE_GPIO, PIO_OUT_1};

struct i2c_bus_t *mux_bus;

struct wr_i2c_mux sfp_mux = {
	.address = {
		MUX_ADDR(0),
		MUX_ADDR(1)
	},
	.reset = {
		.port = PIOD,
		.pin = 12,
		.mode = PIO_MODE_GPIO,
		.dir = PIO_OUT_1
	}
};

struct i2c_bus_t *sfp_bus0;
struct i2c_bus_t *sfp_bus1;

struct pca9554d wr_sfp_gpio[] = {
	{
		/* 0 */
	}, {
		/* 1 */
	}, {
		/* 2 */
	}, {
		/* 3 */
	}, {
		/* 4 */
	}, {
		/* 5 */
	}, {
		/* 6 */
	}, {
		/* 7 */
	}, {
		/* 8 */
	},
};

struct wr_sfp_module[18]

void sfp_init(void)
{
	/* Create a bit-banged i2c for the muxes */
	mux_bus = i2c_cpu_bb_new_bus(&mux_scl, &mux_sda);
	/* Set the bus to be used for the mux control */
	sfp_mux.bus = mux_bus;
}
