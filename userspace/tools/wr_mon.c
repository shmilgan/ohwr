#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <ppsi/ppsi.h>
#include <libwr/shmem.h>
#include <libwr/hal_shmem.h>
#include <libwr/switch_hw.h>
#include <libwr/wrs-msg.h>
#include <libwr/pps_gen.h>
#include <libwr/util.h>
#include <fpga_io.h>
#include <minipc.h>
#include <signal.h>

#include "term.h"

#define PTP_EXPORT_STRUCTURES
#include "ptpd_exports.h"

#define SHOW_GUI			0
#define SHOW_SLAVE_STATS	1
#define SHOW_MASTER_STATS	2
#define SHOW_SERVO_STATS	4
#define SHOW_TEMPERATURES	8
#define SHOW_ALL_STATS		15 /* for convenience with -a option */
#define WEB_INTERFACE       16 /* TJP: still has it's own print function, ugly */

int mode = SHOW_GUI;

static struct minipc_ch *ptp_ch;

static struct wrs_shm_head *hal_head;
static struct hal_port_state *hal_ports;
static struct hal_port_state hal_ports_local_copy[HAL_MAX_PORTS]; /* local copy
							of port state */
static int hal_nports_local;
static struct wrs_shm_head *ppsi_head;
static struct pp_globals *ppg;
static struct wr_servo_state *ppsi_servo;
static struct wr_servo_state ppsi_servo_local; /* local copy of
							servo status */
static pid_t ptp_ch_pid; /* pid of ppsi connected via minipc */
static struct hal_temp_sensors *temp_sensors;
static struct hal_temp_sensors temp_sensors_local; /* local copy of
							temperature sensor readings */

void help(char *prgname)
{
	fprintf(stderr, "%s: Use: \"%s [<options>] <cmd> [<args>]\n",
		prgname, prgname);
	fprintf(stderr,
		"  The program has the following options\n"
		"  -h   print help\n"
		"  -m   show master statistics\n"
		"  -s   show slave statistics\n"
		"  -e   show servo statistics\n"
		"  -t   show temperatures\n"
		"  -a   show all (same as -m -s -e- t options)\n"
		"  -c   use colour output\n"
		"  -w   web interface mode\n"
		"\n"
		"During execution the user can enter 'q' to exit the program\n"
		"and 't' to toggle printing of state information on/off\n");
	exit(1);
}

int read_hal(void){
	unsigned ii;
	unsigned retries = 0;

	/* read data, with the sequential lock to have all data consistent */
	while (1) {
		ii = wrs_shm_seqbegin(hal_head);
		memcpy(hal_ports_local_copy, hal_ports,
			   hal_nports_local*sizeof(struct hal_port_state));
		memcpy(&temp_sensors_local, temp_sensors,
			   sizeof(*temp_sensors));
		retries++;
		if (retries > 100)
			return -1;
		if (!wrs_shm_seqretry(hal_head, ii))
			break; /* consistent read */
		usleep(1000);
	}

	return 0;
}

int read_servo(void){
	unsigned ii;
	unsigned retries = 0;

	/* read data, with the sequential lock to have all data consistent */
	while (1) {
		ii = wrs_shm_seqbegin(ppsi_head);
		memcpy(&ppsi_servo_local, ppsi_servo, sizeof(*ppsi_servo));
		retries++;
		if (retries > 100)
			return -1;
		if (!wrs_shm_seqretry(ppsi_head, ii))
			break; /* consistent read */
		usleep(1000);
	}

	return 0;
}

void ppsi_connect_minipc(void)
{
	if (ptp_ch) {
		/* close minipc, if connected before */
		minipc_close(ptp_ch);
	}
	ptp_ch = minipc_client_create("ptpd", 0);
	if (!ptp_ch) {
		pr_error("Can't establish WRIPC connection to the PTP "
			 "daemon!\n");
		exit(1);
	}
	/* store pid of ppsi connected via minipc */
	ptp_ch_pid = ppsi_head->pid;
}

void init_shm(void)
{
	struct hal_shmem_header *h;
	int ret;
	int n_wait = 0;
	while ((ret = wrs_shm_get_and_check(wrs_shm_hal, &hal_head)) != 0) {
		n_wait++;
		if (ret == 1) {
			pr_error("Unable to open HAL's shm !\n");
		}
		if (ret == 2) {
			pr_error("Unable to read HAL's version!\n");
		}
		if (n_wait > 10) {
			/* timeout! */
			exit(-1);
		}
		sleep(1);
	}

	if (hal_head->version != HAL_SHMEM_VERSION) {
		pr_error("Unknown HAL's shm version %i (known is %i)\n",
			 hal_head->version, HAL_SHMEM_VERSION);
		exit(1);
	}
	h = (void *)hal_head + hal_head->data_off;
	/* Assume number of ports does not change in runtime */
	hal_nports_local = h->nports;
	if (hal_nports_local > HAL_MAX_PORTS) {
		pr_error("Too many ports reported by HAL. %d vs %d "
			 "supported\n", hal_nports_local, HAL_MAX_PORTS);
		exit(1);
	}
	/* Even after HAL restart, HAL will place structures at the same
	 * addresses. No need to re-dereference pointer at each read. */
	hal_ports = wrs_shm_follow(hal_head, h->ports);
	if (!hal_ports) {
		pr_error("Unable to follow hal_ports pointer in HAL's "
			 "shmem\n");
		exit(1);
	}
	temp_sensors = &(h->temp);

	n_wait = 0;
	while ((ret = wrs_shm_get_and_check(wrs_shm_ptp, &ppsi_head)) != 0) {
		n_wait++;
		if (ret == 1) {
			pr_error("Unable to open PPSI's shm !\n");
		}
		if (ret == 2) {
			pr_error("Unable to read PPSI's version!\n");
		}
		if (n_wait > 10) {
			/* timeout! */
			exit(-1);
		}
		sleep(1);
	}

	/* check hal's shm version */
	if (ppsi_head->version != WRS_PPSI_SHMEM_VERSION) {
		pr_error("Unknown PPSI's shm version %i (known is %i)\n",
			 ppsi_head->version, WRS_PPSI_SHMEM_VERSION);
		exit(1);
	}
	ppg = (void *)ppsi_head + ppsi_head->data_off;

	ppsi_servo = wrs_shm_follow(ppsi_head, ppg->global_ext_data);
	if (!ppsi_servo) {
		pr_error("Cannot follow ppsi_servo in shmem.\n");
		exit(1);
	}

	ppsi_connect_minipc();
}

/* TODO: loop over ports first and then decide how to print, maybe even via
		an additional function to avoid endless code duplication? */
void show_ports(void)
{
	int i, j;
	time_t t;
	struct tm *tm;
	char datestr[32];
	struct hal_port_state *port_state;
	struct pp_instance *pp_array;

	pp_array = wrs_shm_follow(ppsi_head, ppg->pp_instances);

	if(mode == SHOW_GUI) {
		time(&t);
		tm = localtime(&t);
		strftime(datestr, sizeof(datestr), "%Y-%m-%d %H:%M:%S", tm);
		term_cprintf(C_BLUE, "Switch time: %s\n", datestr);

		t = (time_t)_fpga_readl(FPGA_BASE_PPS_GEN + 8 /* UTC_LO */);
		tm = localtime(&t);
		strftime(datestr, sizeof(datestr), "%Y-%m-%d %H:%M:%S", tm);
		term_cprintf(C_BLUE, "WR time:	 %s\n", datestr);

		for (i = 0; i < hal_nports_local; i++)
		{
			char if_name[10];

			snprintf(if_name, 10, "wr%d", i);

			port_state = hal_lookup_port(hal_ports_local_copy,
							hal_nports_local, if_name);
			if (!port_state)
				continue;

			term_cprintf(C_WHITE, " %-5s: ", if_name);
			/* check if link is up */
			if (state_up(port_state->state))
				term_cprintf(C_GREEN, "Link up	");
			else
				term_cprintf(C_RED, "Link down  ");

			/*
			 * FIXME: this is from dot-config, but ppsi.conf
			 * can be unrelated, like during tests. So this is
			 * just wrong for us developers (did I write FIXME?)
			 */
			switch (port_state->mode)
			{
				case HEXP_PORT_MODE_WR_MASTER:
					term_cprintf(C_WHITE, "WR Master  ");
					break;
				case HEXP_PORT_MODE_WR_SLAVE:
					term_cprintf(C_WHITE, "WR Slave   ");
					break;
				case HEXP_PORT_MODE_NON_WR:
					term_cprintf(C_WHITE, "Non WR	 ");
					break;
				case HEXP_PORT_MODE_WR_M_AND_S:
					term_cprintf(C_WHITE, "WR auto	");
					break;
				default:
					term_cprintf(C_WHITE, "Unknown	");
					break;
			}

			if (port_state->locked)
				term_cprintf(C_GREEN, "Locked  ");
			else
				term_cprintf(C_RED, "NoLock  ");

			/*
			 * Actually, what is interesting is the PTP state.
			 * For this lookup, the port in ppsi shmem
			 */
			for (j = 0; j < ppg->nlinks; j++) {
				if (!strcmp(if_name,
						pp_array[j].cfg.iface_name))
					break;
			}
			/* Warning: we may have more pp instances per port */
			if (j == ppg->nlinks || !state_up(port_state->state)) {
				term_cprintf(C_RED, "no-ptp\n");
			} else {
				unsigned char *p = pp_array[j].peer;

				term_cprintf(C_WHITE, "peer: %02x:%02x:%02x"
						 ":%02x:%02x:%02x ", p[0], p[1],
						 p[2], p[3], p[4], p[5]);
				term_cprintf(C_GREEN, "ptp state %i\n",
						 pp_array[j].state);
				/* FIXME: string state */
			}
		}
	}
	else if(mode == SHOW_ALL_STATS) {
		printf("PORTS ");
		for (i = 0; i < hal_nports_local; ++i) {
			char if_name[10];

			snprintf(if_name, 10, "wr%d", i);
			port_state = hal_lookup_port(hal_ports_local_copy,
						   hal_nports_local, if_name);
			if (!port_state)
				continue;

			printf("port:%s ", if_name);
			printf("lnk:%d ", state_up(port_state->state));
			switch (port_state->mode) {
			case HEXP_PORT_MODE_WR_MASTER:
				printf("mode:M ");
				break;
			case HEXP_PORT_MODE_WR_SLAVE:
				printf("mode:S ");
				break;
			case HEXP_PORT_MODE_NON_WR:
				printf("mode:N ");
				break;
			case HEXP_PORT_MODE_WR_M_AND_S:
				printf("mode:A ");
				break;
			default:
				printf("mode:U ");
				break;
			}
			printf("lock:%d ", port_state->locked);
		}
		printf("\n");
	}
	else if(mode == SHOW_SLAVE_STATS) {
		printf("PORTS ");
		for (i = 0; i < hal_nports_local; ++i) {
			char if_name[10];

			snprintf(if_name, 10, "wr%d", i);
			port_state = hal_lookup_port(hal_ports_local_copy,
						   hal_nports_local, if_name);
			if (!port_state)
				continue;

			switch (port_state->mode) {
			case HEXP_PORT_MODE_WR_SLAVE:
				printf("port:%s ", if_name);
				printf("lnk:%d ", state_up(port_state->state));
				printf("mode:S ");
				printf("lock:%d ", port_state->locked);
				break;
			case HEXP_PORT_MODE_WR_M_AND_S:
				snprintf(if_name, 10, "wr%d", i);
				printf("port:%s ", if_name);
				printf("lnk:%d ", state_up(port_state->state));
				printf("mode:A ");
				printf("lock:%d ", port_state->locked);
				break;
			default:
				break;
			}
		}
		printf("\n");
	}
	else if(mode == SHOW_MASTER_STATS) {
		printf("PORTS ");
		for (i = 0; i < hal_nports_local; ++i) {
			char if_name[10];

			snprintf(if_name, 10, "wr%d", i);
			port_state = hal_lookup_port(hal_ports_local_copy,
						   hal_nports_local, if_name);
			if (!port_state)
				continue;

			switch (port_state->mode) {
			case HEXP_PORT_MODE_WR_MASTER:
				printf("port:%s ", if_name);
				printf("lnk:%d ", state_up(port_state->state));
				printf("mode:M ");
				printf("lock:%d ", port_state->locked);
				break;
			case HEXP_PORT_MODE_WR_M_AND_S:
				snprintf(if_name, 10, "wr%d", i);
				printf("port:%s ", if_name);
				printf("lnk:%d ", state_up(port_state->state));
				printf("mode:A ");
				printf("lock:%d ", port_state->locked);
				break;
			default:
				break;
			}
		}
		printf("\n");
	}
}

/*
 * This is almost a copy of the above, used by web interface.
 * Code duplication is bad, but this is better than a separate tool
 * which is almost identical but even broken
 */
/* TODO: integrate in the grand print logic */
static void show_unadorned_ports(void)
{
	int i;
	struct hal_port_state *port_state;

	for (i = 0; i < hal_nports_local; i++)
	{
		char if_name[10];

		snprintf(if_name, 10, "wr%d", i);
			port_state = hal_lookup_port(hal_ports_local_copy,
							 hal_nports_local, if_name);
			if (!port_state)
				continue;

		printf("%s %s %s %s\n",
			   state_up(port_state->state)
			   ? "up" : "down",
			   port_state->mode == HEXP_PORT_MODE_WR_MASTER
			   ? "Master" : "Slave", /* FIXME: other options? */
			   port_state->locked
			   ? "Locked" : "NoLock",
			   port_state->calib.rx_calibrated
			   && port_state->calib.tx_calibrated
			   ? "Calibrated" : "Uncalibrated");
	}
}


void show_servo(void)
{
	int64_t total_asymmetry;
	int64_t crtt;
	static time_t lastt;
	static int last_count;

	total_asymmetry = ppsi_servo_local.picos_mu -
			  2LL * ppsi_servo_local.delta_ms;
	crtt = ppsi_servo_local.picos_mu - ppsi_servo_local.delta_tx_m -
		   ppsi_servo_local.delta_rx_m - ppsi_servo_local.delta_tx_s -
		   ppsi_servo_local.delta_rx_s;

	if(mode == SHOW_GUI) {
		term_cprintf(C_BLUE, "\nSynchronization status:\n");

		if (!(ppsi_servo_local.flags & WR_FLAG_VALID)) {
			term_cprintf(C_RED, "Master mode or sync info not valid\n");
			return;
		}

		term_cprintf(C_GREY, "Servo state:			   ");
		if (lastt && time(NULL) - lastt > 5) {
			term_cprintf(C_RED, " --- not updating --- ");
		} else {
			term_cprintf(C_WHITE, "%s: %s%s\n",
					 ppsi_servo_local.if_name,
					 ppsi_servo_local.servo_state_name,
					 ppsi_servo_local.flags & WR_FLAG_WAIT_HW ?
					 " (wait for hw)" : "");
		}

		/* "tracking disabled" is just a testing tool */
		if (!ppsi_servo_local.tracking_enabled)
			term_cprintf(C_RED, "Tracking forcibly disabled\n");

		term_cprintf(C_BLUE, "\nTiming parameters:\n\n");

		term_cprintf(C_GREY, "Round-trip time (mu):	  ");
		term_cprintf(C_WHITE, "%.3f nsec\n",
				 ppsi_servo_local.picos_mu/1000.0);

		term_cprintf(C_GREY, "Master-slave delay:		");
		term_cprintf(C_WHITE, "%.3f nsec\n",
				 ppsi_servo_local.delta_ms/1000.0);

		term_cprintf(C_GREY, "Master PHY delays:		 ");
		term_cprintf(C_WHITE, "TX: %.3f nsec, RX: %.3f nsec\n",
				 ppsi_servo_local.delta_tx_m/1000.0,
				 ppsi_servo_local.delta_rx_m/1000.0);

		term_cprintf(C_GREY, "Slave PHY delays:		  ");
		term_cprintf(C_WHITE, "TX: %.3f nsec, RX: %.3f nsec\n",
				 ppsi_servo_local.delta_tx_s/1000.0,
				 ppsi_servo_local.delta_rx_s/1000.0);

		term_cprintf(C_GREY, "Total link asymmetry:	  ");
		term_cprintf(C_WHITE, "%.3f nsec\n", total_asymmetry/1000.0);

		/*if (0) {
			term_cprintf(C_GREY, "Fiber asymmetry:		   ");
			term_cprintf(C_WHITE, "%.3f nsec\n", ss.fiber_asymmetry/1000.0);
		}*/

		term_cprintf(C_GREY, "Clock offset:			  ");
		term_cprintf(C_WHITE, "%.3f nsec\n",
				 ppsi_servo_local.offset/1000.0);

		term_cprintf(C_GREY, "Phase setpoint:			");
		term_cprintf(C_WHITE, "%.3f nsec\n",
				 ppsi_servo_local.cur_setpoint/1000.0);

		term_cprintf(C_GREY, "Skew:					  ");
		term_cprintf(C_WHITE, "%.3f nsec\n",
				 ppsi_servo_local.skew/1000.0);

		term_cprintf(C_GREY, "Servo update counter:	  ");
		term_cprintf(C_WHITE, "%u times\n",
				 ppsi_servo_local.update_count);
		if (ppsi_servo_local.update_count != last_count) {
			lastt = time(NULL);
			last_count = ppsi_servo_local.update_count;
		}
	}
	else if(mode == SHOW_ALL_STATS) {
		printf("SERVO	");
		printf("sv:%d ", ppsi_servo_local.flags & WR_FLAG_VALID ? 1 : 0);
		printf("ss:'%s' ", ppsi_servo_local.servo_state_name);
		printf("mu:%llu ", ppsi_servo_local.picos_mu);
		printf("dms:%llu ", ppsi_servo_local.delta_ms);
		printf("dtxm:%d drxm:%d ", ppsi_servo_local.delta_tx_m,
					   ppsi_servo_local.delta_rx_m);
		printf("dtxs:%d drxs:%d ", ppsi_servo_local.delta_tx_s,
					   ppsi_servo_local.delta_rx_s);
		printf("asym:%lld ", total_asymmetry);
		printf("crtt:%llu ", crtt);
		printf("cko:%lld ", ppsi_servo_local.offset);
		printf("setp:%d ", ppsi_servo_local.cur_setpoint);
		printf("ucnt:%u ", ppsi_servo_local.update_count);
		printf("\n");
	}
	else if(mode == SHOW_SLAVE_STATS) {
		printf("SERVO	");
		printf("sv:%d ", ppsi_servo_local.flags & WR_FLAG_VALID ? 1 : 0);
		printf("ss:'%s' ", ppsi_servo_local.servo_state_name);
		printf("mu:%llu ", ppsi_servo_local.picos_mu);
		printf("dms:%llu ", ppsi_servo_local.delta_ms);
		printf("dtxm:%d drxm:%d ", ppsi_servo_local.delta_tx_m,
					   ppsi_servo_local.delta_rx_m);
		printf("dtxs:%d drxs:%d ", ppsi_servo_local.delta_tx_s,
					   ppsi_servo_local.delta_rx_s);
		printf("asym:%lld ", total_asymmetry);
		printf("crtt:%llu ", crtt);
		printf("cko:%lld ", ppsi_servo_local.offset);
		printf("setp:%d ", ppsi_servo_local.cur_setpoint);
		printf("ucnt:%u ", ppsi_servo_local.update_count);
		printf("\n");
	}
}

void show_temperatures(void)
{
	if (mode == SHOW_GUI) {
		term_cprintf(C_BLUE, "\nTemperatures:\n");

		term_cprintf(C_GREY, "FPGA: ");
		term_cprintf(C_WHITE, "%2.2f ",
				 temp_sensors_local.fpga/256.0);
		term_cprintf(C_GREY, "PLL: ");
		term_cprintf(C_WHITE, "%2.2f ",
				 temp_sensors_local.pll/256.0);
		term_cprintf(C_GREY, "PSL: ");
		term_cprintf(C_WHITE, "%2.2f ",
				 temp_sensors_local.psl/256.0);
		term_cprintf(C_GREY, "PSR: ");
		term_cprintf(C_WHITE, "%2.2f\n",
				 temp_sensors_local.psr/256.0);
	}
}

int track_onoff = 1;

/* TJP: check if this needs modification */
void show_all(void)
{
	int hal_alive;
	int ppsi_alive;

	if (mode == SHOW_GUI) {
		term_clear();
		term_pcprintf(1, 1, C_BLUE,
				  "WR Switch Sync Monitor %s[q = quit]\n\n",
				  __GIT_VER__);
	}

	hal_alive = (hal_head->pid && (kill(hal_head->pid, 0) == 0));
	ppsi_alive = (ppsi_head->pid && (kill(ppsi_head->pid, 0) == 0));

	if (hal_alive)
		show_ports();
	else if (mode == SHOW_GUI)
		term_cprintf(C_RED, "\nHAL is dead!\n");
	else if (mode == SHOW_ALL_STATS)
		printf("HAL is dead!\n");

	if (ppsi_alive)
		show_servo();
	else if (mode == SHOW_GUI)
		term_cprintf(C_RED, "\nPPSI is dead!\n");
	else if (mode == SHOW_ALL_STATS)
		printf("PPSI is dead!\n");


	if (hal_alive)
		show_temperatures();
	fflush(stdout);
}

int main(int argc, char *argv[])
{
	int opt;
	int usecolor = 0;
	uint32_t last_count = 0;

	/* try a pps_gen based approach */
	uint64_t seconds=0;
	uint64_t last_seconds=0;	
	uint32_t nanoseconds=0;
	uint32_t last_nanoseconds=0;

	wrs_msg_init(argc, argv);
	
	while((opt=getopt(argc, argv, "hmsetacwqv")) != -1)
	{
		switch(opt)
		{
			case 'h':
				help(argv[0]);
			case 's':
				mode |= SHOW_SLAVE_STATS;
				break;
			case 'm':
				mode |= SHOW_MASTER_STATS;
				break;
			case 'e':
				mode |= SHOW_SERVO_STATS;
				break;
			case 't':
				mode |= SHOW_TEMPERATURES;
				break;
			case 'a':
				mode |= SHOW_ALL_STATS;
				break;
			case 'c':
				usecolor = 1;
				break;
			case 'w':
				mode |= WEB_INTERFACE;
				break;
			case 'q': break; /* done in wrs_msg_init() */
			case 'v': break; /* done in wrs_msg_init() */
			default:
				help(argv[0]);
		}
	}

	init_shm();

	if (mode & WEB_INTERFACE) {
		read_hal();
		show_unadorned_ports();
		exit(0);
	}

	if (shw_fpga_mmap_init() < 0) {
		pr_error("Can't initialize FPGA mmap\n");
		exit(1);
	}
	term_init(usecolor);
	setvbuf(stdout, NULL, _IOFBF, 4096);

	/* main loop */
	for(;;)
	{
		if(term_poll(10))
		{
			int c = term_get();

			if(c=='q')
				break;

			if(c=='t') {
				int rval;
				track_onoff = 1-track_onoff;
				if (ptp_ch_pid != ppsi_head->pid) {
					/* ppsi was restarted since minipc
					 * connection, reconnect now */
					ppsi_connect_minipc();
				}
				minipc_call(ptp_ch, 200, &__rpcdef_cmd,
						&rval, PTPDEXP_COMMAND_TRACKING,
						track_onoff);
			}
		}

		shw_pps_gen_read_time(&seconds, &nanoseconds);
		/* printf("sec: %s, nsec: %d\n", format_time(seconds), nanoseconds); */
		if (seconds != last_seconds) {
			last_seconds=seconds;
			last_nanoseconds=nanoseconds;
			usleep(1000);
			
			read_servo();
			read_hal();
			show_all();
		}
		
		/* If we got broken pipe or anything, exit */
		if (ferror(stdout))
			exit(1);
	}

	term_restore();
	setlinebuf(stdout);
	printf("\n");
	return 0;
}
