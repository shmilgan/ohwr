#include <math.h>

#include <hw/pio.h>
#include <hw/trace.h>

#include <hw/ad9516.h>

#define SPI_DELAY 50

#define AD9516_OP_READ 0x80
#define AD9516_OP_WRITE 0x00

#define AD9516_MAX_REGS 1024

struct ad9516_regs {
	struct {
		uint16_t addr;
		uint8_t value;
	} regs[AD9516_MAX_REGS];
	int nregs;
};

int shw_ad9516_set_output_delay(int output, float delay_ns, int bypass);

#include "ad9516_default_regs.h"

static int use_ext_ref = 0;

int shw_use_external_reference(int enable)
{
	if(enable)
		TRACE(TRACE_INFO, "Using external 10 MHz reference clock (grandmaster mode)");
	use_ext_ref = enable;
}



static uint8_t ad9516_spi_read8()
{
    uint8_t rx = 0;
    int i;

    shw_pio_setdir(PIN_ad9516_sdio, PIO_IN);
    shw_pio_set0(PIN_ad9516_sclk);
    for(i = 0; i < 8;i++)
    {
        shw_udelay(SPI_DELAY);

        rx <<= 1;
        if (shw_pio_get(PIN_ad9516_sdio))
            rx |= 1;

        shw_udelay(SPI_DELAY);
        shw_pio_set1(PIN_ad9516_sclk);
        shw_udelay(SPI_DELAY);
        shw_pio_set0(PIN_ad9516_sclk);
    }
    shw_udelay(SPI_DELAY);
    return rx;
}

static uint8_t ad9516_spi_write8(uint8_t tx)
{
    int i;

    shw_pio_setdir(PIN_ad9516_sdio, PIO_OUT);
    shw_pio_set0(PIN_ad9516_sclk);
    for(i = 0; i < 8;i++)
    {
        shw_udelay(SPI_DELAY);

        shw_pio_set(PIN_ad9516_sdio, tx & 0x80);
        tx<<=1;

        shw_udelay(SPI_DELAY);
        shw_pio_set1(PIN_ad9516_sclk);
        shw_udelay(SPI_DELAY);
        shw_pio_set0(PIN_ad9516_sclk);
    }
    shw_udelay(SPI_DELAY);
}

static uint8_t ad9516_read_reg(uint16_t addr)
{
	uint8_t val;

	shw_pio_set0(PIN_ad9516_cs);
	shw_udelay(SPI_DELAY);

	ad9516_spi_write8(AD9516_OP_READ | (addr >> 8));
    ad9516_spi_write8(addr & 0xff);
    val = ad9516_spi_read8();

    shw_udelay(SPI_DELAY);
	shw_pio_set1(PIN_ad9516_cs);
	shw_udelay(SPI_DELAY);

	return val;
}

static void ad9516_write_reg(uint16_t addr, uint8_t data)
{
	shw_pio_set0(PIN_ad9516_cs);
	shw_udelay(SPI_DELAY);

	ad9516_spi_write8(AD9516_OP_WRITE | (addr >> 8));
    ad9516_spi_write8(addr & 0xff);
    ad9516_spi_write8(data);

    shw_udelay(SPI_DELAY);
	shw_pio_set1(PIN_ad9516_cs);
	shw_udelay(SPI_DELAY);
}

static void ad9516_reset()
{
    shw_pio_set0(PIN_ad9516_nrst); // reset the PLL
    shw_udelay(100);
    shw_pio_set1(PIN_ad9516_nrst); // un-reset the PLL
    shw_udelay(100);
}


int ad9516_detect_external_ref()
{
	ad9516_write_reg(0x4, 0x1); // readback active regs
	ad9516_write_reg(0x1c, 0x06); // enable ref1/reg2
	ad9516_write_reg(0x1b, 0xe0); // enable refin monitor
	ad9516_write_reg(0x232, 1); // commit
	usleep(200000);
	return ad9516_read_reg(0x1f) ;
}


static int ad9516_load_regs_from_file(const char *filename, struct ad9516_regs *regs)
{
	FILE *f;
	char str[1024], tmp[100];
	int start_read = 0, n = 0;
	uint32_t addr, val;

	f = fopen(filename ,"rb");
	if(!f)
	{
		TRACE(TRACE_ERROR, "can't open AD9516 regset file: %s", filename);
		return -1;
	}

	while(!feof(f))
	{
		fgets(str, 1024, f);

		if(!strncmp(str, "Addr(Hex)", 8)) start_read = 1;

		if(start_read)
		{
			if( sscanf(str, "%04x %08s %02x\n", &addr, tmp, &val) == 3)
			{
				//TRACE(TRACE_INFO, "   -> ad9516_reg[0x%x]=0x%x", addr, val);
				regs->regs[n].addr = addr;
				regs->regs[n].value = val;
				n++;
			}
		}
	}


	regs->nregs = n;
	fclose(f);

	return start_read == 1 ? 0 : -1;
}

static int ad9516_load_state(const struct ad9516_regs *regs)
{
	int i;
	TRACE(TRACE_INFO, "Loading AD9516 state (%d registers)", regs->nregs);

	for(i=0;i<regs->nregs;i++)
		ad9516_write_reg(regs->regs[i].addr, regs->regs[i].value);

	ad9516_write_reg(AD9516_UPDATE_ALL, 1); // acknowledge register update

	return 0;
}

static int ad9516_check_lock()
{
    uint8_t pll_readback = ad9516_read_reg(AD9516_PLLREADBACK);

    return pll_readback & 1;
}


static void ad9516_power_down()
{

}

#define assert_init(proc) { int ret; if((ret = proc) < 0) return ret; }


int shw_ad9516_init()
{
    int retries = 100;

    TRACE(TRACE_INFO, "Initializing AD9516 PLL....");

    shw_pio_configure(PIN_ad9516_cs);
    shw_pio_configure(PIN_ad9516_nrst);
    shw_pio_configure(PIN_ad9516_refsel);
    shw_pio_configure(PIN_ad9516_sclk);
    shw_pio_configure(PIN_ad9516_sdio);
    shw_pio_configure(PIN_ad9516_sdo);

    shw_udelay(100);

    ad9516_reset();
    //    ad9516_power_down();

    shw_udelay(10000);



    if(ad9516_read_reg(AD9516_SERIALPORT) != 0x18)
    {
        TRACE(TRACE_FATAL, "AD9516 not responding!");
        return -1;
    }

	if(use_ext_ref)
	{
    assert_init(ad9516_load_state(&ad9516_regs_ext_10m));
	} else {
	  assert_init(ad9516_load_state(&ad9516_regs_tcxo_25m));
	}

// wait for the PLL to lock
    while(retries--)
    {
        if(ad9516_check_lock()) break;
        shw_udelay(1000);
    }

		shw_ad9516_set_output_delay(9, 0.5, 0);

//	 	TRACE(TRACE_INFO, "LockReg: %d\n",

    return 0;
}

static int find_optimal_params(float delay_ns, int *caps, int *frac, int *ramp, float *best)
{
 int r, i, ncaps, f;
 float best_error = 10000;


	for(r = 0; r < 8; r++)
	{
		for(i=0;i<8;i++)
		{
			if(i == 0) ncaps= 0;
			else if(i==1 || i==2 || i==4) ncaps = 1;
			else if(i==3 || i==6 || i==5) ncaps = 2;
			else ncaps = 3;

			for(f= 0;f<=0x2f;f++)
			{
				float iramp = (float)(r+1)*200.0;
				float del_range = 200.0 * ((float)(ncaps+3)/iramp) * 1.3286;
				float offset = 0.34 + (1600.0 - iramp) * 10e-4 + (float)(ncaps-1)/iramp*6.0;

//				printf("range: %.3f offset %.3f\n", del_range, offset);
				float del_fine = del_range * (float)f / 63.0  + offset;
				if(fabs(del_fine - delay_ns) < best_error)
				{
	//				printf("New Best: %.3f\n", del_fine);
					best_error = fabs(del_fine - delay_ns);
					*best = del_fine;
					*caps = i;
					*ramp = r;
					*frac = f;
				}
			}
		}
	}
}

int shw_ad9516_set_output_delay(int output, float delay_ns, int bypass)
{
    uint16_t regbase = 0xa0 + 3*(output - 6);
		int ramp,frac,caps;
		float best_dly;

		find_optimal_params(delay_ns,&caps, &frac, &ramp, &best_dly );

//		printf("Opt: caps %d frac %d ramp %d best %.5f req %.5f regbase %x\n", caps, frac, ramp, best_dly, delay_ns, regbase);



    ad9516_write_reg(regbase, bypass?1:0);
    ad9516_write_reg(regbase+1, (caps << 3) | (ramp));
    ad9516_write_reg(regbase+2, frac);

    ad9516_write_reg(AD9516_UPDATE_ALL, 1); // acknowledge register update

    return 0;
}

