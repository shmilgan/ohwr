#ifndef WRS_TEMPERATURE_GROUP_H
#define WRS_TEMPERATURE_GROUP_H

#define WRSTEMPERATURE_CACHE_TIMEOUT 5
#define WRSTEMPERATURE_OID WRS_OID, 7, 1, 3

struct wrsTemperature_s {
	int wrsTempFPGA;		/* FPGA temperature */
	int wrsTempPLL;			/* PLL temperature */
	int wrsTempPSL;			/* PSL temperature */
	int wrsTempPSR;			/* PSR temperature */
	int wrsTempThresholdFPGA;	/* Threshold value for FPGA temperature */
	int wrsTempThresholdPLL;	/* Threshold value for PLL temperature */
	int wrsTempThresholdPSL;	/* Threshold value for PSL temperature */
	int wrsTempThresholdPSR;	/* Threshold value for PSR temperature */
};

extern struct wrsTemperature_s wrsTemperature_s;
time_t wrsTemperature_data_fill(void);

void init_wrsTemperatureGroup(void);
#endif /* WRS_TEMPERATURE_GROUP_H */
