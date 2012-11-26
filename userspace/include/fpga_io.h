#ifndef __FPGA_IO_H
#define __FPGA_IO_H

#include <stdint.h>


/* Base addresses of all FPGA peripherals used in libswitchhw */

/* PPS Generator */
#define FPGA_BASE_PPS_GEN  0x10500

/* Routing Table */
#define FPGA_BASE_RTU 	   0x60000

/* Topology Resolution Unit */
#define FPGA_BASE_TRU 	   0x57000

#define FPGA_BASE_ADDR _fpga_base_virt

extern volatile uint8_t *_fpga_base_virt;

#define _fpga_writel(reg, val){ *(volatile uint32_t *)(FPGA_BASE_ADDR + (reg)) = (val); }
#define _fpga_readl(reg) (*(volatile uint32_t *)(FPGA_BASE_ADDR + (reg)))

#endif
