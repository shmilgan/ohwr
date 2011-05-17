#include <stdio.h>
#include <stdarg.h>

#include <hw/switch_hw.h>
#include <hw/clkb_io.h>
//#include <hw/fpga_io.h>

#include <sys/time.h>

#define N_PHYS 10

#define CLOCK_TOLLERANCE 2

struct {
    uint32_t base;
    char * name;
    int phy_loopback;
    int status_err;
    int status_clk;
    uint32_t errors, freq;
} ports[] = {
	{FPGA_BASE_GIGASPY_UP0, "[up0] uplink0", 0},
	{FPGA_BASE_GIGASPY_UP1, "[up1] uplink1", 0},
	{FPGA_BASE_GIGASPY_DP0, "[dp0] utca-1", 1},
	{FPGA_BASE_GIGASPY_DP1, "[dp1] utca-3", 1},
	{FPGA_BASE_GIGASPY_DP2, "[dp2] utca-2", 1},
	{FPGA_BASE_GIGASPY_DP3, "[dp3] utca-5", 1},
	{FPGA_BASE_GIGASPY_DP4, "[dp4] utca-4", 1},
	{FPGA_BASE_GIGASPY_DP5, "[dp5] utca-7", 1},
	{FPGA_BASE_GIGASPY_DP6, "[dp6] utca-6", 1},
	{FPGA_BASE_GIGASPY_DP7, "[dp7] utca-8", 1},
	0
};

#define MEAS_STEPS 20
#define DAC_STEP 10000
#define CLK_REF 0
#define CLK_DMTD 1

#define NUM_CLKB_CLOCKS 4
#define MEAS_DELAY 2
#define N_CLOCKS 4

struct clk_meas {
  uint32_t reg;
  char *name;
  int tunable;
  int nominal_freq;
  float tuning_range;
  uint32_t dac;
  int dac_val;
  int dac_dval;
  int test_steps;


  int log_size;
  int test_busy;
  int dac_ok;
  int clock_ok;
  int cur_step;
  uint32_t prev_step_t;
  int have_first_meas;

  int vt_log[100];
  int freq_log[100];
  int dac_val_prev;
  float measured_range;
  uint32_t cur_freq;
  

} clocks [] = {
 {CLKB_REG_REF_FREQ, "refclk", 1, 125000000, 10e-6, DAC_REF, 0, DAC_STEP, MEAS_STEPS, 0, 1, 0, 0, 0, 0, 0},
 {CLKB_REG_DMTD_FREQ, "dmtdclk", 1, 125000000, 100e-6, DAC_DMTD, 65535, -DAC_STEP, MEAS_STEPS, 0, 1, 0, 0, 0, 0, 0},
 {CLKB_REG_UP0_FREQ, "up0-clkb", 0, 125000000},
 {CLKB_REG_UP1_FREQ, "up1-clkb", 0, 125000000}
};

uint32_t start_tics = 0xffffffff;

uint32_t get_tics()
{
    struct timezone tz = {0,0};
    struct timeval tv;
    gettimeofday(&tv, &tz);

    return tv.tv_sec;
}

FILE *clk_log;

void check_dac_vcxo(struct clk_meas *c)
{
    int i;
    int dv, df;
    int fail =0;
    
    double minvt=1000000, maxvt=-1000000;
    double minf=200e9, maxf=0;
    
    for(i=0;i<c->cur_step;i++)
    {
	if(c->freq_log[i] < minf) minf = (double)c->freq_log[i];
	if(c->freq_log[i] > maxf) maxf = (double)c->freq_log[i];

	if(c->vt_log[i] < minvt) minvt = (double)c->vt_log[i];
	if(c->vt_log[i] > maxvt) maxvt = (double)c->vt_log[i];
	
    }

    c->measured_range =  0.5 * ( maxf - minf ) * (65536.0 / (maxvt-minvt)) / (double) c->nominal_freq;

    fprintf(clk_log, "Clock summary:            %s\n", c->name);    
    fprintf(clk_log, "MeasuredRange: (+/-) %.1f ppm\n", c->measured_range * 1000000.0);
    
    
    
    for(i=0;i<c->cur_step;i++)
    {
    
	fprintf(clk_log, "vt: %d freq: %d\n", c->vt_log[i], c->freq_log[i]);
	
	if(i>0){
        	dv = (c->vt_log[i-1] - c->vt_log[i]);
		df = (c->freq_log[i-1] - c->freq_log[i]);
		if(dv * df <= 0) { fail = 1; } // non-monotonous?
	}
    }
    
    fflush(clk_log);
    
    c->dac_ok = !fail && (c->measured_range >= c->tuning_range);
}

int meas_remaining_steps;

void check_clock(struct clk_meas *c)
{
	if(!c->test_busy || !c->tunable)
	{
	    if(c->tunable) shw_clkb_dac_write(c->dac, 32000);
	    c->cur_freq =  shw_clkb_read_reg(c->reg);
	    c->clock_ok = ( c->cur_freq >  c->nominal_freq - 100000 ) && ( c->cur_freq <  c->nominal_freq + 100000 );
	
	} if(c->test_busy && c->prev_step_t + MEAS_DELAY < get_tics())
	{
	    c->prev_step_t = get_tics();
	    uint32_t freq = shw_clkb_read_reg(c->reg);

	    c->cur_freq = freq;

	    shw_clkb_dac_write(c->dac, c->dac_val);

	    meas_remaining_steps = c->test_steps - c->cur_step;
	    
	    if(c->have_first_meas)
	    { 
//	    printf("%s: dac = %d, freq = %d\n", c->name, c->dac_val_prev, freq);
	    c->freq_log[c->cur_step] = freq;
	    c->vt_log[c->cur_step] = c->dac_val_prev;

	    c->cur_step++;
    	    if(c->cur_step == MEAS_STEPS)
		{
		    check_dac_vcxo(c);
		    meas_remaining_steps = 0;
    		    c->test_busy = 0;
		}
	    }
	    
	 c->have_first_meas = 1;


	    c->dac_val_prev = c->dac_val;
	    c->dac_val += c->dac_dval;

	    if(c->dac_val <= 0)
	    {
		c->dac_val = 0;
		c->dac_dval = -c->dac_dval;
	    } else if(c->dac_val >= 65535)
	    {
		c->dac_val = 65535;
		c->dac_dval = -c->dac_dval;
	    }
	    
	}
}


void update_clkb_clocks()
{
    int i;
    for(i=0;i<NUM_CLKB_CLOCKS;i++)
    {
	struct clk_meas *c = &clocks[i];
        check_clock(c);
    }
}

void cprintf(int color, const char *fmt, ...)
{
    va_list ap;
    printf("\033[01;3%dm",color);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    fflush(stdout);
}

void clear_screen()
{
    printf("\033[2J\033[1;1H");
}

#define WHITE 7
#define RED 1
#define GREEN 2
#define BLUE 4

main()
{
  int i=0,di=255,previ=0;

	trace_log_stderr();
	shw_init();

	shw_ad9516_set_output_delay(9, 3, 0, 20, 0);

	xpoint_configure();

	for(i=0;i<N_PHYS;i++) 
	{
	    uint32_t flag = GSPY_GSTESTCTL_ENABLE | GSPY_GSTESTCTL_CONNECT | GSPY_GSTESTCTL_PHYIO_ENABLE | GSPY_GSTESTCTL_PHYIO_SYNCEN;
	    if(ports[i].phy_loopback) flag |= GSPY_GSTESTCTL_PHYIO_LOOPEN;
    	    _fpga_writel(ports[i].base+GSPY_REG_GSTESTCTL, flag) ;
    	    usleep(10000);
	    uint32_t ctl = _fpga_readl(ports[i].base+GSPY_REG_GSTESTCTL) ; ctl |= GSPY_GSTESTCTL_RST_CNTR; _fpga_writel(ports[i].base+GSPY_REG_GSTESTCTL, ctl);
	}

	term_init();
	clk_log=fopen("clk_meas.log","wb");
	for(;;)
	{
		char c =term_get();
		if(c=='q') break;
		if(c=='x') 
		{
		    clocks[CLK_REF].test_busy=0;
		    clocks[CLK_DMTD].test_busy=0;
		    meas_remaining_steps =0;
		
		}
		usleep(100000);

		clear_screen();

		cprintf(BLUE, "PHY/Clocking test program\n----------------------------------\n\n");

		update_clkb_clocks();

		if(meas_remaining_steps > 0)
		{
		    cprintf(WHITE, "Checking DACs and VCXOs: %d measurements remaining (x = skip).", meas_remaining_steps);
		    continue;
		}
		
		if(clocks[CLK_REF].test_busy || clocks[CLK_DMTD].test_busy) continue;


		cprintf(BLUE, "PHY test results:\n\n");

		for(i=0;i<N_PHYS;i++)
		{
			
			ports[i].freq = _fpga_readl(ports[i].base+GSPY_REG_GSCLKFREQ) ;
			ports[i].errors = _fpga_readl(ports[i].base + GSPY_REG_GSTESTCNT);

			ports[i].status_err = ports[i].errors > 0;
			ports[i].status_clk = abs(ports[i].freq - clocks[CLK_REF].cur_freq) > CLOCK_TOLLERANCE;
			
			
			
			cprintf(WHITE, "%-20s: ECNT = %d, RBCLK = %.6f [MHz] status: ", ports[i].name, ports[i].errors, (double)ports[i].freq/1000000.0);
			
			if(ports[i].status_err) 
			    cprintf(RED, "DATA ERRORS ");

			if(ports[i].status_clk) 
			    cprintf(RED, "FAULTY RBCLK ");
			
			if(!ports[i].status_clk  && !ports[i].status_err)
			    cprintf(GREEN, "OK ");
			
			printf("\n");
		}
		
		cprintf(BLUE, "\nClocking system test results:\n\n");

		for(i=0;i<N_CLOCKS;i++)
		{
		    struct clk_meas *c= &clocks[i];
		    if(c->tunable)
		    {
			cprintf(WHITE, "%-10s: ", c->name); 
			cprintf(WHITE, "freq: %.6f [MHz] (", (double)c->cur_freq/1000000.0); 
			if(c->clock_ok) cprintf(GREEN, "OK") ; else cprintf(RED, "FAIL");

			cprintf(WHITE, ") DAC/VCXO tuning: "); 
			if(c->dac_ok) cprintf(GREEN, "OK") ; else cprintf(RED, "FAIL");
			cprintf(WHITE, ", range: \xc2\xb1%.1f ppm\n", c->measured_range * 1000000.0);
			
		    } else {
		    	cprintf(WHITE, "%-10s: ", c->name); 
			cprintf(WHITE, "freq: %.6f [MHz] (", (double)c->cur_freq/1000000.0); 
			if(c->clock_ok) cprintf(GREEN, "OK") ; else cprintf(RED, "FAIL");
			cprintf(WHITE,")\n");
					    
		    }
		    
		    
	
		}
		
	
				
		cprintf(BLUE, "\n\n------------------------------\nq = quit");
		fflush(stdout);
		
	}
	fclose(clk_log);
	printf("\033[00;37m");
	printf("\n\n");
	fflush(stdout);	
	term_restore();


}
