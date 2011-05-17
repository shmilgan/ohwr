#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <fcntl.h>

#include <hw/pio.h>
#include <hw/fpga_regs.h>
#include <hw/trace.h>

static const pio_pin_t pck_port_mapping[] =
{
    {PIOA,    13,  PIO_MODE_PERIPH_B, PIO_OUT},
    {PIOB,    10,  PIO_MODE_PERIPH_B, PIO_OUT},
    {PIOA,    6,   PIO_MODE_PERIPH_B, PIO_OUT},
    {PIOE,    11,  PIO_MODE_PERIPH_B, PIO_OUT},
    {0}
};

volatile uint8_t *_pio_base[4][NUM_PIO_BANKS+1];
volatile uint8_t *_sys_base;

int shw_pio_mmap_init()
{
    int i;
    int fd = open("/dev/mem", O_RDWR);

    if (!fd)
    {
    	TRACE(TRACE_FATAL, "can't open /dev/mem! (no root?)");
    	exit(-1);
    }

    _sys_base = mmap(NULL, 0x2000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, AT91_BASE_SYS);

    if (_sys_base == NULL)
    {
    	TRACE(TRACE_FATAL, "can't mmap CPU GPIO regs");
        close(fd);
    	exit(-1);
    }

    TRACE(TRACE_INFO, "AT91_SYS virtual base = 0x%08x", _sys_base);


//  fprintf(stderr,"AT91_SYS mmapped to: 0x%08x\n", _sys_base);

    _pio_base[REG_BASE][PIOA] = _sys_base + AT91_PIOA;
    _pio_base[REG_BASE][PIOB] = _sys_base + AT91_PIOB;
    _pio_base[REG_BASE][PIOC] = _sys_base + AT91_PIOC;
    _pio_base[REG_BASE][PIOD] = _sys_base + AT91_PIOD;
    _pio_base[REG_BASE][PIOE] = _sys_base + AT91_PIOE;

    for (i=1; i<=5; i++)
    {
        _pio_base [REG_CODR][i] = _pio_base[REG_BASE][i] + PIO_CODR;
        _pio_base [REG_SODR][i] = _pio_base[REG_BASE][i] + PIO_SODR;
        _pio_base [REG_PDSR][i] = _pio_base[REG_BASE][i] + PIO_PDSR;
    }

    _pio_base[REG_BASE][PIO_FPGA] = _fpga_base_virt + FPGA_BASE_GPIO;
    _pio_base[REG_CODR][PIO_FPGA] = _fpga_base_virt + FPGA_BASE_GPIO + GPIO_REG_CODR;
    _pio_base[REG_SODR][PIO_FPGA] = _fpga_base_virt + FPGA_BASE_GPIO + GPIO_REG_SODR;
    _pio_base[REG_PDSR][PIO_FPGA] = _fpga_base_virt + FPGA_BASE_GPIO + GPIO_REG_PSR;
    return 0;
}

int shw_pio_configure_all_cpu_pins()
{
	int i;
	TRACE(TRACE_INFO,"Configuring CPU PIO pins...");
	for(i=0; _all_cpu_gpio_pins[i]; i++)
	{
		shw_pio_configure_pins(_all_cpu_gpio_pins[i]);
	}
	TRACE(TRACE_INFO,"...done!");
}

int shw_pio_configure_all_fpga_pins()
{
	int i;
	TRACE(TRACE_INFO,"Configuring FPGA PIO pins...");
	for(i=0; _all_fpga_gpio_pins[i]; i++)
	{
		shw_pio_configure_pins(_all_fpga_gpio_pins[i]);
	}
	TRACE(TRACE_INFO,"...done!");
}


void shw_pio_configure(const pio_pin_t *pin)
{
    uint32_t mask = (1<<pin->pin);
    uint32_t ddr;
    volatile uint8_t *base = (_pio_base[REG_BASE][pin->port]);

    switch (pin->port)
    {
    case PIOA:
    case PIOB:
    case PIOC:
    case PIOD:
    case PIOE:

//		TRACE(TRACE_INFO, "-- configure CPU PIO PIN: P%c.%d base=0x%x", pin->port-PIOA+'A', pin->pin, base);

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

//		TRACE(TRACE_INFO, "-- configure FPGA PIO PIN: P%d", pin->pin);

        ddr = _readl(base + GPIO_REG_DDR);

        if (pin->dir == PIO_OUT_1)
        {
            _writel(base + GPIO_REG_SODR, mask);
            ddr |= mask;
        } else if (pin->dir == PIO_OUT_0)
        {
            _writel(base + GPIO_REG_CODR, mask);
            ddr |= mask;
        } else
            ddr &= ~mask;

        _writel(base + GPIO_REG_DDR, ddr);
        break;
    }
}

void shw_pio_configure_pins(const pio_pin_t *pins)
{
    while (pins->port)
    {
        shw_pio_configure(pins);
        pins++;
    }
}

int shw_clock_out_enable(int pck_num, int prescaler, int source)
{
    if (pck_num > 3) return -1;

    shw_pio_configure(&pck_port_mapping[pck_num]);

    _writel(_sys_base + AT91_PMC_PCKR(pck_num), source | prescaler);
    _writel(_sys_base + AT91_PMC_SCER, (1<< (8+pck_num)));

    return 0;
}

volatile uint8_t *shw_pio_get_sys_base()
{
    return _sys_base;
}

volatile uint8_t *shw_pio_get_port_base(int port)
{
    return _pio_base[REG_BASE][port];
}




void shw_set_fp_led(int led, int state)
{
    if(state == LED_GREEN)
    {
        shw_pio_set(&_fp_leds[led][0], 1);
        shw_pio_set(&_fp_leds[led][1], 0);
    } else if(state == LED_RED) {
        shw_pio_set(&_fp_leds[led][0], 0);
        shw_pio_set(&_fp_leds[led][1], 1);
    } else {
        shw_pio_set(&_fp_leds[led][0], 1);
        shw_pio_set(&_fp_leds[led][1], 1);
    }

}
