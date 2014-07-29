/*
 * wrsw_version.c
 *
 * Obtain the HW version and FPGA type.
 *
 *  Created on: Jan 20, 2012
 *  Authors:
 * 		- Benoit RAT
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <shw_io.h>
#include <hwiu.h>
#include "switch_hw.h"


#ifndef __GIT_VER__
#define __GIT_VER__ "x.x"
#endif

#ifndef __GIT_USR__
#define __GIT_USR__ "?"
#endif


void help(const char* pgrname)
{
	printf("usage: %s <command>\n", pgrname);
	printf("available commands are:\n"
				"   -p PCB version\n"
				"   -f FPGA type\n"
				"   -F FPGA type and init status LED\n"
				"   -g Gateware version\n"
				"   -c Compiling time\n"
				"   -v version (git)\n"
				"   -t tagged-output for other programs\n"
				"   -a All (default)\n"
				"\n");
		exit(1);
}

static void print_gw_info(void)
{
	struct gw_info info;

	shw_fpga_mmap_init();
	if( shw_hwiu_gwver(&info) < 0 ) {
		fprintf(stderr, "Could not get GW version info\n");
		exit(1);
	}
	if( info.struct_ver != HWIU_STRUCT_VERSION )
		fprintf(stderr, "WARNING: dealing with unsupported "
			"info struct, sw:%u, gw:%u\n",
			HWIU_STRUCT_VERSION, info.struct_ver);

	/* Use tagged format, this is a subset of the "-t" operation */
	printf("gateware-version: %u.%u\n", info.ver_major, info.ver_minor);
	printf("gateware-build: %02u/%02u/%02u.%02u\n", info.build_day,
		info.build_month, info.build_year, info.build_no);
	printf("wr_switch_hdl-commit: %07x\n", info.switch_hdl_hash);
	printf("general-cores-commit: %07x\n", info.general_cores_hash);
	printf("wr-cores-commit: %07x\n", info.wr_cores_hash);
}

/* Print everything in tagged format, for snmp parsing etc */
static void wrsw_tagged_versions(void)
{
	printf("software-version: %s\n", __GIT_VER__); /* see Makefile */
	printf("bult-by: %s\n", __GIT_USR__); /* see Makefile */
	printf("build-date: %s %s\n", __DATE__, __TIME__);
	printf("pcb-version: %s\n", get_shw_info('p'));
	printf("fpga-type: %s\n", get_shw_info('f'));
	print_gw_info(); /* This is already tagged */
}

int main(int argc, char **argv)
{
	char func='a';
	if(argc>=2 && argv[1][0]=='-')
	{
		func=argv[1][1];
	}
	else func='a';

	assert_init(shw_pio_mmap_init());
	shw_io_init();
	shw_io_configure_all();

	switch(func)
	{
	case 'F':
		/* When the linux has boot the status led
		 * is unpowered so we set it to yellow
		 * until the HAL start (LED is orange) and finally
		 * WR is setup (LED is green) */
		if(shw_io_read(shw_io_led_state_g)==0)
		{
			shw_io_write(shw_io_led_state_o,1);
			shw_io_write(shw_io_led_state_g,1);
		}
		func='f';
		/* fall through */
	case 'p':
	case 'f':
		/* Warning: this -p and -f is used by the web interface */
		printf("%s\n",get_shw_info(func));
		break;
	case 'g':
		print_gw_info();
		break;
	case 'c':
		/* Warning: this -c is used by the web interface */
		printf("%s %s\n",__DATE__,__TIME__);
		break;
	case 'v':
		printf("%s %s\n",__GIT_VER__,__GIT_USR__);
		break;
	case 't':
		wrsw_tagged_versions();
		break;
	case 'a':
		/* Warning: this with "awk '{print $4}'" is ued by the web if */
		printf("PCB:%s, FPGA:%s; version: %s (%s); compiled at %s %s\n",
		       get_shw_info('p'), get_shw_info('f'),
		       __GIT_VER__, __GIT_USR__, __DATE__, __TIME__);
		break;
	case 'h':
	default:
		help(argv[0]);
		return 1;
	}
	return 0;
}
