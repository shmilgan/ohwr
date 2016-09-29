#ifndef __LIBWR_HIST_H__
#define __LIBWR_HIST_H__

#include <stdint.h>

#define WRS_HIST_TEMP_FPGA 0
#define WRS_HIST_TEMP_PLL  1
#define WRS_HIST_TEMP_PSL  2
#define WRS_HIST_TEMP_PSR  3

#define WRS_HIST_RUN_NAND_MAGIC 0xDADA5D00
#define WRS_HIST_RUN_NAND_MAGIC_MASK 0xFFFFFF00
#define WRS_HIST_RUN_NAND_MAGIC_VER 1
#define WRS_HIST_RUN_NAND_MAGIC_VER_MASK 0xFF

#define WRS_HIST_RUN_SPI_MAGIC 0x5ADA5500
#define WRS_HIST_RUN_SPI_MAGIC_MASK 0xFFFFFF00
#define WRS_HIST_RUN_SPI_MAGIC_VER 1
#define WRS_HIST_RUN_SPI_MAGIC_VER_MASK 0xFF


struct wrs_hist_run_nand {
	uint32_t magic; /* 24bits magic + 8bits version */
	uint32_t lifetime; /* in seconds, ~136 years */
	uint32_t timestamp; /* in seconds, ~136 years */
	uint8_t temp[4]; /* average temperature over last hour */
};
/* 16 bytes/h, ~140KB/year ~1MB/7.5years*/

struct wrs_hist_run_spi {
	uint32_t magic; /* 24bits magic + 8bits version */
	uint32_t lifetime; /* in seconds, ~136 years */
	uint32_t timestamp; /* in seconds, ~136 years */
	uint16_t temp[4][64]; /* histogram, tens of hours of particular
			       * temperature 512 bytes */
};
/*  */
/* Right now we have 7614 erase blocks available in SPI = ~8MB */
/* 1056 each SPI erase block, 528 is a half of it, 264 quater */
/* 0x108000 = 1081344 (~1MB) gives us 40,96 years, so we can take one meg or 2x1MB or 2x0.5MB
 */

#define WRS_HIST_MAX_SFPS 100

struct wrs_hist_sfp_entry {
	char vn[16];
	char pn[16];
	char sn[16];
	uint32_t sfp_uptime;
	uint32_t last_seen; /* value from lifetime, when sfp was last seen
			     */
	struct wrs_hist_sfp_temp *sfp_temp; /* temperature histogram if
					     * available */
};

struct wrs_hist_sfp_temp {
	uint8_t temp[64]; /* 64 bytes */
};

/* size 9 + 50*56 + 50*64 = ~6KB */
struct wrs_hist_sfp_nand {
	uint32_t magic; /* 24bits magic + 8bits version */
	uint32_t timestamp; /* in seconds, ~136 years */
	struct wrs_hist_sfp_entry sfps[WRS_HIST_MAX_SFPS];
};

#endif /* __LIBWR_HIST_H__ */
