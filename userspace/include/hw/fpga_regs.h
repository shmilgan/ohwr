#ifndef __FPGA_IO_H
#define __FPGA_IO_H

#include <sys/types.h>
#include <inttypes.h>
#include "pio.h"

#define FPGA_BASE_ADDR _fpga_base_virt


extern volatile uint8_t *_fpga_base_virt;

#define _fpga_writel(reg, val){ *(volatile uint32_t *)(FPGA_BASE_ADDR + (reg)) = (val); }
#define _fpga_readl(reg) (*(volatile uint32_t *)(FPGA_BASE_ADDR + (reg)))

#define _FPGA_BUS_2_ADDR(x) ((x) * 0x10000)

#define FPGA_BASE_REVID 		_FPGA_BUS_2_ADDR(0)
#define FPGA_BASE_GPIO 			_FPGA_BUS_2_ADDR(1)
#define FPGA_BASE_SPIM			_FPGA_BUS_2_ADDR(2)
#define FPGA_BASE_VIC 			_FPGA_BUS_2_ADDR(3)
#define FPGA_BASE_EP_UP0 _FPGA_BUS_2_ADDR(4)
#define FPGA_BASE_EP_UP1 _FPGA_BUS_2_ADDR(5)
#define FPGA_BASE_EP_DP0 _FPGA_BUS_2_ADDR(6)
#define FPGA_BASE_EP_DP1 _FPGA_BUS_2_ADDR(7)
#define FPGA_BASE_EP_DP2 _FPGA_BUS_2_ADDR(8)
#define FPGA_BASE_EP_DP3 _FPGA_BUS_2_ADDR(9)
#define FPGA_BASE_EP_DP4 _FPGA_BUS_2_ADDR(10)
#define FPGA_BASE_EP_DP5 _FPGA_BUS_2_ADDR(11)
#define FPGA_BASE_EP_DP6 _FPGA_BUS_2_ADDR(12)
#define FPGA_BASE_EP_DP7    _FPGA_BUS_2_ADDR(13)
#define FPGA_BASE_PPS_GEN 	   _FPGA_BUS_2_ADDR(14)
#define FPGA_BASE_CALIBRATOR   _FPGA_BUS_2_ADDR(15) 
#define FPGA_BASE_RTU 				 _FPGA_BUS_2_ADDR(16) 
#define FPGA_BASE_RTU_TESTUNIT _FPGA_BUS_2_ADDR(17) 

#define GPIO_REG_CODR 0x0
#define GPIO_REG_SODR 0x4
#define GPIO_REG_DDR  0x8
#define GPIO_REG_PSR  0xc

#define SPI_REG_SPITX 0x4
#define SPI_REG_SPIRX 0x8
#define SPI_REG_SPICTL 0x0

#define SPICTL_ENABLE(x) ((x)?(1<<2):0)
#define SPICTL_CSEN(x) ((x)?(1<<0):0)
#define SPICTL_CSDIS(x) ((x)?(1<<1):0)
#define SPICTL_BUSY(rval) ((rval)&(1<<3)?1:0)
#define SPICTL_CLKDIV(value) (((value)&0xff) << 8)

#define GPIO_PIN_LED0_A 0
#define GPIO_PIN_LED0_K 1
#define GPIO_PIN_LED1_A 2
#define GPIO_PIN_LED1_K 3
#define GPIO_PIN_LED2_A 4
#define GPIO_PIN_LED2_K 5
#define GPIO_PIN_LED3_A 6
#define GPIO_PIN_LED3_K 7
#define GPIO_PIN_CLKB_RST 31

#define GPIO_PIN_UP0_SFP_SDA 8
#define GPIO_PIN_UP0_SFP_SCL 9
#define GPIO_PIN_UP0_SFP_LOS 10
#define GPIO_PIN_UP0_SFP_TX_FAULT 11
#define GPIO_PIN_UP0_SFP_TX_DISABLE 12
#define GPIO_PIN_UP0_SFP_DETECT 13
#define GPIO_PIN_UP0_PRBSEN 14
#define GPIO_PIN_UP0_SYNCEN 15
#define GPIO_PIN_UP0_LOOPEN 16
#define GPIO_PIN_UP0_ENABLE 17

#define GPIO_PIN_UP1_SFP_SDA 18
#define GPIO_PIN_UP1_SFP_SCL 19
#define GPIO_PIN_UP1_SFP_LOS 20
#define GPIO_PIN_UP1_SFP_TX_FAULT 21
#define GPIO_PIN_UP1_SFP_TX_DISABLE 22
#define GPIO_PIN_UP1_SFP_DETECT 23
#define GPIO_PIN_UP1_PRBSEN 24
#define GPIO_PIN_UP1_SYNCEN 25
#define GPIO_PIN_UP1_LOOPEN 26
#define GPIO_PIN_UP1_ENABLE 27



// GigaSpy wishbone registers

#define GIGASPY_MEM_SIZE 8192

#define GSPY_REG_GSCTL 			0x0
#define GSPY_REG_GSTRIGCTL 		0x4
#define GSPY_REG_GSSTAT 		0x8
#define GSPY_REG_GSNSAMPLES 	0xc
#define GSPY_REG_GSTRIGADDR 	0x10
#define GSPY_REG_GSTESTCTL   	0x14
#define GSPY_REG_GSTESTCNT   	0x18
#define GSPY_REG_GSCLKFREQ   	0x1c

#define GSPY_MEM_CH0			0x8000
#define GSPY_MEM_CH1			0x10000

#define GSPY_GSCTL_CH0_ENABLE(x) ((x)?(1<<0):(0))
#define GSPY_GSCTL_CH1_ENABLE(x) ((x)?(1<<1):(0))
#define GSPY_GSCTL_SLAVE0_ENABLE(x) ((x)?(1<<2):(0))
#define GSPY_GSCTL_SLAVE1_ENABLE(x) ((x)?(1<<3):(0))
#define GSPY_GSCTL_LOAD_TRIG0(x) ((x)?(1<<4):(0))
#define GSPY_GSCTL_LOAD_TRIG1(x) ((x)?(1<<5):(0))
#define GSPY_GSCTL_RESET_TRIG0(x) ((x)?(1<<6):(0))
#define GSPY_GSCTL_RESET_TRIG1(x) ((x)?(1<<7):(0))

#define GSPY_GSTRIGCTL_TRIG0_VAL(k,v) (((k)?(1<<8):0) | ((v) & 0xff))
#define GSPY_GSTRIGCTL_TRIG1_VAL(k,v) (((k)?(1<<24):0) | (((v) & 0xff)<<16))

#define GSPY_GSTRIGCTL_TRIG0_EN(x) (((x)?(1<<15):0))
#define GSPY_GSTRIGCTL_TRIG1_EN(x) (((x)?(1<<31):0))

#define GSPY_GSNSAMPLES(x) ((x)&0x1fff)

#define GSPY_GSSTAT_TRIG_DONE0(reg) ((reg)&(1<<0)?1:0)
#define GSPY_GSSTAT_TRIG_DONE1(reg) ((reg)&(1<<1)?1:0)

#define GSPY_GSSTAT_TRIG_SLAVE0(reg) ((reg)&(1<<2)?1:0)
#define GSPY_GSSTAT_TRIG_SLAVE1(reg) ((reg)&(1<<3)?1:0)

#define GSPY_GSTRIGADDR_CH0(reg) ((reg)&0x1fff)
#define GSPY_GSTRIGADDR_CH1(reg) (((reg)>>16)&0x1fff)

#define GSPY_GSTESTCTL_ENABLE   0x1
#define GSPY_GSTESTCTL_RST_CNTR  0x2
#define GSPY_GSTESTCTL_CONNECT   0x4

#define GSPY_GSTESTCTL_PHYIO_ENABLE 0x100
#define GSPY_GSTESTCTL_PHYIO_SYNCEN 0x200
#define GSPY_GSTESTCTL_PHYIO_LOOPEN 0x400
#define GSPY_GSTESTCTL_PHYIO_PRBSEN 0x800


#endif
