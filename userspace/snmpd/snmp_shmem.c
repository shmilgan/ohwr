#include "wrsSnmp.h"
#include "snmp_shmem.h"

/* HAL */
struct wrs_shm_head *hal_head;
struct hal_shmem_header *hal_shmem;
struct hal_port_state *hal_ports;
int hal_nports_local;

/* PPSI */
struct wrs_shm_head *ppsi_head;
static struct pp_globals *ppg;
struct wr_servo_state *ppsi_servo;
struct pp_instance *ppsi_ppi;
int *ppsi_ppi_nlinks;

/* RTUd */
struct wrs_shm_head *rtud_head;



static void init_shm_hal(void)
{
	hal_head = wrs_shm_get(wrs_shm_hal, "", WRS_SHM_READ);
	if (!hal_head) {
		snmp_log(LOG_ERR, "unable to open shm for HAL!\n");
		exit(-1);
	}
	wrs_shm_wait(hal_head, 500 /* ms */, 20, NULL);

	/* check hal's shm version */
	if (hal_head->version != HAL_SHMEM_VERSION) {
		snmp_log(LOG_ERR, "unknown hal's shm version %i "
			 "(known is %i)\n", hal_head->version,
			 HAL_SHMEM_VERSION);
		exit(-1);
	}

	hal_shmem = (void *)hal_head + hal_head->data_off;
	/* Assume number of ports does not change in runtime */
	hal_nports_local = hal_shmem->nports;
	if (hal_nports_local > WRS_N_PORTS) {
		snmp_log(LOG_ERR, "Too many ports reported by HAL. "
			"%d vs %d supported\n",
			hal_nports_local, WRS_N_PORTS);
		exit(-1);
	}
	/* Even after HAL restart, HAL will place structures at the same
	 * addresses. No need to re-dereference pointer at each read. */
	hal_ports = wrs_shm_follow(hal_head, hal_shmem->ports);
	if (!hal_ports) {
		snmp_log(LOG_ERR, "Unalbe to follow hal_ports pointer in HAL's"
			 " shmem");
		exit(-1);
	}
}

static void init_shm_ppsi(void)
{
	ppsi_head = wrs_shm_get(wrs_shm_ptp, "", WRS_SHM_READ);
	if (!ppsi_head) {
		snmp_log(LOG_ERR, "unable to open shm for PPSI!\n");
		exit(-1);
	}

	/* check ppsi's shm version */
	if (ppsi_head->version != WRS_PPSI_SHMEM_VERSION) {
 		snmp_log(LOG_ERR, "unknown PPSI's shm version %i "
			"(known is %i)\n",
			ppsi_head->version, WRS_PPSI_SHMEM_VERSION);
		exit(-1);
	}
	ppg = (void *)ppsi_head + ppsi_head->data_off;

	ppsi_servo = wrs_shm_follow(ppsi_head, ppg->global_ext_data);
	if (!ppsi_servo) {
		snmp_log(LOG_ERR, "Cannot follow ppsi_servo in shmem.\n");
		exit(-1);
	}

	ppsi_ppi = wrs_shm_follow(ppsi_head, ppg->pp_instances);
	if (!ppsi_ppi) {
		snmp_log(LOG_ERR, "Cannot follow ppsi_ppi in shmem.\n");
		exit(-1);
	}
	/* use pointer instead of copying */
	ppsi_ppi_nlinks = &(ppg->nlinks);
}

static void init_shm_rtud(void)
{
	/* open RTUd's shm */
	rtud_head = wrs_shm_get(wrs_shm_rtu, "", WRS_SHM_READ);
	if (!rtud_head) {
		snmp_log(LOG_ERR, "unable to open shm for RTUd!\n");
		exit(-1);
	}

	/* check rtud's shm version */
	if (rtud_head->version != RTU_SHMEM_VERSION) {
		snmp_log(LOG_ERR, "unknown RTUd's shm version %i "
			 "(known is %i)\n", rtud_head->version,
			 RTU_SHMEM_VERSION);
		exit(-1);
	}
}

void init_shm(void){
	init_shm_hal();
	init_shm_ppsi();
	init_shm_rtud();
}
