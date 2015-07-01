/*
 * Copyright (c) 2015, CERN
 *
 * Author: Grzegorz Daniluk <grzegorz.daniluk@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <getopt.h>
#include <inttypes.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <libwr/switch_hw.h>
#include <libwr/wrs-msg.h>
#include <fpga_io.h>
#include <regs/wdog-regs.h>
#include "wrs_watchdog.h"

#define RST_THR 2

#define wdog_write(reg, val) \
	_fpga_writel(FPGA_BASE_WDOG + offsetof(struct WDOG_WB, reg), val)

#define wdog_read(reg) \
	_fpga_readl(FPGA_BASE_WDOG + offsetof(struct WDOG_WB, reg))

static char *prgname;
int daemon_mode;
int list_mode;
int port_num = 18;

uint32_t show_counter(void)
{
	uint32_t val;
	val = wdog_read(RST_CNT);
	printf("%u\n", val);
	return val;
}

void force_rst(void)
{
	wdog_write(CR, WDOG_CR_RST);
}

struct swc_fsms read_port(int port)
{
	int i;
	struct swc_fsms fsms;
	uint32_t val_fsm, val_act;

	wdog_write(CR, WDOG_CR_PORT_W(port));
	val_fsm = wdog_read(FSM);
	val_act = wdog_read(ACT);

	/* parsing results */
	fsms.state[0] = WDOG_FSM_IB_ALLOC_R(val_fsm);
	fsms.state[1] = WDOG_FSM_IB_TRANS_R(val_fsm);
	fsms.state[2] = WDOG_FSM_IB_RCV_R(val_fsm);
	fsms.state[3] = WDOG_FSM_IB_LL_R(val_fsm);
	fsms.state[4] = WDOG_FSM_OB_PREP_R(val_fsm);
	fsms.state[5] = WDOG_FSM_OB_SEND_R(val_fsm);
	fsms.state[6] = WDOG_FSM_FREE_R(val_fsm);
	for (i = 0; i < FSMS_NO; ++i) {
		fsms.act[i] = (val_act & (1<<i)) >> i;
	}

	return fsms;
}

void list_fsms(void)
{
	int i;
	struct swc_fsms fsms;

	for (i = 0; i <= port_num; ++i) {
		/* yes, it is i<=port_num because we're reading FSMs for all WR
		 * ports + NIC */
		fsms = read_port(i);
		printf("PORT %d: ", i);
		printf("ib_alloc(%d): %-14s",
			fsms.act[ALLOC_IDX], alloc_states[fsms.state[ALLOC_IDX]]);
		printf("ib_trans(%d): %-15s",
			fsms.act[TRANS_IDX], trans_states[fsms.state[TRANS_IDX]]);
		printf("ib_rcv(%d): %-12s",
			fsms.act[RCV_IDX], rcv_states[fsms.state[RCV_IDX]]);
		printf("ib_ll(%d): %-17s",
			fsms.act[LL_IDX], ll_states[fsms.state[LL_IDX]]);
		printf("ob_prep(%d): %-18s",
			fsms.act[PREP_IDX], prep_states[fsms.state[PREP_IDX]]);
		printf("ob_send(%d): %-14s",
			fsms.act[SEND_IDX], send_states[fsms.state[SEND_IDX]]);
		printf("free(%d): %s\n",
			fsms.act[FREE_IDX], free_states[fsms.state[FREE_IDX]]);
	}
}

int update_stuck(struct swc_fsms *fsms, int *stuck_cnt)
{
	int fsm_it;

	for (fsm_it = 0; fsm_it < FSMS_NO; ++fsm_it) {
		if (fsms->state[fsm_it] != idles[fsm_it] &&
			fsms->act[fsm_it] == 0) {

			stuck_cnt[fsm_it]++;
		} else {
			stuck_cnt[fsm_it] = 0;
		}
		/* and we also check if reset is needed */
		if (stuck_cnt[fsm_it] >= RST_THR) {
			return 1;
		}
	}
	return 0;
}

void clear_stuck(int cnt[][FSMS_NO])
{
	int port_it, fsm_it;

	for (port_it = 0; port_it <= port_num; ++port_it) {
	for (fsm_it = 0; fsm_it < FSMS_NO; ++fsm_it) {
		cnt[port_it][fsm_it] = 0;
	}
	}
}

void daemonize()
{
	pid_t pid, sid;

	if (getppid() == 1)
		return;
	pid = fork();
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* At this point we are executing as the child process */

	/* Change the file mode mask */
	umask(0);
	/* Create a new SID for the child process */
	sid = setsid();
	if (sid < 0) {
		exit(EXIT_FAILURE);
	}
	/* Change the current working directory.  This prevents the current
	   directory from being locked; hence not being able to remove it. */
	if ((chdir("/")) < 0) {
		exit(EXIT_FAILURE);
	}
	/* Redirect stdin to /dev/null -- keep output/error: they are logged */
	freopen("/dev/null", "r", stdin);
}

void endless_watchdog(void)
{
	struct swc_fsms fsms[19];
	int stuck_cnt[19][FSMS_NO] = { {0} };
	int port_it;
	int rst = 0;

	while (1) {
		/* first we read all the ports */
		for (port_it = 0; port_it <= port_num; ++port_it) {
			fsms[port_it] = read_port(port_it);
			rst = update_stuck(&(fsms[port_it]), stuck_cnt[port_it]);
			if (rst)
				break;
		}

		/* handle reset if needed */
		if (rst) {
			pr_warning("SWCore stuck... resetting\n");
			force_rst();
			clear_stuck(stuck_cnt);
		}

		sleep(1);
	}
}

void print_help(char *prgname)
{
	printf("usage: %s <options>\n", prgname);
	printf("   -d        Run as daemon in the background\n"
		"   -l        List FSMs state for all ports\n"
		"   -n <8/18> Set the number of ports\n"
		"   -g        Show current value of the restart counter\n"
		"   -r        Force restart of the Swcore\n"
		"   -h        Show this help message\n");
}

int main(int argc, char *argv[])
{
	int c = 0;

	prgname = argv[0];

	if (argc == 1) {
		print_help(prgname);
		return 0;
	}

	if (shw_fpga_mmap_init() < 0) {
		pr_error("%s: Can't access device memory\n", prgname);
		exit(1);
	}

	while ((c = getopt(argc, argv, "dhrgn:l")) != -1) {
		switch (c) {
		case 'd':
			daemon_mode = 1;
			break;
		case 'l':
			list_mode = 1;
			break;
		case 'r':
			force_rst();
			break;
		case 'g':
			show_counter();
			break;
		case 'n':
			port_num = atoi(optarg);
			break;
		case 'h':
		default:
			print_help(prgname);
			exit(1);
		}
	}

	if (!daemon_mode && list_mode) {
		list_fsms();
	}

	if (daemon_mode) {
		daemonize();
		endless_watchdog();
	}

	return 0;
}
