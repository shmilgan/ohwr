#include <stdio.h>
#include <inttypes.h>

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
		return -1;
	}
	
	while(!feof(f))
	{
		if(fgets(str, 1024, f) == EOF) break;
		
		
//		printf("%s\n",str);
		if(!strncmp(str, "Addr(Hex)", 8)) start_read = 1;
		
		if(start_read)
		{
			if( sscanf(str, "%04x %08s %02x\n", &addr, tmp, &val) == 3)
			{
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


main(int argc, char *argv[])
{
	struct ad9516_regs r;
	int i;

	ad9516_load_regset(argv[1], &r);

	printf("static const struct ad9516_regs %s = {\n", argv[2]);
	printf("{\n");	
	for(i=0;i<r.nregs;i++)
	{
		printf( " { 0x%04x, 0x%02x},\n", r.regs[i].addr, r.regs[i].value);
	}
	printf("\n{0}},\n %d\n};", r.nregs);

	return 0;

}
