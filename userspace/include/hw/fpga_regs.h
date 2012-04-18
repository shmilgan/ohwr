#ifndef __FPGA_IO_H
#define __FPGA_IO_H

#include <sys/types.h>
#include <inttypes.h>
#include "pio.h"

#define FPGA_BASE_ADDR _fpga_base_virt


extern volatile uint8_t *_fpga_base_virt;

#define _fpga_writel(reg, val){ *(volatile uint32_t *)(FPGA_BASE_ADDR + (reg)) = (val); }
#define _fpga_readl(reg) (*(volatile uint32_t *)(FPGA_BASE_ADDR + (reg)))

#define FPGA_BASE_PPS_GEN 	   0x10500
#define FPGA_BASE_RTU 	   0x60000

#endif
