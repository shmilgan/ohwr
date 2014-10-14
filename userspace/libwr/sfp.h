#ifndef SFP_H
#define SFP_H

/* This structure describes the pair of PCA9548A chip used as muxes for i2c */
struct wr_i2c_mux {
	/* I2C bus */
	struct i2c_bus *bus;
	/* I2C addresses of the two PCA9548 chips */
	int address[2];
	/* Reset pin */
	struct pio_pin_t reset;
};
int wr_i2c_mux_set_channel(struct wr_i2c_mux *, int);

/* This structure describes a single PCA9554D chip */
struct pca9554d {
	/* The i2c bus used for access */
	struct i2c_bus_t *bus;
	/* The address of this PCA9554D (set by A0,A1,A2 pins) */
	int address;
	int (*init)(struct pca9554d *);
	int (*exit)(struct pca9554d *);
};
int pca9554d_get_input_reg(struct pca9554d *);
int pca9554d_set_output_reg(struct pca9554d *);
int pca9554d_set_polarity_reg(struct pca9554d *);
int pca9554d_set_config_reg(struct pca9554d *);

#define GPIO_BITS_LOWER		0
#define GPIO_BITS_UPPER		1

#define WRLINK_LED_LINK(m)	\
	(1 << (((m->bits_type == GPIO_BITS_LOWER) ? 0 : 2) + 4*m->bits_type))
#define WRLINK_LED_WRMODE(m)	\
	(1 << (((m->bits_type == GPIO_BITS_LOWER) ? 1 : 0) + 4*m->bits_type))
#define WRLINK_LED_SYNCED(m)	\
	(1 << (((m->bits_type == GPIO_BITS_LOWER) ? 3 : 1) + 4*m->bits_type))
#define SFP_TX_DISABLE(m)	\
	(1 << (((m->bits_type == GPIO_BITS_LOWER) ? 2 : 3) + 4*m->bits_type))

struct wr_sfp_module {
	/*
	 * The PCA9554D associated with this device. Use a pointer here
	 * as a single PCA9554D controls the pins for two SFP modules
	 * each.
	 */
	struct pca9554d *gpio_ctl;
	/* One of PCA9554_BITS_LOWER or PCA9554_BITS_UPPER */
	int bits_type;

	/* The PCA9548 chip for accessing this SFP module */
	struct wr_i2c_mux *mux;
	/*
	 * The channel for writing to control register of the
	 * PCA9548 chips.
	 */
	int channel;
};

#endif
