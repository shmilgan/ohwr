#ifndef __FPGABOOT_H
#define __FPGABOOT_H

#include <inttypes.h>

#define FPGA_ID_MAIN 0
#define FPGA_ID_CLKB 1

#define DEFAULT_FPGA_IMAGE "/fpga/fpga.img"
#define REVISION_ANY -1

static const char FPGA_IMAGE_MAGIC[4] = { 'w','r','f','i' };

struct fpga_image_header {
	char magic[4];
	uint32_t num_fpgas;
};

struct fpga_image_entry {
  char *fpga_name; // name of the FPGA (for example: MAIN, CLKB)
  char *fw_name;   // name of the firmware (for example: board_test, rtu_test).
  uint32_t hash_reg; // MD5 hash of the firmware ID.
  uint32_t revision;
  uint32_t size;
  uint32_t compressed_size;
  uint8_t *image_buf;
};


int mblaster_init();

int shw_fpgaboot_init();

int shw_load_fpga_bitstream(int fpga_id, uint8_t *bitstream, uint32_t bitstream_size);
int shw_request_fpga_firmware(int fpga_id, const char *firmware_name);
int shw_boot_fpga(int fpga_id);
int shw_set_fpga_firmware_path(const char *path);

#endif
