/*
 * wr_management.c
 *
 * Obtains the information that is displayed by the local management tool
 *
 *  Created on: Nov 11, 2013
 *  Authors:
 * 		- José Luis Gutiérrez (jgutierrez@ugr.es)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License...
 */

#include <stdio.h>
#include <stdlib.h>

#include <minipc.h>

#include "term.h"

#define PTP_EXPORT_STRUCTURES
#include "ptpd_exports.h"

#include "hal_client.h"

hexp_port_list_t port_list;

static struct minipc_ch *ptp_ch;

void print_args(int argc, char *argv[])
{
	printf("argc=%d: ",argc);
	while(argc>0)
	{
		printf("%s, ",argv[0]);
		argc--;
		argv++;
	}
	printf("\n");
}

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

void show_ports()
{
	int i, j;
	time_t t;
	struct tm *tm;
	char datestr[32];

	time(&t);
	tm = localtime(&t);
	strftime(datestr, sizeof(datestr), "%Y-%m-%d %H:%M:%S", tm);

	for(i=0; i<18;i++)
	{
		char if_name[10], found = 0;
		hexp_port_state_t state;

		snprintf(if_name, 10, "wr%d", i);

		for(j=0;j<port_list.num_ports;j++)
			if(!strcmp(port_list.port_names[j], if_name)) { found = 1; break; }

		if(!found) continue;

		halexp_get_port_state(&state, if_name);

		if(state.up)
			term_cprintf(C_GREEN, "up ");
		else
			term_cprintf(C_RED, "down ");

		switch(state.mode)
		{
			case HEXP_PORT_MODE_WR_MASTER:
				term_cprintf(C_WHITE, "Master ");
				break;
			case HEXP_PORT_MODE_WR_SLAVE:
				term_cprintf(C_WHITE, "Slave ");
				break;
		}

		if(state.is_locked)
			term_cprintf(C_GREEN, "Locked ");
		else
			term_cprintf(C_RED, "NoLock ");

		if(state.rx_calibrated  && state.tx_calibrated)
			term_cprintf(C_GREEN, "Calibrated \n");
		else
			term_cprintf(C_RED, "Uncalibrated \n");
	}
}


int track_onoff = 1;

void show_screen()
{
	term_clear();
	//term_pcprintf(1, 1, C_BLUE, "WR Switch Sync Monitor v 1.0 [q = quit]");

	show_ports();
	//show_servo();
	fflush(stdout);
}

int main(int argc, char **argv)
{
	int usecolor = 1;

	if(argc > 1){
		if(!strcmp(argv[1], "ports")){

			usecolor = 0;
			init(usecolor);

			setvbuf(stdout, NULL, _IOFBF, 4096);

				if(term_poll(500))
				{

						int rval;
						track_onoff = 1-track_onoff;
						minipc_call(ptp_ch, 200, &__rpcdef_cmd,
								&rval, PTPDEXP_COMMAND_TRACKING,
								track_onoff);

				}
				show_screen();

			term_restore();
			setlinebuf(stdout);
			printf("\n");

		}else if (!strcmp(argv[1], "fan")){


		}else{
			printf("\nParam required: ");
			printf("\nProgram usage: %s [param]", argv[0]);
			printf("\tports\t-->\tWRS ports state\n");
		}
	}else{
		printf("\nParam required: ");
		printf("\nProgram usage: %s [param]", argv[0]);
		printf("\tports\t-->\tWRS ports state\n");
	}


	return 0;
}
