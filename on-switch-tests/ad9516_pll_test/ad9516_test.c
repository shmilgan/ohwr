#include <stdio.h>

#include <hw/switch_hw.h>

#define NUM_LEDS 4

uint32_t clkgen_rval(void *clkctl, unsigned int addr);

#define AD9516_MAX_REGS 1024

struct ad9516_regs {
	struct {
		uint16_t addr;
		uint8_t value;
	} regs[AD9516_MAX_REGS];
	int nregs;
};

int ad9516_load_regset(const char *filename, struct ad9516_regs *regs)
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
		
//		printf("%s\n",str);
		if(!strncmp(str, "Addr(Hex)", 8)) start_read = 1;
		
		if(start_read)
		{
			if( sscanf(str, "%04x %08s %02x\n", &addr, tmp, &val) == 3)
			{
				TRACE(TRACE_INFO, "   -> ad9516_reg[0x%x]=0x%x", addr, val);
				regs->regs[n].addr = addr;
				regs->regs[n].value = val;
				n++;
			}
		}
	}

	regs->nregs = n;
	fclose(f);
	return 0;

}

int ad9516_set_state(struct ad9516_regs *regs)
{
	int i;
	TRACE(TRACE_INFO, "Loading AD9516 state (%d registers)", regs->nregs);

	for(i=0;i<regs->nregs;i++)
		clkgen_write(NULL, regs->regs[i].addr, regs->regs[i].value);

	clkgen_write(NULL, 0x232, 1);

	for(i=0;i<regs->nregs;i++)
	{
		uint8_t v = clkgen_rval(NULL, regs->regs[i].addr); 
		if(regs->regs[i].value != v) printf("%x:%x/%x\n", regs->regs[i].addr, regs->regs[i].value,  v);
	}


}

main()
{
	char s[2000];
	int addr;
	int val;
    unsigned int i=1;

	trace_log_stderr();
	shw_init();

	ad9516_spi_init();
	struct ad9516_regs r;

//	shw_pio_set0(PIN_ad9516_refsel);
	ad9516_load_regset("/tmp/regs.txt", &r);
	ad9516_set_state(&r);


}
