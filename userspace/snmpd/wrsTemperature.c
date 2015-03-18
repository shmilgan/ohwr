#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/auto_nlist.h>

/* Crap! -- everybody makes them different, and even ppsi::ieee wants them */
#undef FALSE
#undef TRUE

/* conflict between definition in net-snmp-agent-includes.h (which include
 * snmp_vars.h) and ppsi.h where INST is defined as a inline function */
#undef INST
#include <ppsi/ieee1588_types.h> /* for ClockIdentity */
#include <libwr/shmem.h>
#include <ppsi/ppsi.h>
#include <libwr/hal_shmem.h>
#include <stdio.h>

#include "wrsSnmp.h"

extern struct wrs_shm_head *hal_head;
extern struct hal_shmem_header *hal_shmem;
extern struct hal_port_state *hal_ports;
extern int hal_nports_local;

/* Our data: globals */
static struct wrsTemperature_s {
	int temp_fpga;		/* FPGA temperature */
	int temp_pll;		/* PLL temperature */
	int temp_psl;		/* PSL temperature */
	int temp_psr;		/* PSR temperature */
	int temp_fpga_thold;	/* Threshold value for FPGA temperature */
	int temp_pll_thold;	/* Threshold value for PLL temperature */
	int temp_psl_thold;	/* Threshold value for PSL temperature */
	int temp_psr_thold;	/* Threshold value for PSR temperature */
} wrsTemperature_s;

static struct pickinfo wrsTemperature_pickinfo[] = {
	FIELD(wrsTemperature_s, ASN_INTEGER, temp_fpga),
	FIELD(wrsTemperature_s, ASN_INTEGER, temp_pll),
	FIELD(wrsTemperature_s, ASN_INTEGER, temp_psl),
	FIELD(wrsTemperature_s, ASN_INTEGER, temp_psr),
	FIELD(wrsTemperature_s, ASN_INTEGER, temp_fpga_thold),
	FIELD(wrsTemperature_s, ASN_INTEGER, temp_pll_thold),
	FIELD(wrsTemperature_s, ASN_INTEGER, temp_psl_thold),
	FIELD(wrsTemperature_s, ASN_INTEGER, temp_psr_thold),
};


int wrsTemperature_data_fill(void)
{
	unsigned ii;
	unsigned retries = 0;
	static time_t t0, t1;

	t1 = time(NULL);
	if (t0 && t1 - t0 < 5) {/* TODO: timeout constatnt */
		/* cache not updated */
		return 1;
	}
	t0 = t1;

	memset(&wrsTemperature_s, 0, sizeof(wrsTemperature_s));
	while (1) {
		ii = wrs_shm_seqbegin(hal_head);

		wrsTemperature_s.temp_fpga = hal_shmem->temp.fpga >> 8;
		wrsTemperature_s.temp_pll = hal_shmem->temp.pll >> 8;
		wrsTemperature_s.temp_psl = hal_shmem->temp.psl >> 8;
		wrsTemperature_s.temp_psr = hal_shmem->temp.psr >> 8;
		wrsTemperature_s.temp_fpga_thold = hal_shmem->temp.fpga_thold;
		wrsTemperature_s.temp_pll_thold = hal_shmem->temp.pll_thold;
		wrsTemperature_s.temp_psl_thold = hal_shmem->temp.psl_thold;
		wrsTemperature_s.temp_psr_thold = hal_shmem->temp.psr_thold;

		retries++;
		if (retries > 100) {
			snmp_log(LOG_ERR, "%s: too many retries to read HAL\n",
				 __func__);
			retries = 0;
			}
		if (!wrs_shm_seqretry(hal_head, ii))
			break; /* consistent read */
		usleep(1000);
	}
	/* there was an update, return 0 */
	return 0;
}

#define GT_OID WRS_OID, 6, 2
#define GT_PICKINFO wrsTemperature_pickinfo
#define GT_DATA_FILL_FUNC wrsTemperature_data_fill
#define GT_DATA_STRUCT wrsTemperature_s
#define GT_GROUP_NAME "wrsTemperature"
#define GT_INIT_FUNC init_wrsTemperature

#include "wrsGroupTemplate.h"
