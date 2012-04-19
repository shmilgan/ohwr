#ifndef I2C_SFP_H
#define I2C_SFP_H

#include "i2c.h"

#include <hw/sfp_lib.h>

//address from AT24C01 datasheet (1k, all address lines shorted to the ground)
#define I2C_SFP_ADDRESS 0x50

/* The two FPGA buses */
#define WR_FPGA_BUS0	0
#define WR_FPGA_BUS1	1
/* The multiplexer bus */
#define WR_MUX_BUS	2
/* Individual buses. 0 and 1 are weird  */
#define WR_SFP0_BUS	3
#define WR_SFP1_BUS	4

struct shw_sfp_header {
	uint8_t id;
	uint8_t ext_id;
	uint8_t connector;
	uint8_t transciever[8];
	uint8_t encoding;
	uint8_t br_nom;
	uint8_t reserved1;
	uint8_t length1;	/* Link length supported for 9/125 mm fiber (km) */
	uint8_t length2;	/* Link length supported for 9/125 mm fiber (100m) */
	uint8_t length3;	/* Link length supported for 50/125 mm fiber (10m) */
	uint8_t length4;	/* Link length supported for 62.5/125 mm fiber (10m) */
	uint8_t length5;	/* Link length supported for copper (1m) */
	uint8_t reserved2;
	uint8_t vendor_name[16];
	uint8_t reserved3;
	uint8_t vendor_oui[3];
	uint8_t vendor_pn[16];
	uint8_t vendor_rev[4];
	uint8_t reserved4[3];
	uint8_t cc_base;

	/* extended ID fields start here */
	uint8_t options[2];
	uint8_t br_max;
	uint8_t br_min;
	uint8_t vendor_serial[16];
	uint8_t date_code[8];
	uint8_t reserved[3];
	uint8_t cc_ext;
} __attribute__((packed));

extern struct i2c_bus i2c_buses[];

/**
 * Initializes all the buses related to SFP control. These include:
 *     - FPGA buses x 2
 *     - Bitbanged bus to muxes
 *     - Bitbanged buses to SFP link0 and link1
 * @return: 0 on success, -1 on error
 */
int shw_sfp_buses_init(void);

/* Iniialize all GPIO's related to SFP's */
void shw_sfp_gpio_init(void);

/*
 * Scan a specific bus. Use the bus defines on the top of this header
 * The return value is the number of devices discovered on the bus.
 * The dev_map array should be at least 16 bytes in size to cover the
 * 128 possible addresses. The dev_map is set to a bitmask representing
 * which devices are present.
 */
int shw_sfp_bus_scan(int num, uint8_t *dev_map);

/* Read a header from a specific port (num = 0..17) */
int shw_sfp_read_header(int num, struct shw_sfp_header *head);
/* Verify the base part of an SFP header (first 64 bytes) */
int shw_sfp_header_verify_base(struct shw_sfp_header *head);
/* Verify the extended part of an SFP header (bytes 64 to 94) */
int shw_sfp_header_verify_ext(struct shw_sfp_header *head);
/* Verify both base and extended parts of SFP header */
int shw_sfp_header_verify(struct shw_sfp_header *head);
/* Print information about an SFP header */
void shw_sfp_print_header(struct shw_sfp_header *head);
/* Dump the hex output of an SFP header (12 lines of 8 bytes each) */
void shw_sfp_header_dump(struct shw_sfp_header *head);

/*
 * Read/write to an SFP device.
 * @num:	SFP number (0 to 17)
 * @addr:	i2c address to read/write to/from
 * @off:	Offset to write to (eg. in case of eeprom). Set to -1 to ignore
 * @len:	Length of data
 * @buf:	Buffer with data
 *
 * @return number of bytes sent+received
 */
int32_t shw_sfp_read(int num, uint32_t addr, int off, int len, uint8_t *buf);
int32_t shw_sfp_write(int num, uint32_t addr, int off, int len, uint8_t *buf);

/* Set/get the 4 GPIO's connected to PCA9554's for a particular SFP */
void shw_sfp_gpio_set(int num, uint8_t state);
uint8_t shw_sfp_gpio_get(int num);

static inline void shw_sfp_set_generic(int num, int status, int type)
{
	uint8_t state;
	state = shw_sfp_gpio_get(num);
	if (status)
		state |= type;
	else
		state &= ~type;
	shw_sfp_gpio_set(num, state);
}

#define shw_sfp_set_led_link(num, status)	\
	shw_sfp_set_generic(num, status, SFP_LED_LINK)

#define shw_sfp_set_led_wrmode(num, status)	\
	shw_sfp_set_generic(num, status, SFP_LED_WRMODE)

#define shw_sfp_set_led_synced(num, status)	\
	shw_sfp_set_generic(num, status, SFP_LED_SYNCED)

#define shw_sfp_set_tx_disable(num, status)	\
	shw_sfp_set_generic(num, status, SFP_TX_DISABLE)

#define SFP_FLAG_CLASS_DATA	(1 << 0)
#define SFP_FLAG_DEVICE_DATA	(1 << 1)

struct shw_sfp_caldata {
	int flags;
	char part_num[16];	/* part number of device as found in DB */
	char vendor_serial[16];
	/* Callibration data */
	double alpha;
	uint32_t delta_tx;
	uint32_t delta_rx;
	struct shw_sfp_caldata *next;
};

/* Load the db from a file */
int shw_sfp_read_db(char *filename);
/* return NULL if no data found */
struct shw_sfp_caldata *shw_sfp_get_cal_data(int num);

#endif			//I2C_SFP_H
