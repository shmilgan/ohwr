#include "wrsSnmp.h"


struct wrs_shm_head *hal_head;
struct hal_shmem_header *hal_shmem;
struct hal_port_state *hal_ports;
int hal_nports_local;

struct wrs_shm_head *ppsi_head;
static struct pp_globals *ppg;
struct wr_servo_state_t *ppsi_servo;



void init_shm(void)
{
	hal_head = wrs_shm_get(wrs_shm_hal, "", WRS_SHM_READ);
	if (!hal_head) {
		snmp_log(LOG_ERR, "unable to open shm for HAL!\n");
		exit(-1);
	}
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

	ppsi_head = wrs_shm_get(wrs_shm_ptp, "", WRS_SHM_READ);
	if (!ppsi_head) {
		snmp_log(LOG_ERR, "unable to open shm for PPSI!\n");
		exit(-1);
	}

	/* check hal's shm version */
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

}
