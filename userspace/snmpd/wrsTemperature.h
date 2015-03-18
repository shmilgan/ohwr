#ifndef WRS_WRS_TEMPERATURE_H
#define WRS_WRS_TEMPERATURE_H

struct wrsTemperature_s {
	int temp_fpga;		/* FPGA temperature */
	int temp_pll;		/* PLL temperature */
	int temp_psl;		/* PSL temperature */
	int temp_psr;		/* PSR temperature */
	int temp_fpga_thold;	/* Threshold value for FPGA temperature */
	int temp_pll_thold;	/* Threshold value for PLL temperature */
	int temp_psl_thold;	/* Threshold value for PSL temperature */
	int temp_psr_thold;	/* Threshold value for PSR temperature */
};

extern struct wrsTemperature_s wrsTemperature_s;
int wrsTemperature_data_fill(void);

void init_wrsTemperature(void);
#endif /* WRS_WRS_TEMPERATURE_H */
