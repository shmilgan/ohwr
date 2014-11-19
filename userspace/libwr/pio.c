#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <fcntl.h>

#include <at91/at91sam9g45.h>
#include <at91/at91_pmc.h>
#include <at91/at91_pio.h>

#include <libwr/pio.h>
#include "trace.h"
#include "shw_io.h"
#include "util.h"

volatile uint8_t *_pio_base[4][NUM_PIO_BANKS+1];
volatile uint8_t *_sys_base;

#define AT91C_BASE_PIOA_RAW      0xFFFFF200
#define AT91C_BASE_PIOB_RAW      0xFFFFF400
#define AT91C_BASE_PIOC_RAW      0xFFFFF600
#define AT91C_BASE_PIOD_RAW      0xFFFFF800
#define AT91C_BASE_PIOE_RAW      0xFFFFFA00
#define AT91C_BASE_SYS_RAW       0xFFFFC000
#define AT91C_BASE_PMC_RAW       0xFFFFFC00



static void pmc_enable_clock(int clock)
{
    _writel(_sys_base + AT91C_BASE_PMC_RAW - AT91C_BASE_SYS_RAW + 0x10, (1<<clock)); // fucking atmel headers
//    printf("ClkStat: %x\n", _readl(_sys_base + AT91C_BASE_PMC_RAW - AT91C_BASE_SYS_RAW + 0x18));
}

int shw_pio_mmap_init()
{
    int i;
    int fd = open("/dev/mem", O_RDWR);

    if (!fd)
    {
    	TRACE(TRACE_FATAL, "can't open /dev/mem! (no root?)");
    	exit(-1);
    }

    _sys_base = mmap(NULL, 0x4000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)AT91C_BASE_SYS);

    if (_sys_base == NULL)
    {
    	TRACE(TRACE_FATAL, "can't mmap CPU GPIO regs");
        close(fd);
    	exit(-1);
    }

    TRACE(TRACE_INFO, "AT91_SYS virtual base = 0x%08x", _sys_base);

    pmc_enable_clock(2); /* enable PIO clocks */
    pmc_enable_clock(3);
    pmc_enable_clock(4);
    pmc_enable_clock(5);

//  fprintf(stderr,"AT91_SYS mmapped to: 0x%08x\n", _sys_base);

//    printf("PIOA offset %08X\n", AT91C_BASE_PIOA_RAW - AT91C_BASE_SYS_RAW);
//    printf("Sys base: %08X\n", _sys_base);

    _pio_base[IDX_REG_BASE][PIOA] = _sys_base + AT91C_BASE_PIOA_RAW - AT91C_BASE_SYS_RAW;		//offset counting from AT91C_BASE_SYS
    _pio_base[IDX_REG_BASE][PIOB] = _sys_base + AT91C_BASE_PIOB_RAW - AT91C_BASE_SYS_RAW;		//offset counting from AT91C_BASE_SYS
    _pio_base[IDX_REG_BASE][PIOC] = _sys_base + AT91C_BASE_PIOC_RAW - AT91C_BASE_SYS_RAW;		//offset counting from AT91C_BASE_SYS
    _pio_base[IDX_REG_BASE][PIOD] = _sys_base + AT91C_BASE_PIOD_RAW - AT91C_BASE_SYS_RAW;		//offset counting from AT91C_BASE_SYS
    _pio_base[IDX_REG_BASE][PIOE] = _sys_base + AT91C_BASE_PIOE_RAW - AT91C_BASE_SYS_RAW;		//offset counting from AT91C_BASE_SYS

    for (i=1; i<=5; i++)
    {
        _pio_base [IDX_REG_CODR][i] = _pio_base[IDX_REG_BASE][i] + PIO_CODR;
        _pio_base [IDX_REG_SODR][i] = _pio_base[IDX_REG_BASE][i] + PIO_SODR;
        _pio_base [IDX_REG_PDSR][i] = _pio_base[IDX_REG_BASE][i] + PIO_PDSR;
    }

    return 0;
}


extern const pio_pin_t* _all_cpu_gpio_pins[];
extern const pio_pin_t* _all_fpga_gpio_pins[];


void shw_pio_toggle_pin(pio_pin_t* pin, uint32_t udelay)
{
    while (1)
    {    
	shw_pio_set(pin, 0);
	shw_udelay(udelay);
	shw_pio_set(pin, 1);
	shw_udelay(udelay);
    }
}
    
void shw_pio_configure(const pio_pin_t *pin)
{
    uint32_t mask = (1<<pin->pin);
    uint32_t ddr;
    volatile uint8_t *base = (_pio_base[IDX_REG_BASE][pin->port]);

    switch (pin->port)
    {
    case PIOA:
    case PIOB:
    case PIOC:
    case PIOD:
    case PIOE:

//	printf("-- configure CPU PIO PIN: P%c.%d base=0x%x\n\n", pin->port-PIOA+'A', pin->pin, base);

        _writel(base + PIO_IDR, mask);	// disable irq

        if (pin->mode & PIO_MODE_PULLUP)
            _writel(base + PIO_PUER, mask);// enable pullup
        else
            _writel(base + PIO_PUDR, mask);	// disable pullup

        switch (pin->mode & 0x3)
        {
        case PIO_MODE_GPIO:
            _writel(base + PIO_PER, mask);	// enable gpio mode
            break;
        case PIO_MODE_PERIPH_A:
            _writel(base + PIO_PDR, mask);	// disable gpio mode
            _writel(base + PIO_ASR, mask);	// select peripheral A
            break;
        case PIO_MODE_PERIPH_B:
            _writel(base + PIO_PDR, mask);	// disable gpio mode
            _writel(base + PIO_BSR, mask);	// select peripheral B
            break;
        }

        if (pin->dir == PIO_OUT_1)
        {
            _writel(base + PIO_SODR, mask);
            _writel(base + PIO_OER, mask);	// select output, set it to 1
        } else if (pin->dir == PIO_OUT_0)
        {
            _writel(base + PIO_CODR, mask);
            _writel(base + PIO_OER, mask);	// select output, set it to 0
        } else {
            _writel(base + PIO_ODR, mask);	// select input
        }
        break;
        
        
    case PIO_FPGA:
        ddr = _readl(base + FPGA_PIO_REG_DDR);

        if (pin->dir == PIO_OUT_1)
        {
            _writel(base + FPGA_PIO_REG_SODR, mask);
            ddr |= mask;
        } else if (pin->dir == PIO_OUT_0)
        {
            _writel(base + FPGA_PIO_REG_CODR, mask);
            ddr |= mask;
        } else
            ddr &= ~mask;

        _writel(base + FPGA_PIO_REG_DDR, ddr);
        break;
        
    }		//switch
}


