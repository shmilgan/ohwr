#ifndef __SHW_SFPLIB_H
#define __SHW_SFPLIB_H

#define SFP_LED_LINK	(1 << 0)
#define SFP_LED_WRMODE	(1 << 1)
#define SFP_LED_SYNCED	(1 << 2)
#define SFP_TX_DISABLE	(1 << 3)

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
	uint32_t alpha;
	uint32_t delta_tx;
	uint32_t delta_rx;
	struct shw_sfp_caldata *next;
};

/* Public API */

/*
 * Scan all ports for plugged in SFP's. The return value is a bitmask
 * of all the ports with detected SFP's (bits 0-17 are valid).
 */
uint32_t shw_sfp_module_scan(void);

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

/* Load the db from a file */
int shw_sfp_read_db(char *filename);

/* return NULL if no data found */
struct shw_sfp_caldata *shw_sfp_get_cal_data(int num);

#endif			// __SHW_SFPLIB_H
