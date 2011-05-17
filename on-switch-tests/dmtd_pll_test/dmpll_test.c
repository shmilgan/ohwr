#include <stdio.h>
#include <inttypes.h>
#include <sys/time.h>
#include <signal.h>

#include <hw/switch_hw.h>
#include <hw/clkb_io.h>
#include <hw/dmpll_regs.h>

#define MAX_MEAS 120000

#define DMPLL_CHANNEL_LOCAL 0
#define DMPLL_CHANNEL_UP0 1
#define DMPLL_CHANNEL_UP1 2

uint32_t rec_buf[MAX_MEAS * 4];

int n_meas = 0;

static void dmpll_purge_rfifo()
{

for(;;)
{
  shw_clkb_read_reg(CLKB_BASE_DMPLL + DMPLL_REG_RFIFO_R0);
}

}

static void dmpll_poll_rfifo()
{

for(;;)
{
	if(shw_clkb_read_reg(CLKB_BASE_DMPLL + DMPLL_REG_RFIFO_CSR) & DMPLL_RFIFO_CSR_EMPTY) return;

	if(n_meas < MAX_MEAS) {
	 rec_buf[4*n_meas] = shw_clkb_read_reg(CLKB_BASE_DMPLL + DMPLL_REG_RFIFO_R0);
	 rec_buf[4*n_meas+1] = shw_clkb_read_reg(CLKB_BASE_DMPLL + DMPLL_REG_RFIFO_R1);
	 rec_buf[4*n_meas+2] = shw_clkb_read_reg(CLKB_BASE_DMPLL + DMPLL_REG_RFIFO_R2);
	 rec_buf[4*n_meas+3] = shw_clkb_read_reg(CLKB_BASE_DMPLL + DMPLL_REG_RFIFO_R3);
	 n_meas++;
	}
}
}



static void dmpll_setup_deglitcher(uint32_t reg, int hi, int lo, int glitch)
{
  shw_clkb_write_reg(CLKB_BASE_DMPLL + reg, DMPLL_DGCR_IN0_THR_HI_W(hi) | 
 																						DMPLL_DGCR_IN0_THR_LO_W(lo) | 
 																						DMPLL_DGCR_IN0_THR_GLITCH_W(glitch));

}

static void dmpll_set_phase_shift(int channel, int ps_shift)
{
	switch(channel)
	{
#if 0
	 case DMPLL_CHANNEL_LOCAL: 
		shw_clkb_write_reg(CLKB_BASE_DMPLL + DMPLL_REG_PSCR_LOCAL, ps_shift);
		break;
	 case DMPLL_CHANNEL_UP0: 
		shw_clkb_write_reg(CLKB_BASE_DMPLL + DMPLL_REG_PSCR_0, ps_shift);

		break;
#endif

	 case DMPLL_CHANNEL_UP1: 
		shw_clkb_write_reg(CLKB_BASE_DMPLL + DMPLL_REG_PSCR_IN0, ps_shift);
		break;
		
	}
}



static int dmpll_shifter_busy(int channel)
{
	switch(channel)
	{
/*	 case DMPLL_CHANNEL_LOCAL: 
		return shw_clkb_read_reg(CLKB_BASE_DMPLL + DMPLL_REG_PSCR_LOCAL) & (1<<31);
	 case DMPLL_CHANNEL_UP0: 
		return shw_clkb_read_reg(CLKB_BASE_DMPLL + DMPLL_REG_PSCR_UP0) & (1<<31);*/
	 case DMPLL_CHANNEL_UP1: 
		return shw_clkb_read_reg(CLKB_BASE_DMPLL + DMPLL_REG_PSCR_IN0) & DMPLL_PSCR_IN0_BUSY;
	}
 return 0;	
}

void sighandler(int sig)
{
 fprintf(stderr,"Saving: %d", n_meas);
FILE *f=fopen("/tmp/dmpll_meas.dat","wb");
fwrite(rec_buf, 8, n_meas, f);
fclose(f);
 exit(0);
}

main()
{
 int i;
 int ps_shift = 16000;
 int ps_new_shift = 0;
  trace_log_stderr();
 
  shw_init();

  printf("Waiting for Helper PLL lock...\n");
  while(!shw_hpll_check_lock()) usleep(10000);  

	

  
/*
 for(i=0;i<30;i++)
 {
	 shw_hpll_update(); usleep(100000);
 }*/

 
  printf("Initializing the DMPLL.\n");
  
  shw_clkb_write_reg(CLKB_BASE_DMPLL + DMPLL_REG_PCR, 0);

  dmpll_setup_deglitcher(DMPLL_REG_DGCR_IN0, 150, 150, 60);
  dmpll_setup_deglitcher(DMPLL_REG_DGCR_FB, 150, 150, 60);

// gain
  shw_clkb_write_reg(CLKB_BASE_DMPLL + DMPLL_REG_FBGR, DMPLL_FBGR_F_KP_W(100) | DMPLL_FBGR_F_KI_W(600));
  shw_clkb_write_reg(CLKB_BASE_DMPLL + DMPLL_REG_PBGR, DMPLL_PBGR_P_KP_W(-2000) | DMPLL_PBGR_P_KI_W(-40));
  shw_clkb_write_reg(CLKB_BASE_DMPLL + DMPLL_REG_LDCR, DMPLL_LDCR_LD_THR_W(300) | DMPLL_LDCR_LD_SAMP_W(2000));

 fprintf(stderr,"X=exit\n");
	dmpll_purge_rfifo();
 fprintf(stderr,"X=exit\n");
	shw_clkb_write_reg(CLKB_BASE_DMPLL + DMPLL_REG_PCR, DMPLL_PCR_ENABLE | DMPLL_PCR_REFSEL_W(0) | DMPLL_PCR_DAC_CLKSEL_W(2) | DMPLL_PCR_PS_SPEED_W(100) | DMPLL_PCR_SWRST);

//term_init();
// signal(SIGINT, sighandler);
 //signal(SIGTERM, sighandler);
 //signal(SIGKILL, sighandler);

 fprintf(stderr,"X=exit\n");
 
dmpll_set_phase_shift(DMPLL_CHANNEL_UP1, 16400);

 for(;;)
 {
/* int c=term_get();
	 if(c=='x') break;
 
	 if(c=='a') { ps_shift += 200; ps_new_shift = 1; }
	 if(c=='z') { ps_shift -= 200; ps_new_shift = 1; }
 
//	uint32_t psr = shw_clkb_read_reg(CLKB_BASE_DMPLL+ DMPLL_REG_PSR);
 
//	fprintf(stderr,"PhLk: %d FrLk: %d\n", psr & DMPLL_PSR_PHASE_LK ? 1 : 0, psr & DMPLL_PSR_FREQ_LK ? 1 : 0);
 
	if(ps_new_shift)// && !dmpll_shifter_busy(DMPLL_CHANNEL_UP1))
	{
		fprintf(stderr,"Setting PS = %d\n", ps_shift);
		ps_new_shift = 0;
		dmpll_set_phase_shift(DMPLL_CHANNEL_UP1, ps_shift);

	}
 */
 
//	 dmpll_poll_rfifo();
 }
//term_restore();


  
  
  
}
