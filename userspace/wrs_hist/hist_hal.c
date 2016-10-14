/* wrs_hist file with the interfaces to the HAL's shmem */

#include <stdlib.h>
#include <unistd.h>

#include <libwr/wrs-msg.h>
#include <libwr/shmem.h>
#include <libwr/hal_shmem.h>

struct hist_shmem_data *hist_shmem;
struct wrs_shm_head *hist_shmem_hdr;
static struct wrs_shm_head *hal_shmem_hdr;
static struct hal_temp_sensors *temp_sensors;
static struct hal_port_state *hal_ports;
static struct hal_port_state hal_ports_local_copy[HAL_MAX_PORTS];

void hal_shm_init(void)
{
	
	int ret;
	int n_wait = 0;
	struct hal_shmem_header *h;
	
	while ((ret = wrs_shm_get_and_check(wrs_shm_hal, &hal_shmem_hdr)) != 0) {
		n_wait++;
		if (ret == WRS_SHM_OPEN_FAILED) {
			pr_error("Unable to open HAL's shm !\n");
		}
		if (ret == WRS_SHM_WRONG_VERSION) {
			pr_error("Unable to read HAL's version!\n");
		}
		if (ret == WRS_SHM_INCONSISTENT_DATA) {
			pr_error("Unable to read consistent data from HAL's "
				 "shmem!\n");
		}
		if (n_wait > 10) {
			/* timeout! */
			exit(-1);
		}
		sleep(1);
	}

	if (hal_shmem_hdr->version != HAL_SHMEM_VERSION) {
		pr_error("Unknown HAL's shm version %i (known is %i)\n",
			 hal_shmem_hdr->version, HAL_SHMEM_VERSION);
		exit(1);
	}
	h = (void *)hal_shmem_hdr + hal_shmem_hdr->data_off;
	temp_sensors = &(h->temp);
	hal_ports = wrs_shm_follow(hal_shmem_hdr, h->ports);
	if (!hal_ports) {
		pr_error("Unable to follow hal_ports pointer in HAL's "
			 "shmem\n");
		exit(1);
	}
}


int hal_shmem_read_temp(struct hal_temp_sensors * temp){
	unsigned ii;
	unsigned retries = 0;

	/* read data, with the sequential lock to have all data consistent */
	while (1) {
		ii = wrs_shm_seqbegin(hal_shmem_hdr);
		memcpy(temp, temp_sensors,
		       sizeof(*temp_sensors));
		retries++;
		if (retries > 100)
			return -1;
		if (!wrs_shm_seqretry(hal_shmem_hdr, ii))
			break; /* consistent read */
		usleep(1000);
	}

	return 0;
}

struct hal_port_state * hal_shmem_read_ports(void){
	unsigned ii;
	unsigned retries = 0;

	/* read data, with the sequential lock to have all data consistent */
	while (1) {
		ii = wrs_shm_seqbegin(hal_shmem_hdr);
		memcpy(hal_ports_local_copy, hal_ports,
		       HAL_MAX_PORTS * sizeof(struct hal_port_state));
		retries++;
		if (retries > 100)
			return NULL;
		if (!wrs_shm_seqretry(hal_shmem_hdr, ii))
			break; /* consistent read */
		usleep(1000);
	}

	return hal_ports_local_copy;
}
