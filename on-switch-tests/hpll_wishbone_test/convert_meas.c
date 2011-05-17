#include <stdio.h>
#include <stdlib.h>
#include <hw/hpll_regs.h>

int sign_extend(uint32_t val, int nbits)
{
 if(val & (1<<(nbits-1))) return (0xffffffff ^ ((1<<nbits)-1)) | val; else return val;
}


main(int argc, char *argv[])
{
 if(argc < 3) return -1;
 FILE *fin, *fout;
 uint32_t r0;
 int npoints;
 int i = 0;
 fin = fopen(argv[1], "rb");
 fout = fopen(argv[2], "wb");
 
 npoints = atoi(argv[3]);
 
 while(!feof(fin) && (++i < npoints))
 {
	 fread(&r0, 4, 1, fin);

	 fprintf(fout, "%d %d %d %d\n", i,
		sign_extend(HPLL_RFIFO_R0_ERR_VAL_R(r0), 12),
		HPLL_RFIFO_R0_DAC_VAL_R(r0),
		r0 & HPLL_RFIFO_R0_FP_MODE ? 60000 : 0);
	
		
 }
 
 
}