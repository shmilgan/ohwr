#ifndef __FPGA_IO_H
#define __FPGA_IO_H

#include <stdint.h>


/* Base addresses of all FPGA peripherals used in libswitchhw */

/* PPS Generator */
#define FPGA_BASE_PPS_GEN  0x10500

/* Routing Table */
#define FPGA_BASE_RTU 	   0x60000

/* Simple PWM module */
#define FPGA_BASE_SPWM 		0x55000

/* HW Info Unit */
#define FPGA_BASE_HWIU    0x57800

/* Topology Resolution Unit */
#define FPGA_BASE_TRU 	  0x56000 // 0x57000

/* Time-Aware Traffic Shaper Unit */
#define FPGA_BASE_TATSU 	  0x57000 // 0x57000

/* per-port Statistics */
#define FPGA_BASE_PSTATS 0x58000

/* HardWare Debugging Unit */
#define FPGA_BASE_HWDU 	  0x59000 

/* Endpoint  */
#define FPGA_BASE_EP0 	   0x30000

/* Time-Aware Traffic Shaper Unit */
#define FPGA_BASE_TATSU 	  0x59000 // 0x57000

/* per-port Statistics */
#define FPGA_BASE_PSTATS 0x70000

/* HardWare Debugging Unit */
#define FPGA_BASE_HWDU 	  0x71000 

/* Endpoint  */
#define FPGA_BASE_EP0 	   0x30000

#define FPGA_BASE_ADDR _fpga_base_virt

extern volatile uint8_t *_fpga_base_virt;

#define _fpga_writel(reg, val){ *(volatile uint32_t *)(FPGA_BASE_ADDR + (reg)) = (val); }
#define _fpga_readl(reg) (*(volatile uint32_t *)(FPGA_BASE_ADDR + (reg)))

#endif
