#ifndef __LIBWR_HIST_H__
#define __LIBWR_HIST_H__

#include <stdint.h>

#define WRS_HIST_TEMP_SENSORS_N 4
#define WRS_HIST_TEMP_FPGA 0
#define WRS_HIST_TEMP_PLL  1
#define WRS_HIST_TEMP_PSL  2
#define WRS_HIST_TEMP_PSR  3
#define WRS_HIST_TEMP_ENTRIES 64

#define WRS_HIST_RUN_NAND_MAGIC          0xDAD50000
#define WRS_HIST_RUN_NAND_MAGIC_MASK     0xFFFF0000
#define WRS_HIST_RUN_NAND_MAGIC_CRC_MASK 0x0000FF00
#define WRS_HIST_RUN_NAND_MAGIC_VER      0x00000001
#define WRS_HIST_RUN_NAND_MAGIC_VER_MASK 0x000000FF

#define WRS_HIST_RUN_SPI_MAGIC 0x5ADA5500
#define WRS_HIST_RUN_SPI_MAGIC_MASK 0xFFFFFF00
#define WRS_HIST_RUN_SPI_MAGIC_VER 1
#define WRS_HIST_RUN_SPI_MAGIC_VER_MASK 0xFF

#define WRS_HIST_SFP_MAGIC          0xADAF0000
#define WRS_HIST_SFP_MAGIC_MASK     0xFFFF0000
#define WRS_HIST_SFP_MAGIC_CRC_MASK 0x0000FF00
#define WRS_HIST_SFP_MAGIC_VER      0x00000001
#define WRS_HIST_SFP_MAGIC_VER_MASK 0x000000FF

#define WRS_HIST_MAX_SFPS 100
#define WRS_HIST_SFP_PRESENT 1

struct wrs_hist_run_nand {
	uint32_t magic; /* 16bits magic + 8bits crc + 8bits version */
	uint32_t lifetime; /* in seconds, ~136 years */
	uint32_t timestamp; /* in seconds, ~136 years */
	/* average temperature over last hour */
	int8_t temp[WRS_HIST_TEMP_SENSORS_N];
};
/* 16 bytes/h, ~140KB/year ~1MB/7.5years*/

struct wrs_hist_run_spi {
	uint32_t magic; /* 16bits magic + 8bits crc + 8bits version */
	uint32_t lifetime; /* in seconds, ~136 years */
	uint32_t timestamp; /* in seconds, ~136 years */
	/* histogram, tens of hours of particular temperature 512 bytes */
	uint16_t temp[WRS_HIST_TEMP_SENSORS_N][WRS_HIST_TEMP_ENTRIES];
};
/*  */
/* Right now we have 7614 erase blocks available in SPI = ~8MB */
/* 1056 each SPI erase block, 528 is a half of it, 264 quater */
/* 0x108000 = 1081344 (~1MB) gives us 40,96 years, so we can take one meg or 2x1MB or 2x0.5MB
 */


struct wrs_hist_sfp_entry {
	char vn[16]; /* SFP's vendor name */
	char pn[16]; /* SFP's product name */
	char sn[16]; /* SFP's serial number */
	uint32_t sfp_lifetime; /* Lifetime of a particular SFP. Use the LSB of
				* this value to know whether SFP is plugged */
	uint32_t lastseen_swlifetime; /* Value from the switch's lifetime,
				       * when sfp was checked and seen */
	uint32_t lastseen_timestamp; /* Value from timestamp, when sfp was
				      * checked and seen */
	uint8_t crc; /* crc to validate each sfp entry, these with wrong crc
		      * will be discarded */
};

/* size 12 + (50*68 + 50*64) + 4 = ~6.6KB */
struct wrs_hist_sfp_nand {
	uint32_t magic; /* 16bits magic + 8bits crc + 8bits version */
	uint32_t saved_swlifetime; /* When the SFP data was saved to NAND
				    * as switch lifetime in seconds,
				    * ~136 years */
	uint32_t saved_timestamp; /* When the SFP data was saved to NAND
				   * as a timestamp in seconds, ~136 years */
	struct wrs_hist_sfp_entry sfps[WRS_HIST_MAX_SFPS];
	uint32_t end_magic; /* 16bits magic + 8bits crc + 8bits version,
			     * the same magic at the end of structure to verify
			     * the presence of the end */
};

#endif /* __LIBWR_HIST_H__ */
