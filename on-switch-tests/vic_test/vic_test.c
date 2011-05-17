#include <stdio.h>
#include <inttypes.h>
#include <sys/time.h>
#include <signal.h>

#include <hw/switch_hw.h>
#include <hw/clkb_io.h>


#define PIN_IRQ0 25
#define PIN_IRQ3 24


static const pio_pin_t PIN_irq0[] = {{PIO_FPGA, 25, PIO_MODE_GPIO, PIO_OUT_0}, {0}};

static const pio_pin_t PIN_irq3[] = {{PIO_FPGA, 24, PIO_MODE_GPIO, PIO_OUT_0}, {0}};


/* [0x0]: REG VIC Control Register */
#define VIC_REG_CTL 0x00000000
/* [0x4]: REG Raw Interrupt Status Register */
#define VIC_REG_RISR 0x00000004
/* [0x8]: REG Interrupt Enable Register */
#define VIC_REG_IER 0x00000008
/* [0xc]: REG Interrupt Disable Register */
#define VIC_REG_IDR 0x0000000c
/* [0x10]: REG Interrupt Mask Register */
#define VIC_REG_IMR 0x00000010
/* [0x14]: REG Vector Address Register */
#define VIC_REG_VAR 0x00000014
#define VIC_REG_SWIR 0x00000018
#define VIC_REG_EOIR 0x0000001c

#define VIC_CTL_ENABLE                        (1<<0)
#define VIC_CTL_POL                           (1<<1)

#define VIC_IVT_BASE 0x00000080
#define FPGA_BASE_VIC 0x60000

main()
{
  int i;
 trace_log_stderr();
 shw_init();
 
 shw_pio_configure(PIN_irq0);
 shw_pio_configure(PIN_irq3);

 system("/sbin/insmod /tmp/whiterabbit_vic.ko");
 
 _fpga_writel(FPGA_BASE_VIC+VIC_REG_SWIR, 8);

 system("/sbin/rmmod whiterabbit_vic");


}


/*
_fpga_writel(FPGA_BASE_VIC+VIC_REG_CTL, VIC_CTL_POL);
 printf("VAR: %x\n", _fpga_readl(FPGA_BASE_VIC + VIC_REG_VAR));

 _fpga_writel(FPGA_BASE_VIC+VIC_REG_IDR, 0xffffffff);
 int i;

 for(i=0;i<32;i++)
 _fpga_writel(FPGA_BASE_VIC+VIC_IVT_BASE + (i*4), i+1);

 _fpga_writel(FPGA_BASE_VIC+VIC_REG_IER, 8+1);
 _fpga_writel(FPGA_BASE_VIC+VIC_REG_CTL, VIC_CTL_POL | VIC_CTL_ENABLE);

 printf("IRQ line status: %d\n", shw_pio_get(PIN_cpu_irq0));
 printf("RISR: %x\n", _fpga_readl(FPGA_BASE_VIC + VIC_REG_RISR));
 printf("IMR: %x\n", _fpga_readl(FPGA_BASE_VIC + VIC_REG_IMR));

 shw_pio_set0(PIN_irq0);
 shw_pio_set0(PIN_irq3);

 for(i=0;i<10;i++)
   {
   printf("IRQ line status: %d\n", shw_pio_get(PIN_cpu_irq0));
   }

 printf("VAR: %x\n", _fpga_readl(FPGA_BASE_VIC + VIC_REG_VAR));
 shw_pio_set0(PIN_irq0);
 _fpga_writel(FPGA_BASE_VIC + VIC_REG_EOIR, 0);
 printf("VAR: %x\n", _fpga_readl(FPGA_BASE_VIC + VIC_REG_VAR));
 shw_pio_set0(PIN_irq3);
 _fpga_writel(FPGA_BASE_VIC + VIC_REG_EOIR, 0);
 printf("VAR: %x\n", _fpga_readl(FPGA_BASE_VIC + VIC_REG_VAR));
 

*/
