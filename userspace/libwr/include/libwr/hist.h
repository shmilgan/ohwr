#ifndef __LIBWR_HIST_H__
#define __LIBWR_HIST_H__

#include <stdint.h>

#define WRS_HIST_TEMP_SENSORS_N 4
#define WRS_HIST_TEMP_FPGA 0
#define WRS_HIST_TEMP_PLL  1
#define WRS_HIST_TEMP_PSL  2
#define WRS_HIST_TEMP_PSR  3
#define WRS_HIST_TEMP_ENTRIES 64

#define WRS_HIST_UP_NAND_MAGIC		0xDAD5
#define WRS_HIST_UP_NAND_MAGIC_VER	0x01

#define WRS_HIST_UP_SPI_MAGIC		0x5ADA
#define WRS_HIST_UP_SPI_MAGIC_VER	0x01

#define WRS_HIST_SFP_MAGIC          0xADAF
#define WRS_HIST_SFP_MAGIC_VER      0x01

#define WRS_HIST_SFP_EMAGIC         0xAA
#define WRS_HIST_SFP_EMAGIC_VER     0x01


#define WRS_HIST_MAX_SFPS 100
#define WRS_HIST_SFP_PRESENT 0x01

struct wrs_hist_up_nand {
	uint16_t magic; /* 16bits magic */
	uint8_t ver; /* 8bits version */
	uint8_t crc; /* 8bits crc */
	uint32_t lifetime; /* in seconds, ~136 years */
	uint32_t timestamp; /* in seconds, ~136 years */
	/* average temperature over last hour */
	int8_t temp[WRS_HIST_TEMP_SENSORS_N];
};
/* 16 bytes/h, ~140KB/year ~1MB/7.5years*/

/* Right now we have 7614 total erase blocks available in SPI = ~8MB */
/* Each erase block in SPI is 1056 bytes, 528 is a half of it.
 * Let's use 0x108000 = 1081344 (~1MB) for each partition, this gives us
 * maximum 2048 entries (each entry is half of a sector, 528 bytes).
 * If we write every 168 hours (7*24, one week of uptime). We will run out of
 * entries after 2048*168 = 344 064 hours = 39.28 years
 */
struct wrs_hist_up_spi {
	uint16_t magic; /* 16bits magic */
	uint8_t ver; /* 8bits version */
	uint8_t crc; /* 8bits crc */
	uint32_t lifetime; /* in seconds, ~136 years */
	uint32_t timestamp; /* in seconds, ~136 years */
	/* histogram, hours of particular temperature 512 bytes
	 * 16bit number of hours overlaps after 7,48 years */
	uint16_t temp[WRS_HIST_TEMP_SENSORS_N][WRS_HIST_TEMP_ENTRIES];
	uint8_t padding[4]; /* pad to the size of half of a sector
			     * (524+4=528) */
};



struct wrs_hist_sfp_entry {
	uint8_t mag; /* 8bits magic */
	uint8_t ver; /* 8bits version */
	uint8_t flags; /* 8bits flags */
	uint8_t crc; /* 8bits crc to validate each sfp entry, these with wrong
		      * crc will be discarded */
	char vn[16]; /* SFP's vendor name */
	char pn[16]; /* SFP's product name */
	char sn[16]; /* SFP's serial number */
	uint32_t sfp_lifetime; /* Lifetime of a particular SFP */
	uint32_t lastseen_swlifetime; /* Value from the switch's lifetime,
				       * when sfp was checked and seen */
	uint32_t lastseen_timestamp; /* Value from timestamp, when sfp was
				      * checked and seen */
};

/* size 12 + (50*68 + 50*64) + 4 = ~6.6KB */
struct wrs_hist_sfp_nand {
	uint16_t magic; /* 16bits magic */
	uint8_t ver; /* 8bits version */
	uint8_t crc; /* 8bits crc */
	uint32_t saved_swlifetime; /* When the SFP data was saved to NAND
				    * as switch lifetime in seconds,
				    * ~136 years */
	uint32_t saved_timestamp; /* When the SFP data was saved to NAND
				   * as a timestamp in seconds, ~136 years */
	struct wrs_hist_sfp_entry sfps[WRS_HIST_MAX_SFPS];
	/* the same information at the end of structure to verify the presence
	 * of the end */
	uint16_t end_magic; /* 16bits magic */
	uint8_t end_ver; /* 8bits version */
	uint8_t end_crc; /* 8bits crc */
};

#endif /* __LIBWR_HIST_H__ */
