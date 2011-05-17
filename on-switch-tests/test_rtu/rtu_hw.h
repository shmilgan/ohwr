/*
-------------------------------------------------------------------------------
-- Title      : Routing Table Unit Hardware Access Functions
-- Project    : White Rabbit Switch
-------------------------------------------------------------------------------
-- File       : rtu_hw.h
-- Authors    : Maciej Lipinski (maciej.lipinski@cern.ch)
-- Company    : CERN BE-CO-HT
-- Created    : 2010-06-30
-- Last update: 2010-06-30
-- Description: Some usefull addresses in FPGA defined here

*/

#define HW_POLYNOMIAL_DECT   0x0589 
#define HW_POLYNOMIAL_CCITT  0x1021 
#define HW_POLYNOMIAL_IBM    0x8005 



#define FPGA_BASE_RTU   0x60000
#define FPGA_BASE_PORT0 0x80000
#define FPGA_BASE_PORT1 0xa0000
#define FPGA_BASE_PORT2 0xc0000
#define FPGA_BASE_PORT3 0xe0000
#define FPGA_BASE_PORT4 0x100000
#define FPGA_BASE_PORT5 0x120000
#define FPGA_BASE_PORT6 0x140000
#define FPGA_BASE_PORT7 0x160000
#define FPGA_BASE_PORT8 0x180000
#define FPGA_BASE_PORT9 0x1a0000

#define FPGA_RTU_GCR      0x0
#define FPGA_RTU_AGR_HCAM 0x4

//global RTU settings
#define FPGA_RTU_GCR_HT_BASEL_0 0x0
#define FPGA_RTU_GCR_HT_BASEL_1 0x1
#define FPGA_RTU_GCR_HCAM_BSEL_0 0x0
#define FPGA_RTU_GCR_HCAM_BSEL_1 0x2
#define FPGA_RTU_GCR_G_ENA 0x4
#define FPGA_RTU_GCR_G_DIS 0x0

//port settings
#define FPGA_RTU_PCR_LEARN_EN 0x1
#define FPGA_RTU_PCR_PASS_ALL 0x2
#define FPGA_RTU_PCR_PASS_BPDU 0x4
#define FPGA_RTU_PCR_FIX_PRIO 0x8
#define FPGA_RTU_PCR_B_UNREC 0x80

#define FPGA_RTU_PCR0     0x8
#define FPGA_RTU_PCR1     0xC
#define FPGA_RTU_PCR2     0x10
#define FPGA_RTU_PCR3     0x14
#define FPGA_RTU_PCR4     0x18
#define FPGA_RTU_PCR5     0x1C
#define FPGA_RTU_PCR6     0x20
#define FPGA_RTU_PCR7     0x24
#define FPGA_RTU_PCR8     0x28
#define FPGA_RTU_PCR9     0x2C

#define FPGA_RTU_EIC_IDR  0x40
#define FPGA_RTU_EIC_IER  0x44
#define FPGA_RTU_EIC_IMR  0x48
#define FPGA_RTU_EIC_ISR  0x4C
//DMAC
#define FPGA_RTU_UFIFO_R0 0x50
#define FPGA_RTU_UFIFO_R1 0x54
//SMAC
#define FPGA_RTU_UFIFO_R2 0x58
#define FPGA_RTU_UFIFO_R3 0x5c
//other staff
#define FPGA_RTU_UFIFO_R4 0x60
//control/status
#define FPGA_RTU_UFIFO_CSR 0x64
//ad_sel
#define FPGA_RTU_MFIFO_R0  0x68
//ad_value
#define FPGA_RTU_MFIFO_R1  0x6c
//control/status
#define FPGA_RTU_MFIFO_CSR 0x70

#define FPGA_RTU_HCAM      0x4000

#define FPGA_RTU_ARAM_MAIN 0x8000

#define FPGA_RTU_VLAN_TAB  0xc000 


