#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <minipc.h>

#include "term.h"

#define PTP_EXPORT_STRUCTURES
#include "ptpd_exports.h"

#include <libwr/hal_client.h>

#define SHOW_GUI		0
#define SHOW_STATS		1
#define SHOW_SNMP_PORTS		2
#define SHOW_SNMP_GLOBALS	3

int mode = SHOW_GUI;


hexp_port_list_t port_list;

static struct minipc_ch *ptp_ch;

void init(int usecolor)
{
	halexp_client_init();

	ptp_ch = minipc_client_create("ptpd", 0);
	if (!ptp_ch)
	{
		fprintf(stderr,"Can't establish WRIPC connection "
			"to the PTP daemon!\n");
		exit(-1);
	}


	term_init(usecolor);
	halexp_query_ports(&port_list);
}

void show_ports(void)
{
	int i, j;
	time_t t;
	struct tm *tm;
	char datestr[32];

	if(mode == SHOW_GUI) {
		time(&t);
		tm = localtime(&t);
		strftime(datestr, sizeof(datestr), "%Y-%m-%d %H:%M:%S", tm);
		term_pcprintf(3, 1, C_BLUE, "Switch ports at %s\n", datestr);

		for(i=0; i<18;i++)
		{
			char if_name[10], found = 0;
			hexp_port_state_t state;

			snprintf(if_name, 10, "wr%d", i);

			for(j=0;j<port_list.num_ports;j++)
				if(!strcmp(port_list.port_names[j], if_name)) {
					found = 1; break;
				}

			if(!found) continue;

			halexp_get_port_state(&state, if_name);

			term_cprintf(C_WHITE, " %-5s: ", if_name);
			if(state.up)
				term_cprintf(C_GREEN, "Link up    ");
			else
				term_cprintf(C_RED, "Link down  ");

			term_cprintf(C_GREY, "mode: ");

			switch(state.mode)
			{
				case HEXP_PORT_MODE_WR_MASTER:
					term_cprintf(C_WHITE, "WR Master  ");
					break;
				case HEXP_PORT_MODE_WR_SLAVE:
					term_cprintf(C_WHITE, "WR Slave   ");
					break;
			}

			if(state.is_locked)
				term_cprintf(C_GREEN, "Locked  ");
			else
				term_cprintf(C_RED, "NoLock  ");

			if(state.rx_calibrated  && state.tx_calibrated)
				term_cprintf(C_GREEN, "Calibrated  \n");
			else
				term_cprintf(C_RED, "Uncalibrated  \n");
		}
	}
	else if(mode == SHOW_STATS) {
		printf("PORTS ");
		for(i=0; i<18; ++i) {
			char if_name[10], found = 0;
			hexp_port_state_t state;

			snprintf(if_name, 10, "wr%d", i);
			for(j=0;j<port_list.num_ports;j++)
				if(!strcmp(port_list.port_names[j], if_name)) { found = 1; break; }

			if(!found) continue;

			halexp_get_port_state(&state, if_name);
			printf("port:%s ", if_name);
			printf("lnk:%d ", state.up ? 1:0);
			printf("mode:%s ", state.mode==HEXP_PORT_MODE_WR_SLAVE ? "S":"M");
			printf("lock:%d ", state.is_locked ? 1:0);
		}
		printf("\n");
	}
	else if (mode == SHOW_SNMP_PORTS) {
		for(i=0; i<18; ++i) {
			char if_name[10], found = 0;
			hexp_port_state_t state;

			printf("PORT %i\n", i);
			snprintf(if_name, 10, "wr%d", i);
			for(j=0;j<port_list.num_ports;j++)
				if(!strcmp(port_list.port_names[j], if_name)) { found = 1; break; }

			if(!found) continue;

			halexp_get_port_state(&state, if_name);
			printf("linkup: %d\n", state.up ? 1:0);
			printf("mode: %d\n", state.mode==HEXP_PORT_MODE_WR_SLAVE
			       ? 0 : 1);
			printf("locked: %d\n", state.is_locked ? 1:0);
			printf("peer_id: ff:ff:ff:ff:ff:ff:ff:ff\n"); /* FIXME */
		}
	}
}

void show_servo(void)
{
	ptpdexp_sync_state_t ss;

	minipc_call(ptp_ch, 2000, &__rpcdef_get_sync_state, &ss);

	if(mode == SHOW_GUI) {
		term_cprintf(C_BLUE, "Synchronization status:\n");

		if(!ss.valid)
		{
			term_cprintf(C_RED, "Master mode or sync info not valid\n");
			return;
		}

		term_cprintf(C_GREY, "Servo state:               ");
		term_cprintf(C_WHITE, "%s\n", ss.slave_servo_state);

		term_cprintf(C_GREY, "Phase tracking:            ");
		if(ss.tracking_enabled)
			term_cprintf(C_GREEN, "ON\n");
		else
			term_cprintf(C_RED,"OFF\n");

		term_cprintf(C_GREY, "Synchronization source:    ");
		term_cprintf(C_WHITE, "%s\n", ss.sync_source);

		term_cprintf(C_BLUE, "\nTiming parameters:\n\n");

		term_cprintf(C_GREY, "Round-trip time (mu):      ");
		term_cprintf(C_WHITE, "%.3f nsec\n", ss.mu/1000.0);

		term_cprintf(C_GREY, "Master-slave delay:        ");
		term_cprintf(C_WHITE, "%.3f nsec\n", ss.delay_ms/1000.0);

		term_cprintf(C_GREY, "Link length:               ");
		term_cprintf(C_WHITE, "%.0f meters \n",
			     ss.delay_ms/1e12 * 300e6 / 1.55);

		term_cprintf(C_GREY, "Master PHY delays:         ");
		term_cprintf(C_WHITE, "TX: %.3f nsec, RX: %.3f nsec\n",
			     ss.delta_tx_m/1000.0, ss.delta_rx_m/1000.0);

		term_cprintf(C_GREY, "Slave PHY delays:          ");
		term_cprintf(C_WHITE, "TX: %.3f nsec, RX: %.3f nsec\n",
			     ss.delta_tx_s/1000.0, ss.delta_rx_s/1000.0);

		term_cprintf(C_GREY, "Total link asymmetry:      ");
		term_cprintf(C_WHITE, "%.3f nsec\n", ss.total_asymmetry/1000.0);

		if (0) {
			term_cprintf(C_GREY, "Fiber asymmetry:           ");
			term_cprintf(C_WHITE, "%.3f nsec\n", ss.fiber_asymmetry/1000.0);
		}

		term_cprintf(C_GREY, "Clock offset:              ");
		term_cprintf(C_WHITE, "%.3f nsec\n", ss.cur_offset/1000.0);

		term_cprintf(C_GREY, "Phase setpoint:            ");
		term_cprintf(C_WHITE, "%.3f nsec\n", ss.cur_setpoint/1000.0);

		term_cprintf(C_GREY, "Skew:                      ");
		term_cprintf(C_WHITE, "%.3f nsec\n", ss.cur_skew/1000.0);

		term_cprintf(C_GREY, "Servo update counter:      ");
		term_cprintf(C_WHITE, "%lld times\n", ss.update_count);
	}
	else if(mode == SHOW_STATS) {
		printf("SERVO ");
		printf("sv:%d ", ss.valid ? 1:0);
		printf("ss:'%s' ", ss.slave_servo_state);
		printf("mu:%llu ", ss.mu);
		printf("dms:%llu ", ss.delay_ms);
		printf("dtxm:%llu drxm:%llu ", ss.delta_tx_m, ss.delta_rx_m);
		printf("dtxs:%llu drxs:%llu ", ss.delta_tx_s, ss.delta_rx_s);
		printf("asym:%lld ", ss.total_asymmetry);
		printf("crtt:%llu ", ss.mu - ss.delta_tx_m - ss.delta_rx_m -
				ss.delta_tx_s - ss.delta_rx_s);
		printf("cko:%lld ", ss.cur_offset);
		printf("setp:%lld ", ss.cur_setpoint);
		printf("ucnt:%llu ", ss.update_count);
		printf("\n");
	} else if (mode == SHOW_SNMP_GLOBALS) {
		if(!ss.valid)
			return;
		/* This is oh so similar to the above, but by lines */
		printf("gm_id: f0:f0:f0:f0:f0:f0:f0:f0\n"); /* FIXME */
		printf("clock_id: f1:f1:f1:f1:f1:f1:f1:f1\n"); /* FIXME */
		printf("mode: 9999\n"); /* FIXME */
		printf("servo_state: %s\n", ss.slave_servo_state);
		printf("tracking: %i\n", ss.tracking_enabled ? 1 : 0);
		printf("source: %s\n", ss.sync_source);
		printf("ck_offset: %lli\n", ss.cur_offset);
		printf("skew: %li\n", (long)ss.cur_skew);
		printf("rtt: %lli\n", ss.mu);
		printf("llength: %li\n", (long)(ss.delay_ms/1e12 * 300e6 / 1.55));
		printf("servo_upd: %lli\n", ss.update_count);
	}
}

int track_onoff = 1;

void show_all()
{
	if (mode == SHOW_GUI) {
		term_clear();
		term_pcprintf(1, 1, C_BLUE,
			      "WR Switch Sync Monitor v 1.0 [q = quit]");
	}
	show_ports();
	show_servo();
	fflush(stdout);
}

int main(int argc, char *argv[])
{
	int opt;
	int usecolor = 1;

	while((opt=getopt(argc, argv, "sbgp")) != -1)
	{
		switch(opt)
		{
			case 's':
				mode = SHOW_STATS;
				break;
			case 'b':
				usecolor = 0;
				break;
			case 'g':
				mode = SHOW_SNMP_GLOBALS;
				init(0);
				show_all();
				exit(0);
			case 'p':
				mode = SHOW_SNMP_PORTS;
				init(0);
				show_all();
				exit(0);
			default:
				fprintf(stderr, "Unrecognized option.\n");
				break;
		}
	}

	init(usecolor);

	setvbuf(stdout, NULL, _IOFBF, 4096);
	for(;;)
	{
		if(term_poll(500))
		{
			int c = term_get();

			if(c=='q')
				break;

			if(c=='t') {
				int rval;
				track_onoff = 1-track_onoff;
				minipc_call(ptp_ch, 200, &__rpcdef_cmd,
					    &rval, PTPDEXP_COMMAND_TRACKING,
					    track_onoff);
			}
		}
		show_all();
		/* If we got broken pipe or anything, exit */
		if (ferror(stdout))
			exit(1);
	}
	term_restore();
	setlinebuf(stdout);
	printf("\n");
	return 0;
}
